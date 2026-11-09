#pragma once
#include <JuceHeader.h>

struct PluginScanResult
{
    juce::Array<juce::PluginDescription> plugins;
    juce::String xml;
    juce::String error;
    int unsupportedVst2Candidates = 0;
    bool cancelled = false;
};

class PluginScanJob final : public juce::ThreadPoolJob
{
public:
    using Completion = std::function<void(PluginScanResult)>;
    using Progress = std::function<void(double, juce::String)>;
    PluginScanJob(juce::StringArray directories, juce::String previousXml,
                  juce::File deadMansPedal, bool forceRescan, Completion, Progress,
                  std::shared_ptr<std::atomic<bool>> cancellation);
    JobStatus runJob() override;
private:
    juce::StringArray directories;
    juce::String previousXml;
    juce::File deadMansPedal;
    bool forceRescan = false;
    Completion completion;
    Progress progress;
    std::shared_ptr<std::atomic<bool>> cancellation;
    bool cancelled() const;
    void report(double, const juce::String&);
};
