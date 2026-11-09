#include "ProcessingHistory.h"

juce::var ProcessingRecord::toVar()const
{
    auto*o=new juce::DynamicObject();o->setProperty("id",id);o->setProperty("sampleId",sampleId);
    o->setProperty("originalPath",originalPath);o->setProperty("renderedPath",renderedPath);
    o->setProperty("timestamp",timestamp);o->setProperty("version",version);o->setProperty("locked",locked);
    juce::Array<juce::var>a;for(const auto&e:effects)a.add(e.toVar());o->setProperty("effects",juce::var(a));o->setProperty("operations",operations.toVar());return juce::var(o);
}

bool ProcessingRecord::fromVar(const juce::var&v,ProcessingRecord&r)
{
    auto*o=v.getDynamicObject();if(!o)return false;r.id=o->getProperty("id").toString();r.sampleId=o->getProperty("sampleId").toString();
    r.originalPath=o->getProperty("originalPath").toString();r.renderedPath=o->getProperty("renderedPath").toString();
    r.timestamp=o->getProperty("timestamp").toString();r.version=o->getProperty("version").toString();r.locked=(bool)o->getProperty("locked");
    if(auto*a=o->getProperty("effects").getArray())for(const auto&x:*a){EffectSettings e;if(!EffectSettings::fromVar(x,e))return false;r.effects.push_back(e);}
    if(o->hasProperty("operations"))r.operations=RenderOperations::fromVar(o->getProperty("operations"));
    return r.originalPath.isNotEmpty()&&r.renderedPath.isNotEmpty();
}

juce::var ProcessingHistory::toVar()const{juce::Array<juce::var>a;for(const auto&r:records)a.add(r.toVar());return juce::var(a);}
bool ProcessingHistory::fromVar(const juce::var&v){auto*a=v.getArray();if(!a)return false;juce::Array<ProcessingRecord>loaded;for(const auto&x:*a){ProcessingRecord r;if(!ProcessingRecord::fromVar(x,r))return false;loaded.add(r);}records=loaded;return true;}
