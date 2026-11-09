
#include "MainComponent.h"

class MainComponent::SampleListModel final : public juce::ListBoxModel
{
public:
    explicit SampleListModel(MainComponent& component) : owner(component) {}

    int getNumRows() override { return visibleFiles.size(); }

    void paintListBoxItem(int row, juce::Graphics& g, int width, int height,
                          bool selected) override
    {
        if (!juce::isPositiveAndBelow(row, visibleFiles.size())) return;
        g.fillAll(selected ? juce::Colour(0xff71300f) : juce::Colour(0xff202329));
        g.setColour(selected ? juce::Colour(0xffffa15b) : juce::Colour(0xffe5e5e5));
        g.setFont(13.0f);
        g.drawText(visibleFiles[row].getFileName(), 10, 0, width - 90, height,
                   juce::Justification::centredLeft, true);
        g.setColour(juce::Colour(0xffa7abb3));
        g.drawText(visibleFiles[row].getFileExtension().trimCharactersAtStart(".").toUpperCase(),
                   width - 75, 0, 65, height, juce::Justification::centredRight);
    }

    void selectedRowsChanged(int row) override
    {
        if (juce::isPositiveAndBelow(row, visibleFiles.size()))
            owner.loadSample(visibleFiles[row]);
    }

    void refresh(const juce::Array<juce::File>& files, const juce::String& query,const juce::String& category={})
    {
        visibleFiles.clear();
        for (const auto& file : files)
        {
            bool categoryMatches=category.isEmpty()||category=="All categories";
            if(!categoryMatches)for(const auto&r:owner.project.samples)if(r.path==file.getFullPathName()&&r.category==category){categoryMatches=true;break;}
            if(categoryMatches&&(query.isEmpty()||file.getFileName().containsIgnoreCase(query)))visibleFiles.add(file);
        }
        if (owner.selectedSample != juce::File())
        {
            for (int i = 0; i < visibleFiles.size(); ++i)
                if (visibleFiles[i] == owner.selectedSample) { owner.sampleList.selectRow(i); break; }
        }
        owner.sampleList.updateContent();
        owner.sampleList.repaint();
    }

private:
    MainComponent& owner;
    juce::Array<juce::File> visibleFiles;
};

namespace
{
    const juce::Colour backgroundColour { 0xff101114 };
    const juce::Colour sidebarColour    { 0xff17191e };
    const juce::Colour panelColour      { 0xff1c1f25 };
    const juce::Colour lighterPanel     { 0xff252932 };
    const juce::Colour accentColour     { 0xffea5910 };
    const juce::Colour textColour       { 0xfff3f3f3 };
    const juce::Colour mutedColour      { 0xff969ba5 };
    const juce::Colour borderColour     { 0xff30343d };
}

