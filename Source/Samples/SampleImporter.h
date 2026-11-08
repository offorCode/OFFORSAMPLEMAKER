#pragma once
#include <JuceHeader.h>
#include "SampleRecord.h"

class SampleImporter final : public juce::ThreadPoolJob
{
public:
    using Completion = std::function<void(juce::Array<SampleRecord>, int, int)>;
    using Progress = std::function<void(double)>;
    SampleImporter(juce::File folder, juce::Array<SampleRecord> existing, Completion done, Progress reportProgress);
    JobStatus runJob() override;
private:
    juce::File root;
    juce::Array<SampleRecord> current;
    Completion completion;
    Progress progress;
};
