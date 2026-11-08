
#pragma once

#include <JuceHeader.h>
#include "Project/ProjectManager.h"
#include "Samples/SampleImporter.h"
#include "Processing/ProcessingPanel.h"

class MainComponent final : public juce::AudioAppComponent
{
    class SampleListModel;
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void createButton(juce::TextButton& button,
                      const juce::String& text,
                      juce::Colour colour,
                      std::function<void()> action);

    void showPage(const juce::String& page);
    void chooseSampleFolder();
    void loadSample(const juce::File& file);
    juce::File getResourcesDirectory() const;
    void playStartupSound();
    void createProject();
    void openProject();
    void saveProject(bool saveAs);
    void closeProject();
    void renameProject();
    bool updateProjectFromSamples();
    void updateSampleDetails();
    void relinkMissingSample();
    bool setPreviewFile(const juce::File& file);
    void setDashboardControlsVisible(bool visible);

    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transport;
    juce::ThreadPool importPool { 1 };
    ProjectManager project;
    juce::AudioThumbnailCache thumbnailCache { 16 };
    juce::AudioThumbnail thumbnail { 512, formatManager, thumbnailCache };
    ProcessingPanel processingPanel;

    std::unique_ptr<juce::AudioFormatReaderSource> currentReaderSource;
    std::unique_ptr<juce::FileChooser> fileChooser;

    juce::Image logo;

    std::unique_ptr<SampleListModel> sampleListModel;
    juce::ListBox sampleList { "Sample browser" };
    juce::TextEditor searchField;

    juce::Array<juce::File> sampleFiles;
    juce::File selectedSample;

    juce::String currentPage = "Home";

    juce::Label pageTitle;
    juce::Label pageSubtitle;
    juce::Label selectedSampleLabel;
    juce::Label sampleCountLabel;
    juce::Label statusLabel;

    juce::TextButton homeButton;
    juce::TextButton libraryButton;
    juce::TextButton processingButton;
    juce::TextButton variationsButton;
    juce::TextButton pluginsButton;
    juce::TextButton exportButton;

    juce::TextButton importButton;
    juce::TextButton exportTopButton;
    juce::TextButton newProjectButton, openProjectButton, saveProjectButton, saveAsProjectButton, closeProjectButton, renameProjectButton;
    juce::TextButton cancelImportButton, relinkButton;
    juce::ComboBox categoryBox;
    juce::Label metadataLabel;
    double importProgressValue = 0.0;
    juce::ProgressBar importProgress { importProgressValue };
    juce::uint64 importGeneration = 0;

    juce::TextButton playButton;
    juce::TextButton stopButton;

    juce::TextButton eqButton;
    juce::TextButton compressorButton;
    juce::TextButton distortionButton;
    juce::TextButton reverbButton;

    juce::TextButton variationButtons[5];
    juce::TextButton lockButton;
    juce::TextButton regenerateButton;

    juce::Rectangle<int> sidebarBounds;
    juce::Rectangle<int> contentBounds;
    juce::Rectangle<int> waveformBounds;

    bool isPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
