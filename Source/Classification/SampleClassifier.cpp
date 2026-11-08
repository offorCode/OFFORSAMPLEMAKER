#include "SampleClassifier.h"

ClassificationResult ProvisionalHeuristicClassifier::classify(const AudioFeatures& f) const
{
    if (f.durationSeconds > 0.0 && f.durationSeconds < 0.9
        && f.crestFactor > 3.2 && f.zeroCrossingRate < 0.12
        && f.peak > 0.01 && f.spectralCentroid > 0.0)
        return { "Percussion", "Provisional", 0.40 };
    return {};
}
