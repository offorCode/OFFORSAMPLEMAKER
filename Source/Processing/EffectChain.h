#pragma once
#include "BuiltInEffects.h"

class EffectChain
{
public:
    void setEffects(const std::vector<EffectSettings>&);
    const std::vector<EffectSettings> getSettings() const;
    void add(EffectSettings);
    bool remove(size_t index);
    bool move(size_t from,size_t to);
    void clear();
    void prepare(double sampleRate,int maxBlockSize,int channels);
    void process(juce::AudioBuffer<float>&);
    void reset();
    juce::var toVar() const;
    bool fromVar(const juce::var&);
    size_t size() const noexcept { return effects.size(); }
private:
    std::vector<std::unique_ptr<AudioEffect>> effects;
};
