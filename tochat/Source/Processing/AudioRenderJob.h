#pragma once
#include <JuceHeader.h>
#include "EffectSettings.h"
#include "EffectProcessor.h"
#include <atomic>

class AudioRenderJob final:public juce::ThreadPoolJob
{
public:
    using Completion=std::function<void(juce::File,const juce::String&)>;
    using Progress=std::function<void(double)>;
    AudioRenderJob(juce::File source,juce::File destination,juce::String sampleId,std::vector<EffectSettings>,Completion,Progress,RenderOperations={},std::shared_ptr<std::atomic<bool>> cancellation={});
    JobStatus runJob()override;
private:
    juce::File sourceFile,destinationFile;juce::String sourceId;std::vector<EffectSettings>effects;Completion completion;Progress progress;
    RenderOperations operations;
    std::shared_ptr<std::atomic<bool>> cancellation;
    bool isCancelled() const { return shouldExit() || (cancellation && cancellation->load()); }
    void finish(juce::String error);
};
