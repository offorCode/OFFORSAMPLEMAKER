#include "SampleRecord.h"

juce::var SampleRecord::toVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty("id", id); o->setProperty("path", path); o->setProperty("category", category);
    o->setProperty("manualCategory", manuallyCategorized); o->setProperty("fileSize", (juce::int64)fileSize);
    o->setProperty("duration", durationSeconds); o->setProperty("sampleRate", sampleRate);
    o->setProperty("bitDepth", bitDepth); o->setProperty("channels", channels);
    o->setProperty("format", format); o->setProperty("importDate", importDate);
    auto* f = new juce::DynamicObject();
    f->setProperty("rms", features.rms); f->setProperty("peak", features.peak);
    f->setProperty("crestFactor", features.crestFactor); f->setProperty("spectralCentroid", features.spectralCentroid);
    f->setProperty("spectralRolloff", features.spectralRolloff); f->setProperty("zeroCrossingRate", features.zeroCrossingRate);
    f->setProperty("transientStrength", features.transientStrength); f->setProperty("duration", features.durationSeconds);
    f->setProperty("status", features.classificationStatus); f->setProperty("category", features.predictedCategory);
    f->setProperty("confidence", features.confidence); o->setProperty("features", juce::var(f));
    return juce::var(o);
}

bool SampleRecord::fromVar(const juce::var& v, SampleRecord& r)
{
    auto* o = v.getDynamicObject();
    if (o == nullptr || !o->hasProperty("path") || !o->hasProperty("id")) return false;
    r.id = o->getProperty("id").toString(); r.path = o->getProperty("path").toString();
    if (r.id.isEmpty() || r.path.isEmpty()) return false;
    r.category = (o->hasProperty("category") ? o->getProperty("category") : juce::var("Uncategorized")).toString();
    r.manuallyCategorized = (bool)o->getProperty("manualCategory"); r.fileSize = (juce::int64)o->getProperty("fileSize");
    r.durationSeconds = (double)o->getProperty("duration"); r.sampleRate = (double)o->getProperty("sampleRate");
    r.bitDepth = (int)o->getProperty("bitDepth"); r.channels = (int)o->getProperty("channels");
    r.format = o->getProperty("format").toString(); r.importDate = o->getProperty("importDate").toString();
    if (auto* f = o->getProperty("features").getDynamicObject())
    {
        r.features.rms = f->getProperty("rms"); r.features.peak = f->getProperty("peak");
        r.features.crestFactor = f->getProperty("crestFactor"); r.features.spectralCentroid = f->getProperty("spectralCentroid");
        r.features.spectralRolloff = f->getProperty("spectralRolloff"); r.features.zeroCrossingRate = f->getProperty("zeroCrossingRate");
        r.features.transientStrength = f->getProperty("transientStrength"); r.features.durationSeconds = f->getProperty("duration");
        r.features.classificationStatus = f->getProperty("status").toString();
        r.features.predictedCategory = (f->hasProperty("category") ? f->getProperty("category") : juce::var("Other")).toString(); r.features.confidence = f->getProperty("confidence");
    }
    return true;
}
