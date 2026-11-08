#include "VariationEngine.h"

std::vector<EffectSettings> VariationEngine::createChain(const VariationSettings& input,const juce::String& category,juce::uint64 seed,const std::vector<EffectSettings>& base)
{
    auto s=VariationSettings::fromVar(input.toVar());juce::Random random((juce::int64)seed);const float intensity[]={0.28f,0.5f,0.72f,1.0f,1.0f};const float amt=intensity[(int)s.intensity]*s.strength;
    struct Candidate{EffectType type;float weight;};std::vector<Candidate> types;
    if(s.allowEq)types.push_back({EffectType::parametricEq,1});if(s.allowCompressor)types.push_back({EffectType::compressor,1});if(s.allowSaturation)types.push_back({EffectType::saturation,1});if(s.allowReverb)types.push_back({EffectType::reverb,1});if(s.allowDelay)types.push_back({EffectType::delay,0.8f});if(s.allowFilter)types.push_back({EffectType::filter,0.8f});if(s.allowGain)types.push_back({EffectType::gain,0.6f});
    if(types.empty())return base;
    auto cat=category.toLowerCase();for(auto& c:types){if((cat.contains("kick")||cat.contains("808")||cat.contains("bass"))&&(c.type==EffectType::reverb||c.type==EffectType::delay))c.weight*=0.2f;if((cat.contains("snare")||cat.contains("clap"))&&(c.type==EffectType::reverb||c.type==EffectType::compressor))c.weight*=1.5f;if((cat.contains("hat")||cat.contains("cymbal"))&&(c.type==EffectType::filter||c.type==EffectType::parametricEq))c.weight*=1.5f;if((cat.contains("melody")||cat.contains("vocal")||cat.contains("loop"))&&(c.type==EffectType::reverb||c.type==EffectType::delay))c.weight*=1.5f;}
    std::vector<EffectSettings> result=base;const int wanted=juce::jmin((int)types.size(),1+(random.nextFloat()>.55f?1:0));std::vector<EffectType> used;
    for(int n=0;n<wanted;++n){float total=0;for(auto c:types)if(std::find(used.begin(),used.end(),c.type)==used.end())total+=c.weight;float pick=random.nextFloat()*total;EffectType chosen=types.front().type;for(auto c:types)if(std::find(used.begin(),used.end(),c.type)==used.end()){pick-=c.weight;if(pick<=0){chosen=c.type;break;}}used.push_back(chosen);auto e=EffectSettings::defaults(chosen);
        switch(chosen){case EffectType::parametricEq:e.frequency=120.0f+random.nextFloat()*9000.0f;e.gainDb=(random.nextFloat()*2-1)*s.maxEqDb*amt;e.q=.5f+random.nextFloat()*1.5f;break;case EffectType::compressor:e.thresholdDb=-30+random.nextFloat()*13;e.ratio=1.5f+random.nextFloat()*3.5f*amt;e.attackMs=4+random.nextFloat()*25;e.releaseMs=50+random.nextFloat()*180;break;case EffectType::saturation:e.driveDb=random.nextFloat()*s.maxDriveDb*amt;e.toneHz=2500+random.nextFloat()*14000;e.wet=.35f+.45f*amt;break;case EffectType::reverb:e.roomSize=.2f+random.nextFloat()*.65f;e.damping=.25f+random.nextFloat()*.6f;e.wet=.08f+.2f*amt;break;case EffectType::delay:e.delayMs=70+random.nextFloat()*480;e.feedback=.12f+random.nextFloat()*.35f;e.wet=.07f+.18f*amt;break;case EffectType::filter:e.highPass=random.nextBool();e.cutoffHz=e.highPass?70+random.nextFloat()*900:1800+random.nextFloat()*12000;e.q=.5f+random.nextFloat()*1.1f;break;case EffectType::gain:e.levelDb=(random.nextFloat()*2-1)*s.maxGainDb*amt;break;case EffectType::limiter:break;}result.push_back(e);}
    auto limiter=EffectSettings::defaults(EffectType::limiter);limiter.ceilingDb=-1.0f;result.push_back(limiter);
    return result;
}

double VariationEngine::featureDistance(double ra,double ca,double xa,double da,double rb,double cb,double xb,double db)
{
    const double rms=std::abs(std::log((ra+1.0e-7)/(rb+1.0e-7)));const double centroid=std::abs(ca-cb)/juce::jmax(100.0,juce::jmax(ca,cb));const double crest=std::abs(xa-xb)/juce::jmax(1.0,juce::jmax(xa,xb));const double duration=std::abs(da-db)/juce::jmax(.05,juce::jmax(da,db));return .4*rms+.3*centroid+.2*crest+.1*duration;
}