MainComponent::MainComponent()
{
    setLookAndFeel(&offorLookAndFeel);
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    formatManager.registerBasicFormats();
    juce::PropertiesFile::Options settingsOptions;
    settingsOptions.applicationName = "OFFOR SAMPLE FORGE";
    settingsOptions.filenameSuffix = "settings";
    settingsOptions.folderName = "OFFOR AUDIO";
    settingsOptions.osxLibrarySubFolder = "Application Support";
    userSettings = std::make_unique<juce::PropertiesFile>(settingsOptions);
    tooltipWindow.setLookAndFeel(&offorLookAndFeel);
    pluginPanel = std::make_unique<PluginPanel>(*userSettings, [this](const juce::String& xml)
    {
        if (!project.isOpen()) return;
        project.pluginCatalogXml = xml;
        juce::String error;
        if (!project.save(error)) statusLabel.setText("Could not save plugin catalog: " + error, juce::dontSendNotification);
    });
    addAndMakeVisible(*pluginPanel);
    pluginPanel->setVisible(false);

    logo = juce::ImageFileFormat::loadFrom(
        getResourcesDirectory().getChildFile("OfforSForge.png"));

    setSize(1240, 700);
    setAudioChannels(0, 2);

    addAndMakeVisible(processingPanel);
    processingPanel.setVisible(false);
    addAndMakeVisible(variationPanel);
    variationPanel.setVisible(false);
    processingPanel.setCallbacks(
        [this](const std::vector<EffectSettings>& effects)
        { project.processingChainSettings = effects; if (project.isOpen()) updateProjectFromSamples();for(const auto&r:project.samples)if(r.path==selectedSample.getFullPathName()){variationPanel.setSource(selectedSample,r.id,r.category,project.getAssetsDirectory(),effects);break;} },
        [this](const ProcessingRecord& record)
        { if (project.isOpen()) { project.processingHistory.add(record); juce::String error; if (!project.save(error)) statusLabel.setText("Could not save processing history: " + error, juce::dontSendNotification); processingPanel.setHistory(project.processingHistory.all()); } },
        [this](const juce::File& file) { setPreviewFile(file); },
        [this](const RenderOperations& operations) { project.processingOperations = operations; if (project.isOpen()) updateProjectFromSamples(); });
    processingPanel.setChain(project.processingChainSettings);
    processingPanel.setOperations(project.processingOperations);
    processingPanel.setHistory(project.processingHistory.all());
    variationPanel.setData(project.variationSettings,project.variationHistory.all());
    variationPanel.setCallbacks(
        [this](const VariationSettings& settings,const juce::Array<Variation>& records)
        { project.variationSettings=settings;project.variationHistory.setAll(records);if(project.isOpen()){juce::String error;if(!project.save(error))statusLabel.setText("Could not save variations: "+error,juce::dontSendNotification);} },
        [this](const juce::File& file){if(file==juce::File()){transport.stop();isPlaying=false;playButton.setButtonText("Play Preview");}else if(setPreviewFile(file)){transport.start();isPlaying=true;playButton.setButtonText("Playing Preview");}});

    sampleListModel = std::make_unique<SampleListModel>(*this);
    sampleList.setModel(sampleListModel.get());
    sampleList.setRowHeight(29);
    sampleList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff202329));
    addAndMakeVisible(sampleList);
    searchField.setTextToShowWhenEmpty("Filter imported samples...", juce::Colour(0xff9297a0));
    searchField.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff202329));
    searchField.setColour(juce::TextEditor::textColourId, juce::Colour(0xfff3f3f3));
    searchField.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff30343d));
    searchField.onTextChange = [this]
    {
        if (sampleListModel != nullptr) sampleListModel->refresh(sampleFiles, searchField.getText(),categoryFilterBox.getText());
    };
    addAndMakeVisible(searchField);

    for (const auto& category : { "Uncategorized", "Kick", "Snare", "Clap", "Hi-Hat", "Open Hi-Hat", "Closed Hi-Hat", "Percussion", "808", "Bass", "Melody", "Chord", "Pad", "Vocal", "Vocal Chop", "FX", "Riser", "Impact", "Texture", "Loop", "Other" })
        categoryBox.addItem(category, categoryBox.getNumItems() + 1);
    categoryFilterBox.addItem("All categories",1);for(int i=0;i<categoryBox.getNumItems();++i)categoryFilterBox.addItem(categoryBox.getItemText(i),i+2);categoryFilterBox.setSelectedId(1);addAndMakeVisible(categoryFilterBox);
    categoryFilterBox.onChange=[this]{if(sampleListModel)sampleListModel->refresh(sampleFiles,searchField.getText(),categoryFilterBox.getText());};
    categoryBox.onChange = [this]
    {
        for (auto& record : project.samples) if (record.path == selectedSample.getFullPathName())
        { record.category = categoryBox.getText(); record.manuallyCategorized = true;variationPanel.setSource(selectedSample,record.id,record.category,project.isOpen()?project.getAssetsDirectory():juce::File(),project.processingChainSettings); break; }
        updateProjectFromSamples();
    };
    addAndMakeVisible(categoryBox);
    metadataLabel.setColour(juce::Label::textColourId, mutedColour);
    metadataLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(metadataLabel);
    createButton(newProjectButton, "New", lighterPanel, [this] { createProject(); });
    createButton(openProjectButton, "Open", lighterPanel, [this] { openProject(); });
    createButton(saveProjectButton, "Save", lighterPanel, [this] { saveProject(false); });
    createButton(saveAsProjectButton, "Save As", lighterPanel, [this] { saveProject(true); });
    createButton(closeProjectButton, "Close", lighterPanel, [this] { closeProject(); });
    createButton(renameProjectButton, "Rename", lighterPanel, [this] { renameProject(); });
    newProjectButton.setTooltip("Create a new sample project and its managed asset folder.");
    openProjectButton.setTooltip("Open an existing OFFOR SAMPLE FORGE project (.osfproj).");
    saveProjectButton.setTooltip("Save the current project, sample categories and audio settings.");
    saveAsProjectButton.setTooltip("Save a copy of the current project under a new file name.");
    closeProjectButton.setTooltip("Close the current project and return to the dashboard.");
    renameProjectButton.setTooltip("Change the name shown for the current project.");
    OfforLookAndFeel::setIcon(openProjectButton,"open");OfforLookAndFeel::setIcon(saveProjectButton,"save");OfforLookAndFeel::setIcon(saveAsProjectButton,"save");OfforLookAndFeel::setIcon(importButton,"import");OfforLookAndFeel::setIcon(exportTopButton,"export");
    createButton(cancelImportButton, "Cancel Import", lighterPanel, [this]
    {
        ++importGeneration;
        if (auto* job = importPool.getJob(0)) job->signalJobShouldExit();
        importProgressValue = 0.0; cancelImportButton.setVisible(false);
        statusLabel.setText("Import cancelled.", juce::dontSendNotification);
    });
    cancelImportButton.setButtonText("");OfforLookAndFeel::setIcon(cancelImportButton,"stop");cancelImportButton.setTooltip("Cancel the active folder import.");
    createButton(relinkButton, "Relink Missing", lighterPanel, [this] { relinkMissingSample(); });
    cancelImportButton.setTooltip("Stop the current background folder import.");
    relinkButton.setTooltip("Choose a replacement file for a missing source sample.");
    addAndMakeVisible(importProgress);
    importProgress.setColour(juce::ProgressBar::foregroundColourId, accentColour);
    cancelImportButton.setVisible(false);
    relinkButton.setVisible(false);

    auto addNav = [this](juce::TextButton& button,
                         const juce::String& name)
    {
        createButton(button, name, sidebarColour,
                     [this, name] { showPage(name); });

        button.setColour(
            juce::TextButton::textColourOffId, mutedColour);
    };

    addNav(homeButton, "Home");
    addNav(libraryButton, "Sample Library");
    addNav(processingButton, "Processing");
    addNav(variationsButton, "Variations");
    addNav(pluginsButton, "VST3 Plugins");
    addNav(exportButton, "Export");
    addNav(settingsButton, "Settings");
    juce::TextButton* navItems[] = { &homeButton,&libraryButton,&processingButton,&variationsButton,&pluginsButton,&exportButton,&settingsButton };
    for(auto* item:navItems){item->setClickingTogglesState(true);item->setColour(juce::TextButton::buttonColourId,sidebarColour);item->setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff342014));}
    homeButton.setButtonText("Home");libraryButton.setButtonText("Sample Library");processingButton.setButtonText("Processing");variationsButton.setButtonText("Variations");pluginsButton.setButtonText("VST3 Plugins");exportButton.setButtonText("Export");settingsButton.setButtonText("Settings");
    OfforLookAndFeel::setIcon(homeButton,"home");OfforLookAndFeel::setIcon(libraryButton,"library");OfforLookAndFeel::setIcon(processingButton,"processing");OfforLookAndFeel::setIcon(variationsButton,"variations");OfforLookAndFeel::setIcon(pluginsButton,"plugins");OfforLookAndFeel::setIcon(exportButton,"export");OfforLookAndFeel::setIcon(settingsButton,"settings");
    startupAudioToggle.setButtonText("Play startup sound on launch");
    startupAudioToggle.setToggleState(userSettings->getBoolValue("playStartupAudio",true),juce::dontSendNotification);
    startupAudioToggle.onClick=[this]{userSettings->setValue("playStartupAudio",startupAudioToggle.getToggleState());userSettings->saveIfNeeded();};
    startupAudioToggle.setColour(juce::ToggleButton::textColourId,textColour);
    addAndMakeVisible(startupAudioToggle);

    createButton(importButton, "Import Folder", accentColour,
                 [this] { chooseSampleFolder(); });

    createButton(exportTopButton, "Export Pack", accentColour,
                 [this] { showPage("Export"); });
    importButton.setTooltip("Scan a folder recursively and import supported audio files.");
    exportTopButton.setTooltip("Open the export workspace for variation WAV and ZIP exports.");
    createButton(helpButton,"",lighterPanel,[this]{juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,"OFFOR SAMPLE FORGE Quick Help","Hover over controls for parameter guidance. Start with New, then Import Folder. Processing is offline: add an effect, adjust its labeled parameters, and render a copy. Variations are generated from the selected imported sample. Plugin entries are discovered metadata only; VST3 hosting is not available in this build.","Close",this);});
    OfforLookAndFeel::setIcon(helpButton,"help");helpButton.setTooltip("Quick Help: hover over controls for explanations; open the manual for complete workflows.");

    createButton(playButton, "Play Preview", lighterPanel,[this]{togglePreviewPlayback();});

    createButton(stopButton, "Stop", lighterPanel,
                 [this]
    {
        transport.stop();
        transport.setPosition(0.0);
        isPlaying = false;
        playButton.setButtonText("Play Preview");
    });
    OfforLookAndFeel::setIcon(playButton,"play");OfforLookAndFeel::setIcon(stopButton,"stop");
    playButton.setTooltip("Play the selected sample preview.");
    stopButton.setTooltip("Stop preview playback and return to the start.");

    auto addEffect = [this](juce::TextButton& button,
                            const juce::String& name,
                            EffectType type)
    {
        createButton(button, name, lighterPanel,
                     [this, name, type]
        {
            auto effects = processingPanel.getChain();
            effects.push_back(EffectSettings::defaults(type));
            processingPanel.setChain(effects);
            showPage("Processing");
            if (project.isOpen()) { project.processingChainSettings = effects; updateProjectFromSamples(); }
            statusLabel.setText(name + " added to the processing chain.", juce::dontSendNotification);
        });
    };

    addEffect(eqButton, "EQ", EffectType::parametricEq);
    addEffect(compressorButton, "Compressor", EffectType::compressor);
    addEffect(distortionButton, "Saturation", EffectType::saturation);
    addEffect(reverbButton, "Reverb", EffectType::reverb);

    createButton(homeImportButton,"Import Samples",lighterPanel,[this]{chooseSampleFolder();});
    createButton(homeNewButton,"New Project",lighterPanel,[this]{createProject();});
    createButton(homeOpenButton,"Open Project",lighterPanel,[this]{openProject();});
    createButton(exportVariationsButton,"Open Variations Export",accentColour,[this]{showPage("Variations");statusLabel.setText("Select cards or use Export Selected for WAV and ZIP options.",juce::dontSendNotification);});
    searchField.setTooltip("Filter the imported sample list by file name.");
    categoryFilterBox.setTooltip("Show samples assigned to one category.");
    categoryBox.setTooltip("Correct the selected sample category; this manual change is saved in the project.");
    startupAudioToggle.setTooltip("Enable or disable the one-time startup sound.");

    addAndMakeVisible(pageTitle);
    addAndMakeVisible(pageSubtitle);
    addAndMakeVisible(selectedSampleLabel);
    addAndMakeVisible(sampleCountLabel);
    addAndMakeVisible(statusLabel);
    addAndMakeVisible(projectHeading);
    audioStatusLabel.setColour(juce::Label::textColourId,mutedColour);audioStatusLabel.setJustificationType(juce::Justification::centredRight);audioStatusLabel.setFont(juce::Font(10.5f));audioStatusLabel.setText("Audio output: initializing",juce::dontSendNotification);addAndMakeVisible(audioStatusLabel);

    pageTitle.setColour(juce::Label::textColourId, textColour);
    pageTitle.setFont(juce::Font(25.0f, juce::Font::bold));

    pageSubtitle.setColour(juce::Label::textColourId, mutedColour);

    selectedSampleLabel.setColour(
        juce::Label::textColourId, textColour);

    sampleCountLabel.setColour(
        juce::Label::textColourId, mutedColour);

    statusLabel.setColour(
        juce::Label::textColourId, mutedColour);
    projectHeading.setColour(juce::Label::textColourId, textColour);
    projectHeading.setFont(juce::Font(13.0f,juce::Font::bold));

    pageTitle.setText(
        "Your creative workspace",
        juce::dontSendNotification);

    pageSubtitle.setText(
        "Design, transform and organize your own sample packs.",
        juce::dontSendNotification);

    selectedSampleLabel.setText(
        "No sample selected",
        juce::dontSendNotification);

    sampleCountLabel.setText(
        "0 samples imported",
        juce::dontSendNotification);

    statusLabel.setText(
        "Ready. Import a sample folder to get started.",
        juce::dontSendNotification);
    updateProjectHeading();
    showPage("Home");
    playStartupSound();
}

