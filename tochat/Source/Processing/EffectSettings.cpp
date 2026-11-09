#include "EffectSettings.h"

EffectSettings EffectSettings::defaults(EffectType type)
{ EffectSettings s; s.id = juce::Uuid().toString(); s.type = type; return s; }

juce::String EffectSettings::typeName() const
{
    switch(type){case EffectType::parametricEq:return "Parametric EQ";case EffectType::compressor:return "Compressor";case EffectType::saturation:return "Saturation";case EffectType::reverb:return "Reverb";case EffectType::delay:return "Delay";case EffectType::filter:return "Filter";case EffectType::gain:return "Gain";case EffectType::limiter:return "Limiter";} return "Gain";
}

bool EffectSettings::typeFromName(const juce::String& name, EffectType& t)
{
    const juce::StringArray names { "Parametric EQ", "Compressor", "Saturation", "Reverb", "Delay", "Filter", "Gain", "Limiter" };
    for(int i=0;i<names.size();++i)if(name==names[i]){t=(EffectType)i;return true;}return false;
}

juce::var EffectSettings::toVar() const
{
    auto* o=new juce::DynamicObject();
    o->setProperty("id",id);o->setProperty("type",typeName());o->setProperty("enabled",enabled);o->setProperty("wet",wet);
    o->setProperty("frequency",frequency);o->setProperty("gainDb",gainDb);o->setProperty("q",q);o->setProperty("thresholdDb",thresholdDb);
    o->setProperty("ratio",ratio);o->setProperty("attackMs",attackMs);o->setProperty("releaseMs",releaseMs);o->setProperty("driveDb",driveDb);
    o->setProperty("toneHz",toneHz);o->setProperty("roomSize",roomSize);o->setProperty("damping",damping);o->setProperty("width",width);
    o->setProperty("delayMs",delayMs);o->setProperty("feedback",feedback);o->setProperty("cutoffHz",cutoffHz);o->setProperty("highPass",highPass);
    o->setProperty("levelDb",levelDb);o->setProperty("ceilingDb",ceilingDb);o->setProperty("limiterReleaseMs",limiterReleaseMs);return juce::var(o);
}

bool EffectSettings::fromVar(const juce::var& v, EffectSettings& s)
{
    auto* o=v.getDynamicObject();if(!o)return false;EffectType t;if(!typeFromName(o->getProperty("type").toString(),t))return false;
    s=defaults(t);if(o->hasProperty("id"))s.id=o->getProperty("id").toString();
#define READ_FIELD(field) if(o->hasProperty(#field))s.field=(decltype(s.field))o->getProperty(#field)
    READ_FIELD(enabled);READ_FIELD(wet);READ_FIELD(frequency);READ_FIELD(gainDb);READ_FIELD(q);READ_FIELD(thresholdDb);READ_FIELD(ratio);READ_FIELD(attackMs);READ_FIELD(releaseMs);READ_FIELD(driveDb);READ_FIELD(toneHz);READ_FIELD(roomSize);READ_FIELD(damping);READ_FIELD(width);READ_FIELD(delayMs);READ_FIELD(feedback);READ_FIELD(cutoffHz);READ_FIELD(highPass);READ_FIELD(levelDb);READ_FIELD(ceilingDb);READ_FIELD(limiterReleaseMs);
#undef READ_FIELD
    s.wet=juce::jlimit(0.0f,1.0f,s.wet);s.frequency=juce::jmax(20.0f,s.frequency);s.q=juce::jlimit(0.1f,18.0f,s.q);s.ratio=juce::jlimit(1.0f,20.0f,s.ratio);s.feedback=juce::jlimit(0.0f,0.95f,s.feedback);
    return true;
}
