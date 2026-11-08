#pragma once
#include "SampleRecord.h"

class AudioAnalyzer
{
public:
    static AudioFeatures analyze(juce::AudioFormatManager&, const juce::File&);
};