MainComponent::~MainComponent()
{
    ++importGeneration;
    importPool.removeAllJobs(true, 5000);
    transport.stop();
    transport.setSource(nullptr);
    shutdownAudio();
    setLookAndFeel(nullptr);
}

void MainComponent::createButton(
    juce::TextButton& button,
    const juce::String& text,
    juce::Colour colour,
    std::function<void()> action)
{
    addAndMakeVisible(button);

    button.setButtonText(text);
    button.setColour(juce::TextButton::textColourOffId,textColour);

    button.setColour(
        juce::TextButton::buttonColourId, colour);

    button.setColour(
        juce::TextButton::buttonOnColourId, accentColour);

    button.setColour(
        juce::TextButton::textColourOffId, textColour);

    button.onClick = std::move(action);
}

juce::File MainComponent::getResourcesDirectory() const
{
    return juce::File::getSpecialLocation(
        juce::File::currentExecutableFile)
        .getParentDirectory()
        .getChildFile("Resources");
}

void MainComponent::playStartupSound()
{
    if (!startupAudioToggle.getToggleState()) return;
    const auto soundFile =
        getResourcesDirectory().getChildFile("OfforSForge.mp3");

    if (!soundFile.existsAsFile())
        return;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(soundFile));

    if (reader == nullptr)
        return;

    currentReaderSource =
        std::make_unique<juce::AudioFormatReaderSource>(
            reader.release(), true);

    transport.setSource(
        currentReaderSource.get(),
        0,
        nullptr,
        currentReaderSource->getAudioFormatReader()->sampleRate);

    transport.start();
}

