#include "VariationRenderer.h"
#include "../Samples/AudioAnalyzer.h"

VariationRenderer::VariationRenderer(juce::File source,juce::File assets,juce::String id,juce::String cat,VariationSettings s,std::vector<EffectSettings> base,juce::Array<int> target,Completion done,Progress report,std::shared_ptr<std::atomic<bool>> cancel)
 :ThreadPoolJob("Generate sample variations"),sourceFile(std::move(source)),assetsDirectory(std::move(assets)),sampleId(std::move(id)),category(std::move(cat)),settings(s),baseEffects(std::move(base)),numbers(std::move(target)),completion(std::move(done)),progress(std::move(report)),cancelled(std::move(cancel)){}
void VariationRenderer::finish(juce::Array<Variation>result,juce::String error){if(completion){auto cb=completion;juce::MessageManager::callAsync([cb,result=std::move(result),error]()mutable{cb(std::move(result),error);});}}

juce::ThreadPoolJob::JobStatus VariationRenderer::runJob()
{
    juce::AudioFormatManager fm;fm.registerBasicFormats();auto original=AudioAnalyzer::analyze(fm,sourceFile);
    if(original.durationSeconds<=0||original.rms<=1e-7){finish({},"Source audio is missing, corrupt, or silent.");return jobHasFinished;}
    auto outputDir=assetsDirectory.getChildFile("Variations");if(!outputDir.createDirectory()){finish({},"Could not create the project variations directory.");return jobHasFinished;}
    juce::Array<Variation> made;juce::Array<juce::File> filesToRollback;juce::Array<double> rms,centroid,crest,durations;const auto baseSeed=settings.seed!=0?settings.seed:(juce::uint64)juce::Time::getMillisecondCounterHiRes();
    for(int targetIndex=0;targetIndex<numbers.size();++targetIndex)
    {
        if(isCancelled()){for(auto&f:filesToRollback)f.deleteFile();finish({},"Variation generation cancelled.");return jobHasFinished;}
        bool accepted=false;Variation acceptedVariation;juce::File acceptedFile;
        for(int attempt=0;attempt<4&&!accepted;++attempt)
        {
            const auto seed=baseSeed+(juce::uint64)numbers[targetIndex]*0x9e3779b97f4a7c15ULL+(juce::uint64)attempt*0xbf58476d1ce4e5b9ULL;
            auto effects=VariationEngine::createChain(settings,category,seed,baseEffects);const auto id=juce::Uuid().toString();auto dest=outputDir.getChildFile(juce::File::createLegalFileName(sampleId)+"_"+id+".wav");
            AudioRenderJob render(sourceFile,dest,sampleId,effects,{}, {},{},cancelled);render.runJob();
            if(isCancelled()){dest.deleteFile();for(auto&f:filesToRollback)f.deleteFile();finish({},"Variation generation cancelled.");return jobHasFinished;}
            if(!dest.existsAsFile())continue;
            auto features=AudioAnalyzer::analyze(fm,dest);const bool valid=features.durationSeconds>0&&features.rms>0.0001&&features.peak<1.0&&features.rms<0.95;
            bool diverse=valid&&VariationEngine::featureDistance(original.rms,original.spectralCentroid,original.crestFactor,original.durationSeconds,features.rms,features.spectralCentroid,features.crestFactor,features.durationSeconds)>=0.012;
            for(int k=0;k<rms.size()&&diverse;++k)if(VariationEngine::featureDistance(rms[k],centroid[k],crest[k],durations[k],features.rms,features.spectralCentroid,features.crestFactor,features.durationSeconds)<0.008)diverse=false;
            if(!diverse){dest.deleteFile();continue;}
            std::unique_ptr<juce::AudioFormatReader> metadata(fm.createReaderFor(dest));accepted=true;acceptedFile=dest;acceptedVariation.id=id;acceptedVariation.originalSampleId=sampleId;acceptedVariation.name="Variation "+juce::String(numbers[targetIndex]);acceptedVariation.createdAt=juce::Time::getCurrentTime().toISO8601(true);acceptedVariation.outputPath=dest.getFullPathName();acceptedVariation.number=numbers[targetIndex];acceptedVariation.seed=seed;acceptedVariation.effects=std::move(effects);acceptedVariation.durationSeconds=features.durationSeconds;acceptedVariation.sampleRate=metadata?metadata->sampleRate:0;acceptedVariation.channels=metadata?(int)metadata->numChannels:0;acceptedVariation.bitDepth=metadata?(int)metadata->bitsPerSample:0;acceptedVariation.rms=features.rms;acceptedVariation.peak=features.peak;acceptedVariation.spectralCentroid=features.spectralCentroid;acceptedVariation.crestFactor=features.crestFactor;rms.add(features.rms);centroid.add(features.spectralCentroid);crest.add(features.crestFactor);durations.add(features.durationSeconds);
        }
        if(!accepted){for(auto&f:filesToRollback)f.deleteFile();finish({},"Could not produce a valid, sufficiently distinct variation within four attempts.");return jobHasFinished;}
        filesToRollback.add(acceptedFile);made.add(acceptedVariation);if(progress){auto cb=progress;const double value=(double)(targetIndex+1)/(double)juce::jmax(1,numbers.size());juce::MessageManager::callAsync([cb,value]{cb(value);});}
    }
    if(isCancelled()){for(auto&f:filesToRollback)f.deleteFile();finish({},"Variation generation cancelled.");return jobHasFinished;}
    finish(std::move(made),{});return jobHasFinished;
}
