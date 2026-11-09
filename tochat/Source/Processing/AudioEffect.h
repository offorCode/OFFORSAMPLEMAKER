#pragma once
#include <JuceHeader.h>
#include "EffectSettings.h"

class AudioEffect
{
public:
    explicit AudioEffect(EffectSettings settings) : state(std::move(settings)) {}
    virtual ~AudioEffect() = default;
    virtual void prepare(double sampleRate, int maximumBlockSize, int channels) = 0;
    virtual void reset() = 0;
    virtual void processWet(juce::AudioBuffer<float>&) = 0;
    void process(juce::AudioBuffer<float>& buffer)
    {
        if (!state.enabled || buffer.getNumSamples()==0 || state.wet<=0.0f) return;
        juce::AudioBuffer<float> dry(buffer.getNumChannels(),buffer.getNumSamples());
        for(int c=0;c<buffer.getNumChannels();++c)dry.copyFrom(c,0,buffer,c,0,buffer.getNumSamples());
        processWet(buffer);
        if(state.wet<1.0f)for(int c=0;c<buffer.getNumChannels();++c)for(int i=0;i<buffer.getNumSamples();++i)
            buffer.setSample(c,i,dry.getSample(c,i)*(1.0f-state.wet)+buffer.getSample(c,i)*state.wet);
    }
    const EffectSettings& settings() const noexcept { return state; }
    EffectSettings& settings() noexcept { return state; }
protected:
    EffectSettings state;
    double rate = 44100.0;
};