void MainComponent::prepareToPlay(
    int samplesPerBlockExpected,
    double sampleRate)
{
    outputSampleRate=sampleRate;
    auto safe=juce::Component::SafePointer<MainComponent>(this);juce::MessageManager::callAsync([safe,sampleRate]{if(safe)safe->audioStatusLabel.setText("Audio output · "+juce::String(sampleRate/1000.0,1)+" kHz",juce::dontSendNotification);});
    transport.prepareToPlay(
        samplesPerBlockExpected, sampleRate);
}

bool MainComponent::keyPressed(const juce::KeyPress& key,juce::Component* originatingComponent)
{
    const auto code=key.getKeyCode();
    if(key.getModifiers().isCtrlDown())
    {
        const auto character=juce::CharacterFunctions::toLowerCase((juce::juce_wchar)key.getTextCharacter());
        if(character=='s'){saveProject(false);return true;}
        if(character=='o'){openProject();return true;}
    }
    if(code==juce::KeyPress::spaceKey)
    {
        if(originatingComponent!=nullptr&&(dynamic_cast<juce::TextEditor*>(originatingComponent)!=nullptr||originatingComponent->findParentComponentOfClass<juce::TextEditor>()!=nullptr))return false;
        togglePreviewPlayback();return true;
    }
    if(code==juce::KeyPress::deleteKey&&currentPage=="Variations")
    {variationPanel.deleteSelected();return true;}
    return false;
}

void MainComponent::togglePreviewPlayback()
{
    if(currentReaderSource==nullptr){statusLabel.setText("Select a sample or variation preview first.",juce::dontSendNotification);return;}
    if(transport.isPlaying()){transport.stop();isPlaying=false;playButton.setButtonText("Resume Preview");}
    else{if(transport.getCurrentPosition()>=transport.getLengthInSeconds())transport.setPosition(0.0);transport.start();isPlaying=true;playButton.setButtonText("Playing Preview");}
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill)
{
    transport.getNextAudioBlock(bufferToFill);

    if (isPlaying && !transport.isPlaying())
    {
        isPlaying = false;
        playButton.setButtonText("Play Preview");
    }
}

void MainComponent::releaseResources()
{
    transport.releaseResources();
}

void MainComponent::chooseSampleFolder()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose a folder containing audio samples",
        juce::File(),
        "*");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode |
        juce::FileBrowserComponent::canSelectDirectories,

        [this](const juce::FileChooser& chooser)
        {
            const auto folder = chooser.getResult();

            if (!folder.isDirectory())
                return;

            importProgressValue = 0.0;
            const auto generation = ++importGeneration;
            cancelImportButton.setVisible(true);
            statusLabel.setText("Scanning audio files in background...", juce::dontSendNotification);
            juce::Component::SafePointer<MainComponent> safe(this);
            auto* job = new SampleImporter(folder, project.samples,
                [safe, folder, generation](juce::Array<SampleRecord> records, int unsupported, int added)
                {
                    if (safe == nullptr || safe->importGeneration != generation) return;
                    safe->project.samples = records;
                    safe->sampleFiles.clear();
                    for (const auto& record : records) safe->sampleFiles.add(juce::File(record.path));
                    safe->cancelImportButton.setVisible(false); safe->importProgressValue = 1.0;
                    safe->sampleCountLabel.setText(juce::String(records.size()) + " samples", juce::dontSendNotification);
                    safe->sampleListModel->refresh(safe->sampleFiles, safe->searchField.getText());
                    safe->statusLabel.setText("Added " + juce::String(added) + ", skipped " + juce::String(unsupported) + " unsupported; source: " + folder.getFileName(), juce::dontSendNotification);
                    safe->updateProjectFromSamples();
                    if (!safe->sampleFiles.isEmpty() && !safe->selectedSample.existsAsFile()) safe->loadSample(safe->sampleFiles.getFirst());
                    safe->repaint();
                }, [safe, generation](double progress)
                {
                    if (safe != nullptr && safe->importGeneration == generation) { safe->importProgressValue = progress; safe->repaint(); }
                });
            importPool.addJob(job, true);
        });
}

void MainComponent::loadSample(const juce::File& file)
{
    selectedSample = file;
    juce::String sampleId;
    for (const auto& record : project.samples) if (record.path == file.getFullPathName()) { sampleId = record.id; break; }
    processingPanel.setSource(file, sampleId, project.isOpen() ? project.getAssetsDirectory() : juce::File());
    juce::String selectedCategory;for(const auto& record:project.samples)if(record.id==sampleId){selectedCategory=record.category;break;}
    variationPanel.setSource(file,sampleId,selectedCategory,project.isOpen()?project.getAssetsDirectory():juce::File(),project.processingChainSettings);
    if (!setPreviewFile(file))
    {
        statusLabel.setText(
            (file.existsAsFile() ? "Could not decode: " : "Missing source — use Relink Missing: ") + file.getFileName(),
            juce::dontSendNotification);
        relinkButton.setVisible(!file.existsAsFile());
        return;
    }

    relinkButton.setVisible(false);

    selectedSampleLabel.setText(
        file.getFileName(),
        juce::dontSendNotification);
    for (const auto& record : project.samples) if (record.path == file.getFullPathName())
    {
        for (int i=0;i<categoryBox.getNumItems();++i) if(categoryBox.getItemText(i)==record.category) { categoryBox.setSelectedId(categoryBox.getItemId(i),juce::dontSendNotification); break; }
        if (!file.existsAsFile()) relinkButton.setVisible(true);
        break;
    }
    updateSampleDetails();
    updateProjectFromSamples();
    statusLabel.setText(
        "Selected: " + file.getFileName(),
        juce::dontSendNotification);

    repaint();
}

