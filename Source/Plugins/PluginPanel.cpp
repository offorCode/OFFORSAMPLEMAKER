#include "PluginPanel.h"
#include "../UI/OfforLookAndFeel.h"

namespace
{
const juce::Colour background{0xff101114}, panel{0xff1d2027}, control{0xff262a33};
const juce::Colour border{0xff333843}, accent{0xffea5910}, text{0xfff3f3f5}, muted{0xff9298a5};
}

PluginPanel::PluginPanel(juce::PropertiesFile& p, CatalogChanged changed)
    : preferences(p), onCatalogChanged(std::move(changed))
{
    search.setTextToShowWhenEmpty("Search installed plugins...", muted);
    search.setTooltip("Search discovered VST3 plugin names, manufacturers, and categories.");
    search.onTextChange = [this] { refreshRows(); };
    addAndMakeVisible(search);
    formatFilter.addItem("All formats", 1); formatFilter.addItem("VST3", 2); formatFilter.addItem("VST2 unavailable", 3);
    formatFilter.setSelectedId(1); formatFilter.setTooltip("Filter the discovery list by plugin format. VST2 is not hosted by this build.");formatFilter.onChange = [this] { refreshRows(); }; addAndMakeVisible(formatFilter);
    typeFilter.addItem("All types",1);typeFilter.addItem("Effects",2);typeFilter.addItem("Instruments",3);typeFilter.setSelectedId(1);typeFilter.setTooltip("Filter discovered descriptions by effect or instrument category.");typeFilter.onChange=[this]{refreshRows();};addAndMakeVisible(typeFilter);
    manufacturerFilter.addItem("All manufacturers", 1); manufacturerFilter.setSelectedId(1);
    manufacturerFilter.setTooltip("Show discovered plugins from one manufacturer.");manufacturerFilter.onChange = [this] { refreshRows(); }; addAndMakeVisible(manufacturerFilter);
    scanButton.setButtonText("Rescan VST3"); scanButton.onClick = [this] { scan(true); }; addAndMakeVisible(scanButton);
    addDirectoryButton.setButtonText("Add Plugin Folder"); addDirectoryButton.onClick = [this] { addCustomDirectory(); }; addAndMakeVisible(addDirectoryButton);
    cancelButton.setButtonText("Cancel Scan"); cancelButton.onClick = [this] { cancelScan(); }; addAndMakeVisible(cancelButton);
    scanButton.setTooltip("Rescan configured VST3 plugin folders in the background.");addDirectoryButton.setTooltip("Add a custom folder to VST3 discovery.");cancelButton.setTooltip("Request cancellation of the active plugin scan.");
    OfforLookAndFeel::setIcon(scanButton,"regenerate");OfforLookAndFeel::setIcon(addDirectoryButton,"open");OfforLookAndFeel::setIcon(cancelButton,"stop");
    addAndMakeVisible(results); addAndMakeVisible(progress); addAndMakeVisible(status);
    status.setColour(juce::Label::textColourId, muted);
    progress.setColour(juce::ProgressBar::foregroundColourId, accent);
    customDirectories.addLines(preferences.getValue("pluginDirectories"));
    catalogXml = preferences.getValue("pluginCatalog");
    setCatalog(catalogXml);
    scan(false);
}

PluginPanel::~PluginPanel()
{
    if (cancelToken) cancelToken->store(true);
    scanPool.removeAllJobs(true, 5000);
}

void PluginPanel::setCatalog(const juce::String& xmlText)
{
    catalogXml = xmlText;
    catalog.clear();
    if (auto xml = juce::XmlDocument::parse(catalogXml))
    {
        juce::KnownPluginList known; known.recreateFromXml(*xml); catalog = known.getTypes();
    }
    manufacturerFilter.clear(juce::dontSendNotification);
    manufacturerFilter.addItem("All manufacturers", 1);
    juce::StringArray manufacturers;
    for (const auto& p : catalog) if (p.manufacturerName.isNotEmpty()) manufacturers.addIfNotAlreadyThere(p.manufacturerName);
    manufacturers.sort(true);
    for (int i = 0; i < manufacturers.size(); ++i) manufacturerFilter.addItem(manufacturers[i], i + 2);
    manufacturerFilter.setSelectedId(1, juce::dontSendNotification);
    refreshRows();
}

