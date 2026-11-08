#include "AudioRenderJob.h"
#include "EffectChain.h"

AudioRenderJob::AudioRenderJob(juce::File source,juce::File destination,juce::String sampleId,std::vector<EffectSettings> chain,Completion done,Progress report,RenderOperations renderOptions,std::shared_ptr<std::atomic<bool>> cancel)
    :ThreadPoolJob("Offline sample render"),sourceFile(std::move(source)),destinationFile(std::move(destination)),sourceId(std::move(sampleId)),effects(std::move(chain)),completion(std::move(done)),progress(std::move(report)),operations(renderOptions),cancellation(std::move(cancel)){}

void AudioRenderJob::finish(juce::String error)
{if(completion){auto cb=completion;auto dest=destinationFile;juce::MessageManager::callAsync([cb,dest,error]{cb(error.isEmpty()?dest:juce::File(),error);});}}

juce::ThreadPoolJob::JobStatus AudioRenderJob::runJob()
{
    juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(sourceFile));
    if(!reader||reader->lengthInSamples<=0||reader->sampleRate<=0||reader->numChannels<1||reader->numChannels>2){finish("Unsupported, empty, or corrupt source audio.");return jobHasFinished;}
    if(!destinationFile.getParentDirectory().createDirectory()){finish("Could not create the project render directory.");return jobHasFinished;}
    auto stream=std::make_unique<juce::FileOutputStream>(destinationFile);if(!stream->openedOk()){finish("Could not create the rendered audio file.");return jobHasFinished;}
    juce::WavAudioFormat wav;std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.get(),reader->sampleRate,(unsigned int)reader->numChannels,24,{},0));
    if(!writer){finish("Could not create a WAV writer.");return jobHasFinished;}stream.release();
    EffectChain chain;chain.setEffects(effects);chain.prepare(reader->sampleRate,4096,(int)reader->numChannels);
    constexpr int block=4096;juce::AudioBuffer<float> buffer((int)reader->numChannels,block);juce::int64 pos=0;bool failed=false;juce::int64 renderLength=reader->lengthInSamples;
    if(operations.any())
    {
        constexpr juce::int64 maxOperationFrames=5000000;
        if(reader->lengthInSamples>maxOperationFrames){writer.reset();destinationFile.deleteFile();finish("This operation set is limited to samples shorter than five million frames.");return jobHasFinished;}
        buffer.setSize((int)reader->numChannels,(int)reader->lengthInSamples,false,false,true);buffer.clear();
        if(!reader->read(&buffer,0,buffer.getNumSamples(),0,true,true)){writer.reset();destinationFile.deleteFile();finish("Could not read the complete source for the selected operations.");return jobHasFinished;}
        if(operations.trimSilence&&!AudioOperations::trim(buffer,operations.trimThresholdDb)){writer.reset();destinationFile.deleteFile();finish("The source contains no audio above the trim threshold.");return jobHasFinished;}
        if(operations.reverse)AudioOperations::reverse(buffer);
        if(operations.fadeInMs>0)AudioOperations::fadeIn(buffer,reader->sampleRate,operations.fadeInMs);
        if(operations.fadeOutMs>0)AudioOperations::fadeOut(buffer,reader->sampleRate,operations.fadeOutMs);
        juce::AudioBuffer<float> processBlock((int)reader->numChannels,block);
        const auto totalFrames=buffer.getNumSamples();
        for(juce::int64 offset=0;offset<totalFrames;offset+=block){if(isCancelled()){failed=true;break;}const int n=(int)juce::jmin<juce::int64>(block,totalFrames-offset);processBlock.setSize((int)reader->numChannels,n,false,false,true);for(int c=0;c<processBlock.getNumChannels();++c)processBlock.copyFrom(c,0,buffer,c,(int)offset,n);chain.process(processBlock);for(int c=0;c<processBlock.getNumChannels();++c)buffer.copyFrom(c,(int)offset,processBlock,c,0,n);if(progress){auto cb=progress;const double v=0.65*(double)(offset+n)/(double)totalFrames;juce::MessageManager::callAsync([cb,v]{cb(v);});}}
        if(failed){writer.reset();destinationFile.deleteFile();finish("Render cancelled.");return jobHasFinished;}
        if(operations.normalize)AudioOperations::normalize(buffer,operations.normalizeTargetDb,operations.normalizeCeilingDb);
        renderLength=buffer.getNumSamples();
        for(pos=0;pos<renderLength;pos+=block){if(isCancelled()){failed=true;break;}const int n=(int)juce::jmin<juce::int64>(block,renderLength-pos);if(!writer->writeFromAudioSampleBuffer(buffer,(int)pos,n)){failed=true;break;}if(progress){auto cb=progress;const double v=0.65+0.35*(double)(pos+n)/(double)renderLength;juce::MessageManager::callAsync([cb,v]{cb(v);});}}
    }
    while(!operations.any()&&pos<reader->lengthInSamples)
    {
        if(isCancelled()){failed=true;break;}const int n=(int)juce::jmin<juce::int64>(block,reader->lengthInSamples-pos);buffer.clear();
        buffer.setSize((int)reader->numChannels,n,false,false,true);
        if(!reader->read(&buffer,0,n,pos,true,true)){failed=true;break;}chain.process(buffer);
        if(!writer->writeFromAudioSampleBuffer(buffer,0,n)){failed=true;break;}pos+=n;
        if(progress&&((pos/block)%8==0||pos>=reader->lengthInSamples)){auto cb=progress;const double v=(double)pos/(double)reader->lengthInSamples;juce::MessageManager::callAsync([cb,v]{cb(v);});}
    }
    bool tail=false;for(const auto&e:effects)if(e.enabled&&(e.type==EffectType::reverb||e.type==EffectType::delay)&&e.wet>0)tail=true;
    if(!failed&&tail){juce::AudioBuffer<float> silence((int)reader->numChannels,block);silence.clear();const int blocks=(int)std::ceil(reader->sampleRate*2.0/block);for(int i=0;i<blocks;++i){if(isCancelled()){failed=true;break;}chain.process(silence);if(!writer->writeFromAudioSampleBuffer(silence,0,block)){failed=true;break;}silence.clear();}}
    writer->flush();writer.reset();
    if(failed||!destinationFile.existsAsFile()||destinationFile.getSize()==0){destinationFile.deleteFile();finish(isCancelled()?"Render cancelled.":"Audio rendering failed.");return jobHasFinished;}
    finish({});return jobHasFinished;
}