bool MainComponent::setPreviewFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr) return false;
    transport.stop(); transport.setSource(nullptr);
    currentReaderSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
    transport.setSource(currentReaderSource.get(), 0, nullptr, currentReaderSource->getAudioFormatReader()->sampleRate);
    thumbnail.setSource(new juce::FileInputSource(file));
    isPlaying = false; playButton.setButtonText("Play Preview"); repaint(); return true;
}

void MainComponent::setDashboardControlsVisible(bool visible)
{
    selectedSampleLabel.setVisible(visible); categoryBox.setVisible(visible); searchField.setVisible(visible); sampleList.setVisible(visible);
    categoryFilterBox.setVisible(visible);
    metadataLabel.setVisible(visible); playButton.setVisible(visible); stopButton.setVisible(visible); relinkButton.setVisible(visible&&!selectedSample.existsAsFile());
}

void MainComponent::updateProjectHeading()
{
    projectHeading.setText(project.isOpen()?project.getName():"No project open",juce::dontSendNotification);
    sampleCountLabel.setText(juce::String(project.samples.size())+" samples",juce::dontSendNotification);
    repaint();
}

bool MainComponent::updateProjectFromSamples()
{
    if (!project.isOpen()) return false;
    const juce::File current(selectedSample);
    for (auto& record : project.samples) if (current.existsAsFile() && record.path == current.getFullPathName()) project.selectedSampleId = record.id;
    juce::String error; if (!project.save(error)) { statusLabel.setText("Project save failed: " + error, juce::dontSendNotification); return false; }
    updateProjectHeading();
    repaint();
    return true;
}

void MainComponent::updateSampleDetails()
{
    for (const auto& r : project.samples) if (r.path == selectedSample.getFullPathName())
    {
        metadataLabel.setTooltip(r.path);
        metadataLabel.setText(juce::String(r.durationSeconds, 2) + " s   " + juce::String((int)r.sampleRate) + " Hz   " + juce::String(r.bitDepth) + " bit   " + juce::String(r.channels) + " ch   " + juce::String(r.fileSize / 1024) + " KB   " + r.format + "   " + r.features.classificationStatus + " " + juce::String(r.features.confidence * 100.0, 0) + "%", juce::dontSendNotification);
        return;
    }
    metadataLabel.setText("Metadata is available after importing into a project.", juce::dontSendNotification);
    metadataLabel.setTooltip(selectedSample.getFullPathName());
}

void MainComponent::createProject()
{
    auto dialog = std::make_shared<juce::AlertWindow>("New Project", "Choose a project name", juce::AlertWindow::NoIcon);
    dialog->addTextEditor("name", "Untitled Sample Pack"); dialog->addButton("Choose Location...", 1); dialog->addButton("Cancel", 0);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int result)
    {
        if (result != 1) return;
        const auto name = dialog->getTextEditorContents("name");
        fileChooser = std::make_unique<juce::FileChooser>("Create project", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.osfproj");
        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this, name](const juce::FileChooser& c)
        {
            auto f=c.getResult(); if (!f.getFileExtension().equalsIgnoreCase(".osfproj")) f=f.withFileExtension("osfproj");
            variationPanel.cancelGeneration();if (!project.create(f,name)) { statusLabel.setText("Could not create project.",juce::dontSendNotification); return; }
            ++importGeneration; importPool.removeAllJobs(true,5000);
            processingPanel.setChain(project.processingChainSettings);processingPanel.setOperations(project.processingOperations);processingPanel.setHistory(project.processingHistory.all());variationPanel.setData(project.variationSettings,project.variationHistory.all());if(project.pluginCatalogXml.isNotEmpty())pluginPanel->setCatalog(project.pluginCatalogXml);
            updateProjectHeading();updateProjectFromSamples(); repaint();
        });
    }));
}

void MainComponent::openProject()
{
    fileChooser=std::make_unique<juce::FileChooser>("Open Sample Forge project",juce::File(),"*.osfproj");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c)
    {
        variationPanel.cancelGeneration();juce::String error; if (!project.open(c.getResult(),error)) { statusLabel.setText(error,juce::dontSendNotification); return; }
        ++importGeneration; importPool.removeAllJobs(true,5000);
        processingPanel.setChain(project.processingChainSettings);processingPanel.setOperations(project.processingOperations);processingPanel.setHistory(project.processingHistory.all());variationPanel.setData(project.variationSettings,project.variationHistory.all());if(project.pluginCatalogXml.isNotEmpty())pluginPanel->setCatalog(project.pluginCatalogXml);
        updateProjectHeading();
        selectedSample = {};
        sampleFiles.clear(); for (const auto& r:project.samples) sampleFiles.add(juce::File(r.path));
        sampleListModel->refresh(sampleFiles,searchField.getText());
        if (!project.selectedSampleId.isEmpty()) for (const auto& r:project.samples) if (r.id==project.selectedSampleId) { selectedSample=juce::File(r.path); break; }
        if (selectedSample == juce::File() && !sampleFiles.isEmpty()) selectedSample=sampleFiles.getFirst();
        if (selectedSample != juce::File()) loadSample(selectedSample);
        relinkButton.setVisible(std::any_of(project.samples.begin(),project.samples.end(),[](const SampleRecord& r){return !juce::File(r.path).existsAsFile();}));
        sampleCountLabel.setText(juce::String(project.samples.size())+" samples",juce::dontSendNotification); repaint();
    });
}

