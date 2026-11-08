#include "EffectChain.h"

void EffectChain::setEffects(const std::vector<EffectSettings>& settings)
{ effects.clear();for(const auto& s:settings)add(s); }
const std::vector<EffectSettings> EffectChain::getSettings() const
{ std::vector<EffectSettings> out;out.reserve(effects.size());for(const auto& e:effects)out.push_back(e->settings());return out; }
void EffectChain::add(EffectSettings s){effects.push_back(makeBuiltInEffect(s));}
bool EffectChain::remove(size_t i){if(i>=effects.size())return false;effects.erase(effects.begin()+(std::ptrdiff_t)i);return true;}
bool EffectChain::move(size_t from,size_t to){if(from>=effects.size()||to>=effects.size())return false;if(from==to)return true;auto p=std::move(effects[from]);effects.erase(effects.begin()+(std::ptrdiff_t)from);effects.insert(effects.begin()+(std::ptrdiff_t)to,std::move(p));return true;}
void EffectChain::clear(){effects.clear();}
void EffectChain::prepare(double sr,int block,int channels){for(auto& e:effects)e->prepare(sr,block,channels);}
void EffectChain::process(juce::AudioBuffer<float>& b){for(auto& e:effects)e->process(b);}
void EffectChain::reset(){for(auto& e:effects)e->reset();}
juce::var EffectChain::toVar() const {juce::Array<juce::var> a;for(const auto& e:effects)a.add(e->settings().toVar());return juce::var(a);}
bool EffectChain::fromVar(const juce::var& v){auto* a=v.getArray();if(!a)return false;std::vector<EffectSettings> s;for(const auto& x:*a){EffectSettings e;if(!EffectSettings::fromVar(x,e))return false;s.push_back(e);}setEffects(s);return true;}