void PluginPanel::scan(bool forceRescan)
{
    if (scanning) return;
    juce::StringArray directories;
    for (const auto& path : { juce::File("C:/Program Files/Common Files/VST3"),
                              juce::File("C:/Program Files (x86)/Common Files/VST3") })
        if (path.isDirectory()) directories.add(path.getFullPathName());
    for (const auto& p : customDirectories) if (juce::File(p).isDirectory()) directories.addIfNotAlreadyThere(p);
    if (directories.isEmpty())
    {
        statusText = "No VST3 plugin folders found. Use Add Plugin Folder to include a custom location.";
        status.setText(statusText, juce::dontSendNotification); repaint(); return;
    }
    cancelToken = std::make_shared<std::atomic<bool>>(false); scanning = true; progressValue = 0;
    scanButton.setEnabled(false); cancelButton.setEnabled(true); status.setText("Scanning VST3 folders in background...", juce::dontSendNotification);
    const auto pedal = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("OFFOR AUDIO/Offor Sample Forge/vst3-scan-pedal.txt");
    auto safe = juce::Component::SafePointer<PluginPanel>(this);
    auto* job = new PluginScanJob(directories, catalogXml, pedal, forceRescan,
        [safe](PluginScanResult result)
        {
            if (!safe) return;
            safe->scanning = false; safe->scanButton.setEnabled(true); safe->cancelButton.setEnabled(false);
            safe->progressValue = result.cancelled ? safe->progressValue : 1.0;
            if (!result.cancelled)
            {
                safe->setCatalog(result.xml);
                safe->preferences.setValue("pluginCatalog", result.xml); safe->preferences.saveIfNeeded();
            }
            safe->statusText = result.cancelled ? "Scan cancelled. Previous valid scan results are retained."
                : result.error.isNotEmpty() ? result.error
                : "Found " + juce::String(result.plugins.size()) + " valid VST3 plugin type(s). VST2 hosting is unavailable in this build.";
            safe->status.setText(safe->statusText, juce::dontSendNotification);
            if (safe->onCatalogChanged && !result.cancelled) safe->onCatalogChanged(result.xml);
            safe->repaint();
        },
        [safe](double value, juce::String current)
        { if (safe) { safe->progressValue = value; safe->status.setText("Scanning " + current, juce::dontSendNotification); safe->repaint(); } },
        cancelToken);
    scanPool.addJob(job, true);
}

void PluginPanel::cancelScan()
{
    if (cancelToken) cancelToken->store(true);
    cancelButton.setEnabled(false); status.setText("Cancellation requested; current plugin check will finish safely.", juce::dontSendNotification);
}

void PluginPanel::addCustomDirectory()
{
    chooser = std::make_unique<juce::FileChooser>("Add VST3 plugin directory", juce::File(), "*");
    chooser->launchAsync(juce::FileBrowserComponent::canSelectDirectories | juce::FileBrowserComponent::openMode,
        [this](const juce::FileChooser& c)
        {
            auto directory = c.getResult(); if (!directory.isDirectory()) return;
            customDirectories.addIfNotAlreadyThere(directory.getFullPathName());
            preferences.setValue("pluginDirectories", customDirectories.joinIntoString("\n")); preferences.saveIfNeeded();
            scan(true);
        });
}

void PluginPanel::refreshRows()
{
    visible.clear(); const auto query = search.getText();
    for (const auto& p : catalog)
    {
        if (query.isNotEmpty() && !p.name.containsIgnoreCase(query) && !p.manufacturerName.containsIgnoreCase(query)
            && !p.category.containsIgnoreCase(query)) continue;
        if (formatFilter.getSelectedId() == 2 && !p.pluginFormatName.containsIgnoreCase("VST3")) continue;
        if (formatFilter.getSelectedId() == 3) continue;
        const bool instrument=p.category.containsIgnoreCase("instrument")||p.category.containsIgnoreCase("synth");
        if(typeFilter.getSelectedId()==2&&instrument)continue;
        if(typeFilter.getSelectedId()==3&&!instrument)continue;
        if (manufacturerFilter.getSelectedId() > 1
            && manufacturerFilter.getText() != p.manufacturerName) continue;
        visible.add(p);
    }
    results.updateContent(); results.repaint();
}