void MainComponent::saveProject(bool saveAs)
{
    if (project.isOpen() && !saveAs) { if (updateProjectFromSamples()) statusLabel.setText("Project saved.",juce::dontSendNotification); return; }
    fileChooser=std::make_unique<juce::FileChooser>("Save Sample Forge project",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(project.getName()+".osfproj"),"*.osfproj");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c)
    { auto f=c.getResult(); if(!f.getFileExtension().equalsIgnoreCase(".osfproj")) f=f.withFileExtension("osfproj"); juce::String error; if(!project.saveAs(f,error)) statusLabel.setText(error,juce::dontSendNotification); else updateProjectFromSamples(); repaint(); });
}

void MainComponent::closeProject()
{
    variationPanel.cancelGeneration();
    ++importGeneration;
    importPool.removeAllJobs(true,5000); transport.stop(); transport.setSource(nullptr); selectedSample={}; sampleFiles.clear(); project.close();
    thumbnail.clear(); sampleListModel->refresh(sampleFiles,{}); metadataLabel.setText({},juce::dontSendNotification); sampleCountLabel.setText("0 samples",juce::dontSendNotification); processingPanel.setChain({});processingPanel.setOperations({});processingPanel.setHistory({});processingPanel.setSource({}, {}, {});variationPanel.setData({},{});variationPanel.setSource({}, {}, {}, {}, {}); statusLabel.setText("Project closed.",juce::dontSendNotification);updateProjectHeading(); repaint();
}

void MainComponent::renameProject()
{
    if (!project.isOpen()) { statusLabel.setText("Open a project before renaming it.",juce::dontSendNotification); return; }
    auto dialog=std::make_shared<juce::AlertWindow>("Rename Project","Enter the new project name",juce::AlertWindow::NoIcon);
    dialog->addTextEditor("name",project.getName());dialog->addButton("Rename",1);dialog->addButton("Cancel",0);
    dialog->enterModalState(true,juce::ModalCallbackFunction::create([this,dialog](int result)
    { if(result!=1)return; if(!project.rename(dialog->getTextEditorContents("name"))){statusLabel.setText("Project name cannot be empty.",juce::dontSendNotification);return;} updateProjectHeading();updateProjectFromSamples(); statusLabel.setText("Project renamed and saved.",juce::dontSendNotification); repaint(); }));
}

void MainComponent::relinkMissingSample()
{
    if (selectedSample == juce::File()) { statusLabel.setText("Select a missing sample in the project first.",juce::dontSendNotification); return; }
    fileChooser=std::make_unique<juce::FileChooser>("Relink missing sample",juce::File(),"*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& c)
    { auto f=c.getResult(); if(!f.existsAsFile())return; for(auto& r:project.samples) if(r.path==selectedSample.getFullPathName()){r.path=f.getFullPathName();r.fileSize=f.getSize();r.format=f.getFileExtension().trimCharactersAtStart(".").toUpperCase();} selectedSample=f; sampleFiles.clear();for(const auto& r:project.samples)sampleFiles.add(juce::File(r.path));sampleListModel->refresh(sampleFiles,searchField.getText());loadSample(f);relinkButton.setVisible(false);updateProjectFromSamples(); });
}

void MainComponent::showPage(const juce::String& page)
{
    currentPage = page;
    const bool processing = page == "Processing";
    const bool variations = page == "Variations";
    const bool library = page == "Sample Library";
    const bool home = page == "Home";
    const bool settings = page == "Settings";
    processingPanel.setVisible(processing);
    variationPanel.setVisible(variations);
    pluginPanel->setVisible(page == "VST3 Plugins");
    setDashboardControlsVisible(library);
    startupAudioToggle.setVisible(settings);
    homeImportButton.setVisible(home);homeNewButton.setVisible(home);homeOpenButton.setVisible(home);
    exportVariationsButton.setVisible(page=="Export");
    homeButton.setToggleState(home,juce::dontSendNotification);libraryButton.setToggleState(library,juce::dontSendNotification);processingButton.setToggleState(processing,juce::dontSendNotification);variationsButton.setToggleState(variations,juce::dontSendNotification);pluginsButton.setToggleState(page=="VST3 Plugins",juce::dontSendNotification);exportButton.setToggleState(page=="Export",juce::dontSendNotification);settingsButton.setToggleState(settings,juce::dontSendNotification);
    pageTitle.setText(page,juce::dontSendNotification);

    if (page == "Home")
        pageSubtitle.setText(
            "Design, transform and organize your own sample packs.",
            juce::dontSendNotification);

    else if (page == "Sample Library")
        pageSubtitle.setText(
            "Browse imported audio and choose samples to preview.",
            juce::dontSendNotification);

    else if (page == "Processing")
        pageSubtitle.setText(
            "Build a real offline effect chain and render a non-destructive copy.",
            juce::dontSendNotification);

    else if (page == "Variations")
        pageSubtitle.setText(
            "Compare and protect your favorite variations.",
            juce::dontSendNotification);

    else if (page == "VST3 Plugins")
        pageSubtitle.setText(
            "Plugin hosting is planned for a future release.",
            juce::dontSendNotification);

    else if(page=="Settings")pageSubtitle.setText("Application preferences and audio behavior.",juce::dontSendNotification);

    else
        pageSubtitle.setText(
            "Export rendered variations. Full pack packaging is being prepared.",
            juce::dontSendNotification);

    if(home)updateProjectHeading();
    repaint();
}

