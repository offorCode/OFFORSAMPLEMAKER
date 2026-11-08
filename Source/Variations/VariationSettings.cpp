#include "VariationSettings.h"
juce::var VariationSettings::toVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty("count", count); o->setProperty("intensity", (int) intensity); o->setProperty("strength", strength); o->setProperty("seed", (juce::int64) seed);
    o->setProperty("allowEq", allowEq); o->setProperty("allowCompressor", allowCompressor); o->setProperty("allowSaturation", allowSaturation); o->setProperty("allowReverb", allowReverb); o->setProperty("allowDelay", allowDelay); o->setProperty("allowFilter", allowFilter); o->setProperty("allowGain", allowGain);
    o->setProperty("maxGainDb", maxGainDb); o->setProperty("maxEqDb", maxEqDb); o->setProperty("maxDriveDb", maxDriveDb); return juce::var(o);
}
VariationSettings VariationSettings::fromVar(const juce::var& v)
{
    VariationSettings s; if (auto* o = v.getDynamicObject())
    {
        s.count = (int)o->getProperty("count"); s.intensity = (VariationIntensity)juce::jlimit(0, 4, (int)o->getProperty("intensity")); s.strength = (float)o->getProperty("strength"); s.seed = (juce::uint64)(juce::int64)o->getProperty("seed");
#define VREAD(x) if(o->hasProperty(#x)) s.x=(bool)o->getProperty(#x)
        VREAD(allowEq); VREAD(allowCompressor); VREAD(allowSaturation); VREAD(allowReverb); VREAD(allowDelay); VREAD(allowFilter); VREAD(allowGain);
#undef VREAD
        if(o->hasProperty("maxGainDb"))s.maxGainDb=(float)o->getProperty("maxGainDb"); if(o->hasProperty("maxEqDb"))s.maxEqDb=(float)o->getProperty("maxEqDb"); if(o->hasProperty("maxDriveDb"))s.maxDriveDb=(float)o->getProperty("maxDriveDb");
    }
    s.count=juce::jlimit(1,32,s.count); s.strength=juce::jlimit(0.0f,1.0f,s.strength); s.maxGainDb=juce::jlimit(0.0f,12.0f,s.maxGainDb); s.maxEqDb=juce::jlimit(0.0f,18.0f,s.maxEqDb); s.maxDriveDb=juce::jlimit(0.0f,24.0f,s.maxDriveDb); return s;
}
