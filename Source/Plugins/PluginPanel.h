#pragma once
#include <JuceHeader.h>
#include "PluginScanJob.h"

class PluginPanel final : public juce::Component, private juce::ListBoxModel
{
public:
    using CatalogChanged = std::function<void(const juce::String&)>;
    PluginPanel(juce::PropertiesFile&, CatalogChanged);
    ~PluginPanel() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void scan(bool forceRescan = true);
    void setCatalog(const juce::String&);
private:
    int getNumRows() override;
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override;
    void refreshRows();
    void addCustomDirectory();
    void cancelScan();
    juce::PropertiesFile& preferences;
    CatalogChanged onCatalogChanged;
    juce::Array<juce::PluginDescription> catalog, visible;
    juce::String catalogXml, statusText;
    juce::StringArray customDirectories;
    juce::ThreadPool scanPool { 1 };
    std::shared_ptr<std::atomic<bool>> cancelToken;
    bool scanning = false;
    double progressValue = 0.0;
    juce::TextEditor search;
    juce::ComboBox formatFilter, manufacturerFilter;
    juce::TextButton scanButton, addDirectoryButton, cancelButton;
    juce::ListBox results { "Installed VST3 plugins", this };
    juce::ProgressBar progress { progressValue };
    juce::Label status;
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginPanel)
};
