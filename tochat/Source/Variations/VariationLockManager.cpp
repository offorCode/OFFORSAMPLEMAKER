#include "VariationLockManager.h"
bool VariationLockManager::setLocked(VariationHistory& h,const juce::String&id,bool value){if(auto*v=h.find(id)){v->locked=value;return true;}return false;}
juce::Array<juce::String> VariationLockManager::unlockedIds(const VariationHistory&h,const juce::String&sample){juce::Array<juce::String>ids;for(auto&v:h.all())if(v.originalSampleId==sample&&!v.locked&&!v.deleted)ids.add(v.id);return ids;}
