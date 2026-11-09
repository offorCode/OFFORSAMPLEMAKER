#include "BuiltInEffects.h"
#include <array>

namespace
{
class BiquadEffect : public AudioEffect
{
public:
    using AudioEffect::AudioEffect;
    void prepare(double sr,int,int ch) override { rate=sr;channels=juce::jmin(ch,2);update();reset(); }
    void reset() override { x1.fill(0);x2.fill(0);y1.fill(0);y2.fill(0); }
    void processWet(juce::AudioBuffer<float>& b) override
    {
        update();const int chs=juce::jmin(2,b.getNumChannels());
        for(int c=0;c<chs;++c){auto* x=b.getWritePointer(c);for(int i=0;i<b.getNumSamples();++i){const double in=x[i];const double y=b0*in+b1*x1[c]+b2*x2[c]-a1*y1[c]-a2*y2[c];x2[c]=x1[c];x1[c]=in;y2[c]=y1[c];y1[c]=y;x[i]=(float)y;}}
    }
protected:
    virtual void update()=0;
    int channels=2; double b0=1,b1=0,b2=0,a1=0,a2=0;std::array<double,2>x1{},x2{},y1{},y2{};
};

class Eq final:public BiquadEffect{public:using BiquadEffect::BiquadEffect;private:void update()override{const auto f=juce::jlimit(20.0f,(float)(rate*0.48),state.frequency);const double A=std::pow(10.0,state.gainDb/40.0),w=2.0*juce::MathConstants<double>::pi*f/rate,alpha=std::sin(w)/(2.0*juce::jlimit(0.1f,18.0f,state.q)),c=std::cos(w),a0=1+alpha/A;b0=(1+alpha*A)/a0;b1=(-2*c)/a0;b2=(1-alpha*A)/a0;a1=(-2*c)/a0;a2=(1-alpha/A)/a0;}};

class Filter final:public BiquadEffect{public:using BiquadEffect::BiquadEffect;private:void update()override{const auto f=juce::jlimit(20.0f,(float)(rate*0.48),state.cutoffHz);const double w=2.0*juce::MathConstants<double>::pi*f/rate,c=std::cos(w),alpha=std::sin(w)/(2.0*juce::jlimit(0.1f,18.0f,state.q)),a0=1+alpha;if(state.highPass){b0=(1+c)/(2*a0);b1=-(1+c)/a0;b2=(1+c)/(2*a0);}else{b0=(1-c)/(2*a0);b1=(1-c)/a0;b2=(1-c)/(2*a0);}a1=-2*c/a0;a2=(1-alpha)/a0;}};

class Compressor final:public AudioEffect{public:using AudioEffect::AudioEffect;void prepare(double sr,int,int)override{rate=sr;reset();}void reset()override{env=0;}void processWet(juce::AudioBuffer<float>& b)override{const double ac=std::exp(-1.0/(juce::jmax(0.1f,state.attackMs)*0.001*rate)),rc=std::exp(-1.0/(juce::jmax(1.0f,state.releaseMs)*0.001*rate));for(int i=0;i<b.getNumSamples();++i){double level=0;for(int c=0;c<b.getNumChannels();++c)level=juce::jmax(level,(double)std::abs(b.getSample(c,i)));env=(level>env?ac:rc)*env+(1.0-(level>env?ac:rc))*level;const double db=20*std::log10(juce::jmax(env,1e-9)),over=db-state.thresholdDb;const double gainDb=over>0?-(over-over/juce::jmax(1.0f,state.ratio)):0;const float g=(float)(std::pow(10.0,(gainDb+state.levelDb)/20.0));for(int c=0;c<b.getNumChannels();++c)b.setSample(c,i,b.getSample(c,i)*g);}}private:double env=0;};

class Saturation final:public AudioEffect{public:using AudioEffect::AudioEffect;void prepare(double sr,int,int ch)override{rate=sr;channels=juce::jmin(ch,2);reset();}void reset()override{toneState.fill(0);}void processWet(juce::AudioBuffer<float>& b)override{const double a=std::exp(-2.0*juce::MathConstants<double>::pi*juce::jlimit(20.0f,(float)(rate*.48),state.toneHz)/rate),drive=std::pow(10.0,state.driveDb/20.0),normal=std::tanh(drive);for(int c=0;c<juce::jmin(2,b.getNumChannels());++c)for(int i=0;i<b.getNumSamples();++i){double x=b.getSample(c,i);toneState[c]=a*toneState[c]+(1-a)*x;x=std::tanh((x*0.65+toneState[c]*0.35)*drive)/juce::jmax(0.01,normal);b.setSample(c,i,(float)(x*std::pow(10.0,state.levelDb/20.0)));}}private:int channels=2;std::array<double,2>toneState{};};

class BuiltInReverb final:public AudioEffect{public:using AudioEffect::AudioEffect;void prepare(double sr,int,int)override{rate=sr;reverb.setSampleRate(sr);reset();}void reset()override{reverb.reset();}void processWet(juce::AudioBuffer<float>& b)override{juce::Reverb::Parameters p;p.roomSize=juce::jlimit(0.0f,1.0f,state.roomSize);p.damping=juce::jlimit(0.0f,1.0f,state.damping);p.width=juce::jlimit(0.0f,1.0f,state.width);p.wetLevel=1.0f;p.dryLevel=0.0f;reverb.setParameters(p);if(b.getNumChannels()==1)reverb.processMono(b.getWritePointer(0),b.getNumSamples());else if(b.getNumChannels()>=2)reverb.processStereo(b.getWritePointer(0),b.getWritePointer(1),b.getNumSamples());}private:juce::Reverb reverb;};

class Delay final:public AudioEffect{public:using AudioEffect::AudioEffect;void prepare(double sr,int,int ch)override{rate=sr;channels=juce::jmin(2,ch);size=juce::jmax(1,(int)(sr*2.0));for(auto& d:buffer)d.assign((size_t)size,0.0f);reset();}void reset()override{for(auto& d:buffer)std::fill(d.begin(),d.end(),0);write=0;}void processWet(juce::AudioBuffer<float>& b)override{const int delay=juce::jlimit(1,size-1,(int)(rate*juce::jlimit(1.0f,2000.0f,state.delayMs)/1000.0f));for(int i=0;i<b.getNumSamples();++i){const int read=(write-delay+size)%size;for(int c=0;c<juce::jmin(channels,b.getNumChannels());++c){const float input=b.getSample(c,i),echo=buffer[(size_t)c][(size_t)read];buffer[(size_t)c][(size_t)write]=input+echo*juce::jlimit(0.0f,0.95f,state.feedback);b.setSample(c,i,echo+input*(float)std::pow(10.0,state.levelDb/20.0));}write=(write+1)%size;}}private:int size=1,write=0,channels=2;std::array<std::vector<float>,2>buffer;};

class Gain final:public AudioEffect{public:using AudioEffect::AudioEffect;void prepare(double sr,int,int)override{rate=sr;}void reset()override{}void processWet(juce::AudioBuffer<float>& b)override{b.applyGain(juce::Decibels::decibelsToGain(state.levelDb));}};

class Limiter final:public AudioEffect{public:using AudioEffect::AudioEffect;void prepare(double sr,int,int)override{rate=sr;env=0;}void reset()override{env=0;}void processWet(juce::AudioBuffer<float>& b)override{const double ceiling=juce::Decibels::decibelsToGain(juce::jlimit(-24.0f,0.0f,state.ceilingDb)),rel=std::exp(-1.0/(juce::jmax(1.0f,state.limiterReleaseMs)*.001*rate));for(int i=0;i<b.getNumSamples();++i){double peak=0;for(int c=0;c<b.getNumChannels();++c)peak=juce::jmax(peak,(double)std::abs(b.getSample(c,i)));env=juce::jmax(peak,env*rel);const double g=env>ceiling?ceiling/env:1.0;for(int c=0;c<b.getNumChannels();++c)b.setSample(c,i,(float)(b.getSample(c,i)*g));}}private:double env=0;};
}

std::unique_ptr<AudioEffect> makeBuiltInEffect(const EffectSettings& s)
{
    switch(s.type){case EffectType::parametricEq:return std::make_unique<Eq>(s);case EffectType::compressor:return std::make_unique<Compressor>(s);case EffectType::saturation:return std::make_unique<Saturation>(s);case EffectType::reverb:return std::make_unique<BuiltInReverb>(s);case EffectType::delay:return std::make_unique<Delay>(s);case EffectType::filter:return std::make_unique<Filter>(s);case EffectType::gain:return std::make_unique<Gain>(s);case EffectType::limiter:return std::make_unique<Limiter>(s);}return {};
}
