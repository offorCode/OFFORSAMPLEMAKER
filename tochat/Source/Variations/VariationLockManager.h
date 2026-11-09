#pragma once
#include "VariationHistory.h"
class VariationLockManager
{
public:
    static bool setLocked(VariationHistory&,const juce::String&,bool);
    static juce::Array<juce::String> unlockedIds(const VariationHistory&,const juce::String& sampleId);
};
