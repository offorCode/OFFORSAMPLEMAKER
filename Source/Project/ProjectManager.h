#pragma once
#include <JuceHeader.h>
#include "../Samples/SampleRecord.h"
#include "../Processing/ProcessingHistory.h"

class ProjectManager
{
public:
    bool create(const juce::File& file, const juce::String& name);
    bool open(const juce::File& file, juce::String& error);
    bool save(juce::String& error);
    bool saveAs(const juce::File& file, juce::String& error);
    bool rename(const juce::String& name);
    void close();
    bool isOpen() const { return openState; }
    juce::File getFile() const { return projectFile; }
    juce::String getName() const { return name; }
    juce::Array<SampleRecord> samples;
    juce::String selectedSampleId;
    juce::DynamicObject::Ptr exportSettings { new juce::DynamicObject() };
    std::vector<EffectSettings> processingChainSettings;
    RenderOperations processingOperations;
    ProcessingHistory processingHistory;
    juce::File getAssetsDirectory() const;
private:
    juce::File projectFile;
    juce::String name = "Untitled Sample Pack";
    bool openState = false;
};