int PluginPanel::getNumRows() { return visible.size(); }

juce::String PluginPanel::getTooltipForRow(int row)
{
    if(!juce::isPositiveAndBelow(row,visible.size()))return {};
    const auto&p=visible.getReference(row);const bool instrument=p.category.containsIgnoreCase("instrument")||p.category.containsIgnoreCase("synth");
    return p.name+" — "+p.manufacturerName+" | "+(instrument?"Instrument":"Effect")+" | "+p.pluginFormatName+" "+p.version+" | DISCOVERED (not loaded or validated for hosting)";
}

void PluginPanel::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (!juce::isPositiveAndBelow(row, visible.size())) return;
    const auto& p = visible.getReference(row); g.fillAll(selected ? juce::Colour(0xff392218) : panel);
    const bool instrument=p.category.containsIgnoreCase("instrument")||p.category.containsIgnoreCase("synth");
    g.setColour(text); g.setFont(13.0f); g.drawText(p.name, 12, 2, width * 32 / 100, height - 4, juce::Justification::centredLeft, true);
    g.setColour(muted); g.setFont(11.0f); g.drawText(p.manufacturerName, width * 33 / 100, 2, width * 19 / 100, height - 4, juce::Justification::centredLeft, true);
    g.drawText(instrument?"Instrument":"Effect", width * 53 / 100, 2, width * 13 / 100, height - 4, juce::Justification::centredLeft, true);
    g.drawText(p.category, width * 67 / 100, 2, width * 15 / 100, height - 4, juce::Justification::centredLeft, true);
    g.setColour(accent); g.drawText("VST3 · Discovered", width * 82 / 100, 2, width * 18 / 100 - 12, height - 4, juce::Justification::centredRight);
}

void PluginPanel::paint(juce::Graphics& g)
{
    g.fillAll(background); auto area = getLocalBounds().reduced(12); g.setColour(panel); g.fillRoundedRectangle(area.toFloat(), 10);
    g.setColour(border); g.drawRoundedRectangle(area.toFloat(), 10, 1);
    g.setColour(text); g.setFont(juce::Font(16.0f, juce::Font::bold)); g.drawText("VST3 Plugin Discovery", 24, 18, 360, 26, juce::Justification::left);
    g.setColour(muted); g.setFont(12.0f); g.drawText("DISCOVERED means a plugin description was found. This build does not validate, load, or process with plugins.", 24, 47, getWidth()-48, 20, juce::Justification::left);
    g.drawText("VST2 is unsupported here. VST3 hosting status: NOT IMPLEMENTED.", 24, getHeight()-62, getWidth()-48, 18, juce::Justification::left);
    if (visible.isEmpty() && !scanning) { g.setColour(muted); g.drawText("No valid VST3 plugins found yet.", 24, 104, getWidth()-48, 36, juce::Justification::centred); }
}

void PluginPanel::resized()
{
    const int x=24,y=78,w=getWidth()-48;const int searchW=juce::jmax(155,w-676);search.setBounds(x,y,searchW,30);
    int fx=x+searchW+8;formatFilter.setBounds(fx,y,130,30);fx+=138;typeFilter.setBounds(fx,y,120,30);fx+=128;manufacturerFilter.setBounds(fx,y,juce::jmax(120,juce::jmin(170,getWidth()-fx-24)),30);
    const int buttonsY=y+38;scanButton.setBounds(getWidth()-348,buttonsY,105,30);addDirectoryButton.setBounds(getWidth()-235,buttonsY,130,30);cancelButton.setBounds(getWidth()-96,buttonsY,72,30);
    results.setBounds(x,y+78,w,getHeight()-216); results.setRowHeight(36); progress.setBounds(x,getHeight()-124,w,20); status.setBounds(x,getHeight()-100,w,20);
}
