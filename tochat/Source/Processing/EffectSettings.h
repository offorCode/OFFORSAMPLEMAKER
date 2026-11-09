#pragma once
#include <JuceHeader.h>

enum class EffectType { parametricEq, compressor, saturation, reverb, delay, filter, gain, limiter };

struct EffectSettings
{
    juce::String id = juce::Uuid().toString();
    EffectType type = EffectType::gain;
    bool enabled = true;
    float wet = 1.0f;
    float frequency = 1000.0f;
    float gainDb = 0.0f;
    float q = 0.707f;
    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float driveDb = 12.0f;
    float toneHz = 12000.0f;
    float roomSize = 0.5f;
    float damping = 0.5f;
    float width = 1.0f;
    float delayMs = 250.0f;
    float feedback = 0.35f;
    float cutoffHz = 1000.0f;
    bool highPass = false;
    float levelDb = 0.0f;
    float ceilingDb = -1.0f;
    float limiterReleaseMs = 80.0f;

    static EffectSettings defaults(EffectType);
    juce::var toVar() const;
    static bool fromVar(const juce::var&, EffectSettings&);
    juce::String typeName() const;
    static bool typeFromName(const juce::String&, EffectType&);
};