void MainComponent::paint(juce::Graphics& g)
{
    const auto bg=juce::Colour(0xff101114),sidebar=juce::Colour(0xff181a1f),panel=juce::Colour(0xff1d2027),control=juce::Colour(0xff262a33),border=juce::Colour(0xff333843),accent=juce::Colour(0xffea5910),primary=juce::Colour(0xfff3f3f5),secondary=juce::Colour(0xff9298a5);
    g.fillAll(bg);g.setColour(sidebar);g.fillRect(0,0,196,getHeight());g.setColour(panel);g.fillRect(196,0,getWidth()-196,70);g.setColour(border);g.drawHorizontalLine(69,0,(float)getWidth());g.drawVerticalLine(195,0,(float)getHeight()-26);g.drawHorizontalLine(getHeight()-27,196,(float)getWidth());
    if(logo.isValid())g.drawImageWithin(logo,17,12,43,43,juce::RectanglePlacement::centred);
    g.setColour(primary);g.setFont(juce::Font(15.0f,juce::Font::bold));g.drawText("OFFOR",68,14,116,20,juce::Justification::left);
    g.setColour(accent);g.setFont(juce::Font(13.0f,juce::Font::bold));g.drawText("SAMPLE FORGE",68,34,122,18,juce::Justification::left);
    g.setColour(secondary);g.setFont(11.0f);g.drawText("OFFOR AUDIO  •  v1.0.0",17,getHeight()-60,160,18,juce::Justification::left);
    auto card=[&](juce::Rectangle<int> r,const juce::String& title){g.setColour(panel);g.fillRoundedRectangle(r.toFloat(),10);g.setColour(border);g.drawRoundedRectangle(r.toFloat(),10,1);g.setColour(secondary);g.setFont(11.0f);g.drawText(title.toUpperCase(),r.getX()+16,r.getY()+12,r.getWidth()-32,18,juce::Justification::left);};
    const int x=contentBounds.getX(),w=contentBounds.getWidth(),areaY=contentBounds.getY()+70,areaH=contentBounds.getHeight()-78;
    if(currentPage=="Home")
    {
        const int gap=12,cw=(w-3*gap)/4;juce::String variations=juce::String(std::count_if(project.variationHistory.all().begin(),project.variationHistory.all().end(),[](const Variation&v){return !v.deleted;}));const juce::String values[]={juce::String(project.samples.size()),project.isOpen()?project.getName():"No project open",variations,project.isOpen()?"Ready":"Create or open a project"};const juce::String titles[]={"IMPORTED SAMPLES","CURRENT PACK","SAVED VARIATIONS","PROJECT STATUS"};
        for(int i=0;i<4;++i){juce::Rectangle<int> r{x+i*(cw+gap),areaY,cw,88};card(r,titles[i]);g.setColour(i==3&&!project.isOpen()?secondary:primary);g.setFont(juce::Font(i==1?16.0f:23.0f,juce::Font::bold));g.drawText(values[i],r.getX()+16,r.getY()+39,r.getWidth()-30,30,juce::Justification::left,true);if(i==1&&project.isOpen())g.setColour(accent);}
        const int lowerY=areaY+104,lowerH=juce::jmax(160,areaH-104),leftW=(w-14)*58/100;card({x,lowerY,leftW,lowerH},"Recently imported samples");card({x+leftW+14,lowerY,w-leftW-14,lowerH},"Quick actions & activity");
        const int rows=juce::jmin(5,sampleFiles.size());for(int i=0;i<rows;++i){const auto&f=sampleFiles.getReference(sampleFiles.size()-rows+i);const int yy=lowerY+42+i*34;g.setColour(i==rows-1?accent:control);g.fillRoundedRectangle((float)(x+16),(float)yy,28.0f,26.0f,6.0f);g.setColour(primary);g.setFont(12.0f);g.drawText("♪",x+16,yy+2,28,22,juce::Justification::centred);g.drawText(f.getFileName(),x+54,yy,leftW-76,26,juce::Justification::centredLeft,true);}
        if(rows==0){g.setColour(secondary);g.setFont(13.0f);g.drawText("Import a folder to start building your sample library.",x+18,lowerY+50,leftW-36,44,juce::Justification::left,true);}
        g.setColour(primary);g.setFont(13.0f);g.drawText(statusLabel.getText(),x+leftW+32,lowerY+45,w-leftW-54,48,juce::Justification::topLeft,true);g.setColour(secondary);g.setFont(12.0f);g.drawText("Project files save sample references, processing settings and variation history.",x+leftW+32,lowerY+98,w-leftW-54,50,juce::Justification::topLeft,true);
    }
    else if(currentPage=="Sample Library")
    {
        const int leftW=(w-16)*46/100;card({x,areaY,leftW,areaH},"Sample browser");card({x+leftW+16,areaY,w-leftW-16,areaH},"Selected sample");
        const int rx=x+leftW+34;g.setColour(secondary);g.setFont(11.0f);g.drawText("REAL-TIME PREVIEW WAVEFORM",rx,areaY+98,w-leftW-52,18,juce::Justification::left);
        auto wave=waveformBounds;g.setColour(control);g.fillRoundedRectangle(wave.toFloat(),8.0f);if(thumbnail.getNumChannels()>0&&thumbnail.getTotalLength()>0.0)thumbnail.drawChannels(g,wave,0,thumbnail.getTotalLength(),.9f);else{g.setColour(secondary);g.setFont(12.0f);g.drawText("Select a sample to view its waveform",wave,juce::Justification::centred);}
        if(!selectedSample.existsAsFile()){g.setColour(secondary);g.setFont(13.0f);g.drawText("Choose a row in the browser to inspect metadata and preview audio.",rx,areaY+64,w-leftW-52,30,juce::Justification::left,true);}
    }
    else if(currentPage=="Export")
    {
        card({x,areaY,w,areaH},"Variation export");g.setColour(primary);g.setFont(juce::Font(20.0f,juce::Font::bold));g.drawText("Export variation WAVs or create a ZIP pack",x+28,areaY+64,w-56,32,juce::Justification::left);g.setColour(secondary);g.setFont(13.0f);g.drawText("From Variations, choose Export Selected to export selected WAV files, all available variations, or create a ZIP from selected/all variations. Exports run in the background and use unique names to avoid overwriting existing files.",x+28,areaY+105,w-56,54,juce::Justification::topLeft,true);g.setColour(border);g.drawHorizontalLine(areaY+184,(float)(x+28),(float)(x+w-28));g.setColour(primary);g.setFont(14.0f);g.drawText("Source-sample pack packaging and project-wide export are not implemented.",x+28,areaY+207,w-56,32,juce::Justification::left,true);g.setColour(secondary);g.setFont(12.0f);g.drawText("WAV and ZIP output include generated variations only.",x+28,areaY+248,w-56,38,juce::Justification::left,true);
    }
    else if(currentPage=="Settings")
    {
        card({x,areaY,w,areaH},"Application settings");g.setColour(primary);g.setFont(15.0f);g.drawText("Audio",x+24,areaY+55,220,24,juce::Justification::left);g.setColour(secondary);g.setFont(12.0f);g.drawText("Control optional startup playback of the OFFOR audio logo.",x+24,areaY+86,500,24,juce::Justification::left);g.setColour(border);g.drawHorizontalLine(areaY+130,(float)(x+24),(float)(x+w-24));
    }
}

