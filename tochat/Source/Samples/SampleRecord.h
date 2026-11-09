#pragma once
#include <JuceHeader.h>

struct AudioFeatures
{
    double rms = 0.0, peak = 0.0, crestFactor = 0.0, spectralCentroid = 0.0;
    double spectralRolloff = 0.0, zeroCrossingRate = 0.0, durationSeconds = 0.0;
    double transientStrength = 0.0;
    juce::String classificationStatus = "Uncertain";
    juce::String predictedCategory = "Other";
    double confidence = 0.0;
};

struct SampleRecord
{
    juce::String id;
    juce::String path;
    juce::String category = "Uncategorized";
    bool manuallyCategorized = false;
    juce::int64 fileSize = 0;
    double durationSeconds = 0.0;
    double sampleRate = 0.0;
    int bitDepth = 0;
    int channels = 0;
    juce::String format;
    juce::String importDate;
    AudioFeatures features;
    juce::String filename() const { return juce::File(path).getFileName(); }
    juce::var toVar() const;
    static bool fromVar(const juce::var&, SampleRecord&);
};
