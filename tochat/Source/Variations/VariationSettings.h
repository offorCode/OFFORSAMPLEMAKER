#pragma once
#include <JuceHeader.h>

enum class VariationIntensity { subtle, moderate, creative, aggressive, custom };

struct VariationSettings
{
    int count = 5;
    VariationIntensity intensity = VariationIntensity::moderate;
    float strength = 0.55f;
    juce::uint64 seed = 0;
    bool allowEq = true, allowCompressor = true, allowSaturation = true;
    bool allowReverb = true, allowDelay = true, allowFilter = true, allowGain = true;
    float maxGainDb = 3.0f, maxEqDb = 5.0f, maxDriveDb = 8.0f;
    juce::var toVar() const;
    static VariationSettings fromVar(const juce::var&);
};
