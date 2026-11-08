
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

    void refresh(const juce::Array<juce::File>& files, const juce::String& query)
    {
        visibleFiles.clear();
        for (const auto& file : files)
            if (query.isEmpty() || file.getFileName().containsIgnoreCase(query))
                visibleFiles.add(file);
        if (owner.selectedSample.existsAsFile())
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
    formatManager.registerBasicFormats();

    logo = juce::ImageFileFormat::loadFrom(
        getResourcesDirectory().getChildFile("OfforSForge.png"));

    setSize(1440, 900);
    setAudioChannels(0, 2);

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
        if (sampleListModel != nullptr) sampleListModel->refresh(sampleFiles, searchField.getText());
    };
    addAndMakeVisible(searchField);

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

    createButton(importButton, "Import Folder", accentColour,
                 [this] { chooseSampleFolder(); });

    createButton(exportTopButton, "Export Pack", accentColour,
                 [this] { showPage("Export"); });

    createButton(playButton, "Play Preview", lighterPanel,
                 [this]
    {
        if (currentReaderSource != nullptr)
        {
            transport.start();
            isPlaying = true;
            playButton.setButtonText("Playing...");
        }
        else
        {
            statusLabel.setText(
                "Import a sample folder first.",
                juce::dontSendNotification);
        }
    });

    createButton(stopButton, "Stop", lighterPanel,
                 [this]
    {
        transport.stop();
        transport.setPosition(0.0);
        isPlaying = false;
        playButton.setButtonText("Play Preview");
    });

    auto addEffect = [this](juce::TextButton& button,
                            const juce::String& name)
    {
        createButton(button, name + "  OFF", lighterPanel,
                     [this, &button, name]
        {
            const bool enabled = button.getToggleState();

            button.setToggleState(
                !enabled, juce::dontSendNotification);

            button.setButtonText(
                name + (enabled ? "  OFF" : "  ON"));

            statusLabel.setText(
                name + (enabled
                    ? " disabled in interface."
                    : " enabled in interface preview."),
                juce::dontSendNotification);
        });
    };

    addEffect(eqButton, "EQ");
    addEffect(compressorButton, "Compressor");
    addEffect(distortionButton, "Distortion");
    addEffect(reverbButton, "Reverb");

    for (int i = 0; i < 5; ++i)
    {
        const auto name = "Variation " + juce::String(i + 1);

        createButton(
            variationButtons[i],
            name,
            lighterPanel,
            [this, name]
        {
            statusLabel.setText(
                name + " selected. Rendering comes next.",
                juce::dontSendNotification);
        });
    }

    createButton(lockButton, "Lock Selected", lighterPanel,
                 [this]
    {
        statusLabel.setText(
            "Variation locking will be connected to the variation manager.",
            juce::dontSendNotification);
    });

    createButton(regenerateButton, "Regenerate Variations",
                 accentColour, [this]
    {
        statusLabel.setText(
            "Variation rendering engine is not connected yet.",
            juce::dontSendNotification);
    });

    addAndMakeVisible(pageTitle);
    addAndMakeVisible(pageSubtitle);
    addAndMakeVisible(selectedSampleLabel);
    addAndMakeVisible(sampleCountLabel);
    addAndMakeVisible(statusLabel);

    pageTitle.setColour(juce::Label::textColourId, textColour);
    pageTitle.setFont(juce::Font(25.0f, juce::Font::bold));

    pageSubtitle.setColour(juce::Label::textColourId, mutedColour);

    selectedSampleLabel.setColour(
        juce::Label::textColourId, textColour);

    sampleCountLabel.setColour(
        juce::Label::textColourId, mutedColour);

    statusLabel.setColour(
        juce::Label::textColourId, mutedColour);

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

    playStartupSound();
}

MainComponent::~MainComponent()
{
    transport.stop();
    transport.setSource(nullptr);
    shutdownAudio();
}

void MainComponent::createButton(
    juce::TextButton& button,
    const juce::String& text,
    juce::Colour colour,
    std::function<void()> action)
{
    addAndMakeVisible(button);

    button.setButtonText(text);

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
    transport.prepareToPlay(
        samplesPerBlockExpected, sampleRate);
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

            sampleFiles.clear();

            juce::Array<juce::File> found;

            folder.findChildFiles(
                found,
                juce::File::findFiles,
                true);

            for (const auto& file : found)
            {
                const auto ext =
                    file.getFileExtension().toLowerCase();

                if (ext == ".wav" ||
                    ext == ".aif" ||
                    ext == ".aiff" ||
                    ext == ".mp3" ||
                    ext == ".flac" ||
                    ext == ".ogg")
                {
                    sampleFiles.add(file);
                }
            }

            sampleCountLabel.setText(
                juce::String(sampleFiles.size()) +
                " samples imported",
                juce::dontSendNotification);

            if (sampleListModel != nullptr)
                sampleListModel->refresh(sampleFiles, searchField.getText());

            if (sampleFiles.isEmpty())
            {
                statusLabel.setText(
                    "No supported audio files found.",
                    juce::dontSendNotification);
            }
            else
            {
                statusLabel.setText(
                    "Imported from: " + folder.getFullPathName(),
                    juce::dontSendNotification);

                loadSample(sampleFiles.getFirst());
            }

            repaint();
        });
}