void MainComponent::resized()
{
    constexpr int sidebarWidth=196,footer=27;
    sidebarBounds={0,0,sidebarWidth,getHeight()};
    contentBounds={sidebarWidth+22,82,getWidth()-sidebarWidth-42,getHeight()-82-footer-18};
    const int titleX=contentBounds.getX(),titleY=contentBounds.getY();
    pageTitle.setBounds(titleX,titleY,contentBounds.getWidth(),30);pageSubtitle.setBounds(titleX,titleY+31,contentBounds.getWidth(),20);
    processingPanel.setBounds(titleX,titleY+58,contentBounds.getWidth(),contentBounds.getHeight()-58);processingPanel.setVisible(currentPage=="Processing");
    variationPanel.setBounds(titleX,titleY+58,contentBounds.getWidth(),contentBounds.getHeight()-58);variationPanel.setVisible(currentPage=="Variations");
    pluginPanel->setBounds(titleX,titleY+58,contentBounds.getWidth(),contentBounds.getHeight()-58);pluginPanel->setVisible(currentPage=="VST3 Plugins");
    juce::TextButton* nav[]={&homeButton,&libraryButton,&processingButton,&variationsButton,&pluginsButton,&exportButton,&settingsButton};
    int navY=102;for(auto*b:nav){b->setBounds(12,navY,172,40);navY+=47;}
    const double toolbarScale=juce::jlimit(.75,1.0,(getWidth()-1000.0)/240.0);int tx=210;const int headerY=20,headerH=32,gap=juce::roundToInt(5*toolbarScale);const int titleW=juce::roundToInt(160*toolbarScale);projectHeading.setBounds(tx,23,titleW,28);tx+=titleW+gap;
    auto put=[&tx,gap,headerY,headerH,toolbarScale](juce::Component&c,int base){const int width=juce::jmax(40,juce::roundToInt(base*toolbarScale));c.setBounds(tx,headerY,width,headerH);tx+=width+gap;};
    put(newProjectButton,60);put(openProjectButton,74);put(saveProjectButton,64);put(saveAsProjectButton,80);put(renameProjectButton,74);put(closeProjectButton,56);put(importButton,112);
    const int progressWidth=juce::roundToInt(68*toolbarScale);importProgress.setBounds(tx,25,progressWidth,20);const int cancelWidth=juce::roundToInt(40*toolbarScale);cancelImportButton.setBounds(tx+progressWidth+gap,headerY,cancelWidth,headerH);tx+=progressWidth+cancelWidth+2*gap;
    helpButton.setBounds(getWidth()-150,headerY,28,headerH);exportTopButton.setBounds(getWidth()-116,headerY,102,headerH);
    pageTitle.setText(currentPage,juce::dontSendNotification);pageTitle.setFont(juce::Font(25.0f,juce::Font::bold));pageSubtitle.setFont(juce::Font(12.0f));
    const int areaY=titleY+70,areaH=contentBounds.getHeight()-76,x=contentBounds.getX(),w=contentBounds.getWidth();
    const int homeLeft=(w-14)*58/100,homeRightX=x+homeLeft+30,homeActionW=juce::jmax(78,(w-homeLeft-56)/3);homeImportButton.setBounds(homeRightX,areaY+252,homeActionW,32);homeNewButton.setBounds(homeRightX+homeActionW+8,areaY+252,homeActionW,32);homeOpenButton.setBounds(homeRightX+2*(homeActionW+8),areaY+252,homeActionW,32);
    const int leftW=(w-16)*46/100;
    searchField.setBounds(x+14,areaY+48,leftW-150,30);categoryFilterBox.setBounds(x+leftW-128,areaY+48,114,30);sampleList.setBounds(x+14,areaY+88,leftW-28,juce::jmax(100,areaH-104));
    const int rx=x+leftW+32;selectedSampleLabel.setBounds(rx,areaY+48,w-leftW-188,28);categoryBox.setBounds(x+w-164,areaY+48,148,30);metadataLabel.setBounds(rx,areaY+78,w-leftW-48,42);metadataLabel.setFont(juce::Font(11.0f));waveformBounds={rx,areaY+124,w-leftW-48,juce::jmax(90,areaH-205)};playButton.setBounds(rx,areaY+areaH-64,118,34);stopButton.setBounds(rx+126,areaY+areaH-64,70,34);relinkButton.setBounds(rx+204,areaY+areaH-64,122,34);
    startupAudioToggle.setBounds(x+24,areaY+150,300,30);
    exportVariationsButton.setBounds(x+28,areaY+302,190,36);
    sampleCountLabel.setBounds(18,getHeight()-23,170,18);statusLabel.setBounds(210,getHeight()-23,juce::jmax(120,getWidth()-410),18);audioStatusLabel.setBounds(getWidth()-194,getHeight()-23,178,18);
    homeImportButton.setVisible(currentPage=="Home");homeNewButton.setVisible(currentPage=="Home");homeOpenButton.setVisible(currentPage=="Home");startupAudioToggle.setVisible(currentPage=="Settings");
}
