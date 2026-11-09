#pragma once
#include <JuceHeader.h>

class VariationExporter
{
public:
    static bool copyWavs(const juce::Array<juce::File>&, const juce::File& directory,
                         int& exported, juce::String& error);
    static bool createZip(const juce::Array<juce::File>&, const juce::File& requestedTarget,
                          juce::File& createdTarget, juce::String& error);
};
