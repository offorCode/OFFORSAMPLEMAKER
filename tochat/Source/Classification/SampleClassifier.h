#pragma once
#include "../Samples/SampleRecord.h"

struct ClassificationResult
{
    juce::String category = "Other";
    juce::String status = "Uncertain";
    double confidence = 0.0;
};

class ISampleClassifier
{
public:
    virtual ~ISampleClassifier() = default;
    virtual ClassificationResult classify(const AudioFeatures&) const = 0;
};

// A deliberately low-confidence baseline. Replace or extend through this interface
// when a local model becomes available; this is not marketed as an AI classifier.
class ProvisionalHeuristicClassifier final : public ISampleClassifier
{
public:
    ClassificationResult classify(const AudioFeatures&) const override;
};
