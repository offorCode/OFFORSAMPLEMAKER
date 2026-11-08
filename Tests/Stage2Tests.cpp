#include <JuceHeader.h>
#include "../Source/Project/ProjectManager.h"
#include "../Source/Samples/SampleImporter.h"
#include "../Source/Samples/AudioAnalyzer.h"
#include "../Source/Classification/SampleClassifier.h"
#include "../Source/Processing/EffectProcessor.h"
#include "../Source/Processing/AudioRenderJob.h"
#include <iostream>

namespace
{
    void require(bool condition, const juce::String& message);

    double toneAmplitude(const juce::AudioBuffer<float>& b,double frequency,double sampleRate)
    {
        double re=0,im=0;for(int i=0;i<b.getNumSamples();++i){const double p=2.0*juce::MathConstants<double>::pi*frequency*i/sampleRate;re+=b.getSample(0,i)*std::cos(p);im-=b.getSample(0,i)*std::sin(p);}return 2.0*std::sqrt(re*re+im*im)/b.getNumSamples();
    }

    void runDspChecks()
    {
        constexpr double sr=44100.0;constexpr int n=44100;
        juce::AudioBuffer<float> eq(1,n);for(int i=0;i<n;++i)eq.setSample(0,i,.15f*std::sin(2*juce::MathConstants<float>::pi*100*i/(float)sr)+.15f*std::sin(2*juce::MathConstants<float>::pi*1000*i/(float)sr));
        const auto before=toneAmplitude(eq,1000,sr);auto s=EffectSettings::defaults(EffectType::parametricEq);s.frequency=1000;s.gainDb=12;EffectProcessor::processBuffer(eq,sr,{s});require(toneAmplitude(eq,1000,sr)>before*2.5,"EQ failed to boost its target frequency");

        juce::AudioBuffer<float> comp(1,n);for(int i=0;i<n;++i)comp.setSample(0,i,(i<n/2?.15f:.8f)*std::sin(2*juce::MathConstants<float>::pi*220*i/(float)sr));
        const auto ratioBefore=(double)comp.getMagnitude(0,n/2,n/2)/std::max(1.0e-6,(double)comp.getMagnitude(0,0,n/2));s=EffectSettings::defaults(EffectType::compressor);s.thresholdDb=-18;s.ratio=8;s.attackMs=1;s.releaseMs=10;EffectProcessor::processBuffer(comp,sr,{s});const auto ratioAfter=(double)comp.getMagnitude(0,n/2,n/2)/std::max(1.0e-6,(double)comp.getMagnitude(0,0,n/2));require(ratioAfter<ratioBefore*.55,"Compressor failed to reduce dynamic range");

        juce::AudioBuffer<float> sat(1,512);for(int i=0;i<512;++i)sat.setSample(0,i,.9f*std::sin(2*juce::MathConstants<float>::pi*997*i/(float)sr));juce::AudioBuffer<float> satOriginal(1,512);satOriginal.copyFrom(0,0,sat,0,0,512);s=EffectSettings::defaults(EffectType::saturation);s.driveDb=24;EffectProcessor::processBuffer(sat,sr,{s});double satDiff=0;for(int i=0;i<512;++i)satDiff+=std::abs(sat.getSample(0,i)-satOriginal.getSample(0,i));require(satDiff>1.0,"Saturation did not change the waveform");

        juce::AudioBuffer<float> delay(1,8192);delay.clear();delay.setSample(0,0,1);s=EffectSettings::defaults(EffectType::delay);s.delayMs=50;s.feedback=.3f;EffectProcessor::processBuffer(delay,sr,{s});require(std::abs(delay.getSample(0,2205))>.1f,"Delay did not produce a delayed impulse");
        juce::AudioBuffer<float> reverb(1,65536);reverb.clear();reverb.setSample(0,0,1);s=EffectSettings::defaults(EffectType::reverb);EffectProcessor::processBuffer(reverb,sr,{s});double tail=0;for(int i=8000;i<reverb.getNumSamples();++i)tail+=std::abs(reverb.getSample(0,i));require(tail>0.01,"Reverb produced no measurable tail");

        juce::AudioBuffer<float> gain(1,8);gain.clear();gain.setSample(0,0,.25f);s=EffectSettings::defaults(EffectType::gain);s.levelDb=6;EffectProcessor::processBuffer(gain,sr,{s});require(std::abs(gain.getSample(0,0)-.25f*juce::Decibels::decibelsToGain(6.0f))<1e-5,"Gain amplitude is incorrect");
        juce::AudioBuffer<float> bypass(1,16);for(int i=0;i<16;++i)bypass.setSample(0,i,.03f*i);juce::AudioBuffer<float> bypassCopy(1,16);bypassCopy.copyFrom(0,0,bypass,0,0,16);s=EffectSettings::defaults(EffectType::gain);s.levelDb=18;s.enabled=false;EffectProcessor::processBuffer(bypass,sr,{s});require(bypass.getMagnitude(0,0,16)==bypassCopy.getMagnitude(0,0,16),"Disabled effect altered its input");

        juce::AudioBuffer<float> limit(1,256);for(int i=0;i<256;++i)limit.setSample(0,i,2.0f*std::sin(2*juce::MathConstants<float>::pi*1000*i/(float)sr));s=EffectSettings::defaults(EffectType::limiter);s.ceilingDb=-6;EffectProcessor::processBuffer(limit,sr,{s});require(limit.getMagnitude(0,0,256)<=juce::Decibels::decibelsToGain(-6.0f)+1e-4,"Limiter exceeded its output ceiling");
        juce::AudioBuffer<float> normalized(1,4);normalized.setSample(0,0,.1f);normalized.setSample(0,1,-.5f);normalized.setSample(0,2,.25f);normalized.setSample(0,3,0);AudioOperations::normalize(normalized,-1,0);require(std::abs(normalized.getMagnitude(0,0,4)-juce::Decibels::decibelsToGain(-1.0f))<1e-5,"Normalize missed its target");
        juce::AudioBuffer<float> ops(1,6);for(int i=0;i<6;++i)ops.setSample(0,i,(float)i/5);AudioOperations::reverse(ops);require(ops.getSample(0,0)==1.0f&&ops.getSample(0,5)==0.0f,"Reverse failed");AudioOperations::fadeIn(ops,sr,1.0);require(ops.getSample(0,0)==0.0f,"Fade in failed");AudioOperations::fadeOut(ops,sr,.1);require(std::abs(ops.getSample(0,5))<1e-6,"Fade out failed");
        juce::AudioBuffer<float> trim(1,8);trim.clear();trim.setSample(0,3,.2f);trim.setSample(0,4,.1f);require(AudioOperations::trim(trim,-60)&&trim.getNumSamples()==2,"Silence trim failed");

        juce::AudioBuffer<float> orderA(1,256);for(int i=0;i<256;++i)orderA.setSample(0,i,.35f*std::sin(2*juce::MathConstants<float>::pi*330*i/(float)sr));juce::AudioBuffer<float>orderB(1,256);orderB.copyFrom(0,0,orderA,0,0,256);auto g=EffectSettings::defaults(EffectType::gain);g.levelDb=12;auto satSetting=EffectSettings::defaults(EffectType::saturation);satSetting.driveDb=20;EffectProcessor::processBuffer(orderA,sr,{g,satSetting});EffectProcessor::processBuffer(orderB,sr,{satSetting,g});double orderDifference=0;for(int i=0;i<256;++i)orderDifference+=std::abs(orderA.getSample(0,i)-orderB.getSample(0,i));require(orderDifference>.01,"Effect chain order was ignored");
    }

