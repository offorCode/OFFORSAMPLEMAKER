#pragma once
#include <JuceHeader.h>
#include "EffectSettings.h"
#include "EffectProcessor.h"

class AudioRenderJob final:public juce::ThreadPoolJob
{
public:
    using Completion=std::function<void(juce::File,const juce::String&)>;
    using Progress=std::function<void(double)>;
    AudioRenderJob(juce::File source,juce::File destination,juce::String sampleId,std::vector<EffectSettings>,Completion,Progress,RenderOperations={});
    JobStatus runJob()override;
private:
    juce::File sourceFile,destinationFile;juce::String sourceId;std::vector<EffectSettings>effects;Completion completion;Progress progress;
    RenderOperations operations;
    void finish(juce::String error);
};