void MainComponent::loadSample(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
    {
        statusLabel.setText(
            "Could not open: " + file.getFileName(),
            juce::dontSendNotification);
        return;
    }

    transport.stop();
    transport.setSource(nullptr);

    currentReaderSource =
        std::make_unique<juce::AudioFormatReaderSource>(
            reader.release(), true);

    transport.setSource(
        currentReaderSource.get(),
        0,
        nullptr,
        currentReaderSource->getAudioFormatReader()->sampleRate);

    selectedSample = file;
    thumbnail.setSource(new juce::FileInputSource(file));

    selectedSampleLabel.setText(
        file.getFileName(),
        juce::dontSendNotification);

    isPlaying = false;
    playButton.setButtonText("Play Preview");

    statusLabel.setText(
        "Selected: " + file.getFileName(),
        juce::dontSendNotification);

    repaint();
}

void MainComponent::showPage(const juce::String& page)
{
    currentPage = page;

    pageTitle.setText(
        page,
        juce::dontSendNotification);

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
            "Choose the sound treatments you want to apply.",
            juce::dontSendNotification);

    else if (page == "Variations")
        pageSubtitle.setText(
            "Compare and protect your favorite variations.",
            juce::dontSendNotification);

    else if (page == "VST3 Plugins")
        pageSubtitle.setText(
            "Manage external VST3 instruments and effects.",
            juce::dontSendNotification);

    else
        pageSubtitle.setText(
            "Prepare your sample pack for delivery.",
            juce::dontSendNotification);

    repaint();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);

    g.setColour(sidebarColour);
    g.fillRect(0, 0, 238, getHeight());

    g.setColour(borderColour);
    g.drawHorizontalLine(82, 0.0f, (float)getWidth());

    if (logo.isValid())
    {
        g.drawImageWithin(
            logo,
            20, 18, 44, 44,
            juce::RectanglePlacement::centred);
    }

    g.setColour(textColour);
    g.setFont(juce::Font(17.0f, juce::Font::bold));
    g.drawText("OFFOR", 76, 18, 130, 22,
               juce::Justification::left);

    g.setColour(accentColour);
    g.drawText("SAMPLE FORGE", 76, 40, 145, 18,
               juce::Justification::left);

    const int x = contentBounds.getX();
    const int y = contentBounds.getY();
    const int w = contentBounds.getWidth();

    auto drawPanel = [&g](
        juce::Rectangle<int> bounds,
        const juce::String& title)
    {
        g.setColour(panelColour);
        g.fillRoundedRectangle(bounds.toFloat(), 12.0f);

        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.toFloat(), 12.0f, 1.0f);

        g.setColour(textColour);
        g.setFont(juce::Font(14.0f, juce::Font::bold));

        g.drawText(
            title,
            bounds.getX() + 16,
            bounds.getY() + 12,
            bounds.getWidth() - 32,
            22,
            juce::Justification::left);
    };

    auto overview = juce::Rectangle<int>(
        x, y + 84, w, 108);

    auto preview = juce::Rectangle<int>(
        x, y + 208, (w * 62) / 100, 278);

    auto effects = juce::Rectangle<int>(
        preview.getRight() + 16,
        y + 208,
        w - preview.getWidth() - 16,
        278);

    auto variations = juce::Rectangle<int>(
        x, y + 502, w, 190);

    drawPanel(overview, "PROJECT OVERVIEW");
    drawPanel(preview, "SAMPLE PREVIEW");
    drawPanel(effects, "SOUND DESIGN");
    drawPanel(variations, "VARIATION GENERATOR");

    const int cardY = overview.getY() + 43;
    const int cardW = (overview.getWidth() - 60) / 3;

    for (int i = 0; i < 3; ++i)
    {
        juce::Rectangle<float> card(
            (float)(overview.getX() + 16 + i * (cardW + 12)),
            (float)cardY,
            (float)cardW,
            48.0f);

        g.setColour(lighterPanel);
        g.fillRoundedRectangle(card, 8.0f);
    }

    g.setColour(mutedColour);
    g.setFont(12.0f);

    g.drawText("IMPORTED SAMPLES",
        overview.getX() + 28, cardY + 5,
        cardW - 20, 15, juce::Justification::left);

    g.drawText("CURRENT PACK",
        overview.getX() + cardW + 40, cardY + 5,
        cardW - 20, 15, juce::Justification::left);

    g.drawText("VARIATIONS / SAMPLE",
        overview.getX() + 2 * (cardW + 12) + 28,
        cardY + 5, cardW - 20, 15,
        juce::Justification::left);

    g.setColour(textColour);
    g.setFont(juce::Font(17.0f, juce::Font::bold));

    g.drawText(
        juce::String(sampleFiles.size()),
        overview.getX() + 28, cardY + 21,
        cardW - 20, 22,
        juce::Justification::left);

    g.drawText(
        "Untitled Sample Pack",
        overview.getX() + cardW + 40, cardY + 21,
        cardW - 20, 22,
        juce::Justification::left);

    g.drawText(
        "5",
        overview.getX() + 2 * (cardW + 12) + 28,
        cardY + 21, cardW - 20, 22,
        juce::Justification::left);

    auto waveArea = waveformBounds;
    waveArea.removeFromBottom(52);

    g.setColour(lighterPanel);
    g.fillRoundedRectangle(waveArea.toFloat(), 8.0f);

    if (thumbnail.getNumChannels() > 0 && thumbnail.getTotalLength() > 0.0)
        thumbnail.drawChannels(g, waveArea, 0.0, thumbnail.getTotalLength(), 1.0f);
    else
    {
        g.setColour(mutedColour);
        g.setFont(12.0f);
        g.drawText("Select an audio file to view its waveform", waveArea,
                   juce::Justification::centred);
    }
}

