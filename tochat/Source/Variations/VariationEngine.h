#pragma once
#include "Variation.h"

class VariationEngine
{
public:
    static std::vector<EffectSettings> createChain(const VariationSettings&, const juce::String& category, juce::uint64 seed, const std::vector<EffectSettings>& base = {});
    static double featureDistance(double rmsA,double centroidA,double crestA,double durationA,double rmsB,double centroidB,double crestB,double durationB);
};
