#pragma once
#include "../Processing/EffectSettings.h"
#include "VariationSettings.h"

struct Variation
{
    juce::String id, originalSampleId, name, createdAt, outputPath;
    int number = 0;
    juce::uint64 seed = 0;
    bool locked = false, favorite = false, deleted = false;
    double durationSeconds = 0.0, sampleRate = 0.0;
    int channels = 0, bitDepth = 0;
    double rms = 0.0, peak = 0.0, spectralCentroid = 0.0, crestFactor = 0.0;
    std::vector<EffectSettings> effects;
    juce::var toVar() const;
    static bool fromVar(const juce::var&, Variation&);
};
