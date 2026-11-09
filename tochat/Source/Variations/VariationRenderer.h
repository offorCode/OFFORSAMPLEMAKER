#pragma once
#include "VariationEngine.h"
#include "VariationHistory.h"
#include "../Processing/AudioRenderJob.h"

class VariationRenderer final : public juce::ThreadPoolJob
{
public:
    using Completion=std::function<void(juce::Array<Variation>,const juce::String&)>;
    using Progress=std::function<void(double)>;
    VariationRenderer(juce::File source,juce::File assets,juce::String sampleId,juce::String category,VariationSettings,std::vector<EffectSettings> baseEffects,juce::Array<int> targetNumbers,Completion,Progress,std::shared_ptr<std::atomic<bool>> cancellation);
    JobStatus runJob() override;
private:
    juce::File sourceFile,assetsDirectory;juce::String sampleId,category;VariationSettings settings;std::vector<EffectSettings>baseEffects;juce::Array<int>numbers;Completion completion;Progress progress;std::shared_ptr<std::atomic<bool>> cancelled;
    bool isCancelled()const{return shouldExit()||(cancelled&&cancelled->load());}
    void finish(juce::Array<Variation>,juce::String);
};