void MainComponent::resized()
{
    const int sidebarWidth = 238;
    const int headerHeight = 82;

    sidebarBounds = { 0, 0, sidebarWidth, getHeight() };

    contentBounds = {
        sidebarWidth + 28,
        headerHeight + 24,
        getWidth() - sidebarWidth - 56,
        getHeight() - headerHeight - 48
    };

    int navY = 112;

    juce::TextButton* navButtons[] = {
        &homeButton,
        &libraryButton,
        &processingButton,
        &variationsButton,
        &pluginsButton,
        &exportButton
    };

    for (auto* button : navButtons)
    {
        button->setBounds(18, navY, sidebarWidth - 36, 44);
        navY += 52;
    }

    importButton.setBounds(sidebarWidth + 28, 22, 140, 38);
    exportTopButton.setBounds(getWidth() - 174, 22, 146, 38);

    pageTitle.setBounds(
        contentBounds.getX(),
        contentBounds.getY(),
        500, 34);

    pageSubtitle.setBounds(
        contentBounds.getX(),
        contentBounds.getY() + 36,
        650, 24);

    const int panelY = contentBounds.getY() + 208;
    const int panelW = (contentBounds.getWidth() * 62) / 100;
    const int panelX = contentBounds.getX();

    selectedSampleLabel.setBounds(
        panelX + 18, panelY + 43, panelW - 36, 24);

    searchField.setBounds(panelX + 18, panelY + 68, panelW - 36, 28);
    sampleList.setBounds(panelX + 18, panelY + 100, panelW - 36, 86);
    waveformBounds = { panelX + 18, panelY + 190, panelW - 36, 78 };

    playButton.setBounds(
        panelX + 18, panelY + 224, 128, 34);

    stopButton.setBounds(
        panelX + 154, panelY + 224, 76, 34);

    const int fxX = panelX + panelW + 34;
    const int fxW = contentBounds.getWidth() - panelW - 34;

    eqButton.setBounds(fxX + 16, panelY + 54, fxW - 32, 34);
    compressorButton.setBounds(fxX + 16, panelY + 96, fxW - 32, 34);
    distortionButton.setBounds(fxX + 16, panelY + 138, fxW - 32, 34);
    reverbButton.setBounds(fxX + 16, panelY + 180, fxW - 32, 34);

    const int varY = contentBounds.getY() + 502;
    const int varX = contentBounds.getX();
    const int varW = contentBounds.getWidth();
    const int slotW = (varW - 52) / 5;

    for (int i = 0; i < 5; ++i)
    {
        variationButtons[i].setBounds(
            varX + 16 + i * (slotW + 4),
            varY + 58,
            slotW,
            54);
    }

    lockButton.setBounds(
        varX + 16, varY + 126, 126, 34);

    regenerateButton.setBounds(
        varX + varW - 206, varY + 126, 190, 34);

    sampleCountLabel.setBounds(
        sidebarWidth + 28, getHeight() - 46, 190, 18);

    statusLabel.setBounds(
        sidebarWidth + 240, getHeight() - 46,
        getWidth() - sidebarWidth - 260, 18);
}
