#pragma once
#include "EffectChain.h"

class EffectProcessor
{
public:
    static void processBuffer(juce::AudioBuffer<float>&,double sampleRate,const std::vector<EffectSettings>&,int maximumBlockSize=4096);
};

class AudioOperations
{
public:
    static void normalize(juce::AudioBuffer<float>&,float targetDb=-1.0f,float ceilingDb=0.0f);
    static void reverse(juce::AudioBuffer<float>&);
    static void fadeIn(juce::AudioBuffer<float>&,double sampleRate,double milliseconds);
    static void fadeOut(juce::AudioBuffer<float>&,double sampleRate,double milliseconds);
    static bool trim(juce::AudioBuffer<float>&,float thresholdDb=-60.0f);
};

struct RenderOperations
{
    bool normalize=false,reverse=false,trimSilence=false;
    double fadeInMs=0.0,fadeOutMs=0.0;
    float normalizeTargetDb=-1.0f,normalizeCeilingDb=0.0f,trimThresholdDb=-60.0f;
    bool any()const{return normalize||reverse||trimSilence||fadeInMs>0.0||fadeOutMs>0.0;}
    juce::var toVar()const;
    static RenderOperations fromVar(const juce::var&);
};
