#include "VariationHistory.h"
bool VariationHistory::remove(const juce::String&id){for(int i=0;i<records.size();++i)if(records.getReference(i).id==id){records.remove(i);return true;}return false;}
Variation* VariationHistory::find(const juce::String&id){for(auto&r:records)if(r.id==id)return &r;return nullptr;}
juce::var VariationHistory::toVar()const{juce::Array<juce::var>a;for(auto&r:records)a.add(r.toVar());return juce::var(a);}
bool VariationHistory::fromVar(const juce::var&v){auto*a=v.getArray();if(!a)return false;juce::Array<Variation> loaded;for(auto&x:*a){Variation r;if(!Variation::fromVar(x,r))return false;loaded.add(r);}records=loaded;return true;}