    void require(bool condition, const juce::String& message)
    {
        if (!condition) throw std::runtime_error(message.toStdString());
    }

    juce::File makeWav(const juce::File& folder)
    {
        auto file = folder.getChildFile("fixture.wav");
        auto stream = std::make_unique<juce::FileOutputStream>(file);
        juce::WavAudioFormat wav;
        auto writer = std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(
            stream.get(), 44100.0, 1, 16, {}, 0));
        require(writer != nullptr, "Could not create WAV fixture");
        stream.release();
        juce::AudioBuffer<float> audio(1, 44100);
        auto* data = audio.getWritePointer(0);
        for (int i = 0; i < audio.getNumSamples(); ++i)
            data[i] = 0.65f * std::sin(juce::MathConstants<float>::twoPi * 120.0f * (float)i / 44100.0f)
                    * std::exp(-(float)i / 44100.0f * 3.0f);
        require(writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()), "Could not write WAV fixture");
        writer.reset();
        return file;
    }

    void runTests()
    {
        runDspChecks();
        auto root = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("OfforSampleForgeStage2Tests");
        root.deleteRecursively();
        require(root.createDirectory(), "Could not create temporary test directory");
        auto sourceFolder = root.getChildFile("sources"); sourceFolder.createDirectory();
        auto wave = makeWav(sourceFolder);
        sourceFolder.getChildFile("unsupported.txt").replaceWithText("not audio");

        juce::AudioFormatManager fm; fm.registerBasicFormats();
        auto features = AudioAnalyzer::analyze(fm, wave);
        require(features.durationSeconds > 0.99 && features.durationSeconds < 1.01, "Duration extraction failed");
        require(features.rms > 0.05 && features.rms < 1.0 && features.peak > features.rms
                && features.peak <= 1.0 && features.spectralCentroid > 0.0,
                "Audio feature extraction failed or returned out-of-range values");
        require((features.classificationStatus == "Uncertain" && features.confidence == 0.0)
                || (features.classificationStatus == "Provisional" && features.predictedCategory == "Percussion"
                    && features.confidence > 0.0 && features.confidence < 0.5),
                "Classifier did not return a conservative provisional result");
        require(ProvisionalHeuristicClassifier().classify(AudioFeatures {}).status == "Uncertain",
                "Empty features should remain uncertain");

        std::atomic<bool> completed { false };
        juce::Array<SampleRecord> imported; int unsupported = -1, added = -1;
        juce::ThreadPool pool(1);
        auto* job = new SampleImporter(sourceFolder, {}, [&](juce::Array<SampleRecord> records, int bad, int count)
        { imported = records; unsupported = bad; added = count; completed = true; }, [](double) {});
        pool.addJob(job, false);
        const auto deadline = juce::Time::getMillisecondCounter() + 10000;
        while (!completed && juce::Time::getMillisecondCounter() < deadline)
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        require(completed, "Background import did not complete");
        require(added == 1 && unsupported == 1 && imported.size() == 1, "Import count or unsupported-file reporting failed");
        require(imported[0].fileSize == wave.getSize() && imported[0].channels == 1 && imported[0].bitDepth == 16, "Imported metadata incorrect");
        require(imported[0].id.isNotEmpty() && imported[0].path == wave.getFullPathName(), "Stable ID or original path missing");
        pool.removeJob(job, false, 1000); delete job;

        completed = false; unsupported = -1; added = -1;
        auto* duplicateJob = new SampleImporter(sourceFolder, imported,
            [&](juce::Array<SampleRecord> records, int bad, int count)
            { imported = records; unsupported = bad; added = count; completed = true; }, [](double) {});
        pool.addJob(duplicateJob, false);
        const auto duplicateDeadline = juce::Time::getMillisecondCounter() + 10000;
        while (!completed && juce::Time::getMillisecondCounter() < duplicateDeadline)
            juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        require(completed && added == 0 && imported.size() == 1, "Duplicate audio path was imported twice");
        pool.removeJob(duplicateJob, false, 1000); delete duplicateJob;

        juce::MemoryBlock sourceBefore;require(wave.loadFileAsData(sourceBefore),"Could not snapshot source fixture");
        auto renderFile=root.getChildFile("renders/gain_render.wav");bool renderDone=false;juce::String renderError;juce::File rendered;
        auto renderGain=EffectSettings::defaults(EffectType::gain);renderGain.levelDb=2.0f;juce::ThreadPool renderPool(1);
        auto* renderJob=new AudioRenderJob(wave,renderFile,imported[0].id,{renderGain},[&](juce::File result,const juce::String& error){rendered=result;renderError=error;renderDone=true;},[](double){});
        renderPool.addJob(renderJob,false);const auto renderDeadline=juce::Time::getMillisecondCounter()+10000;
        while(!renderDone&&juce::Time::getMillisecondCounter()<renderDeadline)juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        require(renderDone&&renderError.isEmpty()&&rendered.existsAsFile(),"Offline render failed: "+renderError);
        juce::MemoryBlock sourceAfter;require(wave.loadFileAsData(sourceAfter)&&sourceBefore==sourceAfter,"Rendering modified original sample bytes");
        std::unique_ptr<juce::AudioFormatReader> renderedReader(fm.createReaderFor(rendered));require(renderedReader&&renderedReader->lengthInSamples>=44100,"Rendered WAV could not be reopened");
        juce::AudioBuffer<float> renderCheck(1,4096);renderCheck.clear();require(renderedReader->read(&renderCheck,0,4096,0,true,true),"Could not decode rendered output");double renderedPeak=0;for(int i=0;i<4096;++i)renderedPeak=juce::jmax(renderedPeak,(double)std::abs(renderCheck.getSample(0,i)));require(renderedPeak>0.2,"Rendered gain output is silent or unchanged");
        renderPool.removeJob(renderJob,false,1000);delete renderJob;

        auto projectFile = root.getChildFile("Demo.osfproj");
        ProjectManager project;
        require(project.create(projectFile, "Demo Pack"), "Project creation failed");
        imported.getReference(0).category = "Bass"; imported.getReference(0).manuallyCategorized = true;
        project.samples = imported; project.selectedSampleId = imported[0].id;
        auto gainSetting=EffectSettings::defaults(EffectType::gain);gainSetting.levelDb=6.0f;project.processingChainSettings={gainSetting};project.processingOperations.reverse=true;project.processingOperations.fadeInMs=5.0;
        project.exportSettings->setProperty("format", "FLAC");
        ProcessingRecord history;history.sampleId=imported[0].id;history.originalPath=wave.getFullPathName();history.renderedPath=rendered.getFullPathName();history.effects={renderGain};history.timestamp=juce::Time::getCurrentTime().toISO8601(true);project.processingHistory.add(history);
        juce::String error; require(project.save(error), "Project save failed: " + error);
        ProjectManager reopened;
        require(reopened.open(projectFile, error), "Project reopen failed: " + error);
        require(reopened.getName() == "Demo Pack" && reopened.samples.size() == 1, "Project metadata/sample persistence failed");
        require(reopened.samples[0].category == "Bass" && reopened.samples[0].manuallyCategorized, "Manual category did not persist");
        require(reopened.samples[0].features.spectralCentroid > 0.0 && reopened.samples[0].durationSeconds > 0.99, "Analysis metadata did not persist");
        require(reopened.processingChainSettings.size()==1&&std::abs(reopened.processingChainSettings[0].levelDb-6.0f)<1e-5&&reopened.processingHistory.all().size()==1,"Processing settings/history did not persist");
        require(reopened.processingOperations.reverse&&std::abs(reopened.processingOperations.fadeInMs-5.0)<1e-6,"Audio operation settings did not persist");
        require(reopened.exportSettings->getProperty("format").toString()=="FLAC","Export settings did not persist");
        require(reopened.rename("Renamed Pack")&&reopened.save(error),"Project rename/save failed");auto saveAsFile=root.getChildFile("Copied.osfproj");require(reopened.saveAs(saveAsFile,error),"Project Save As failed: "+error);ProjectManager saveAsReopened;require(saveAsReopened.open(saveAsFile,error)&&saveAsReopened.getName()=="Renamed Pack"&&saveAsReopened.processingHistory.all().size()==1,"Save As data round-trip failed");
        auto legacyFile=root.getChildFile("Legacy.osfproj");auto* legacy=new juce::DynamicObject();legacy->setProperty("schemaVersion",1);legacy->setProperty("name","Legacy");legacy->setProperty("selectedSampleId",imported[0].id);juce::Array<juce::var> legacySamples;legacySamples.add(imported[0].toVar());legacy->setProperty("samples",juce::var(legacySamples));legacyFile.replaceWithText(juce::JSON::toString(juce::var(legacy),true));ProjectManager legacyProject;require(legacyProject.open(legacyFile,error)&&legacyProject.samples.size()==1&&legacyProject.processingChainSettings.empty()&&!legacyProject.processingOperations.any(),"Stage 2 schema compatibility failed");
        require(reopened.selectedSampleId == imported[0].id && wave.existsAsFile(), "Selection or original file was not preserved");
        std::cout << "Stage 3 tests passed: DSP, offline rendering, import/analyzer regression, metadata and Stage 1/2 project compatibility.\n";
        root.deleteRecursively();
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    try { runTests(); return 0; }
    catch (const std::exception& e) { std::cerr << "Stage 2 test failure: " << e.what() << "\n"; return 1; }
}
