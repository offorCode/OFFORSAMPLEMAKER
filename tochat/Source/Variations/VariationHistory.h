#pragma once
#include "Variation.h"

class VariationHistory
{
public:
    const juce::Array<Variation>& all()const{return records;}
    void add(const Variation& v){records.add(v);}
    void setAll(const juce::Array<Variation>& v){records=v;}
    bool remove(const juce::String& id);
    Variation* find(const juce::String& id);
    juce::var toVar()const;
    bool fromVar(const juce::var&);
private:juce::Array<Variation> records;
};
