#include <JuceHeader.h>
#include "../Source/Project/ProjectManager.h"
#include "../Source/Samples/SampleImporter.h"
#include "../Source/Samples/AudioAnalyzer.h"
#include "../Source/Classification/SampleClassifier.h"
#include <iostream>

namespace
{
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

        auto projectFile = root.getChildFile("Demo.osfproj");
        ProjectManager project;
        require(project.create(projectFile, "Demo Pack"), "Project creation failed");
        imported.getReference(0).category = "Bass"; imported.getReference(0).manuallyCategorized = true;
        project.samples = imported; project.selectedSampleId = imported[0].id;
        juce::String error; require(project.save(error), "Project save failed: " + error);
        ProjectManager reopened;
        require(reopened.open(projectFile, error), "Project reopen failed: " + error);
        require(reopened.getName() == "Demo Pack" && reopened.samples.size() == 1, "Project metadata/sample persistence failed");
        require(reopened.samples[0].category == "Bass" && reopened.samples[0].manuallyCategorized, "Manual category did not persist");
        require(reopened.samples[0].features.spectralCentroid > 0.0 && reopened.samples[0].durationSeconds > 0.99, "Analysis metadata did not persist");
        require(reopened.selectedSampleId == imported[0].id && wave.existsAsFile(), "Selection or original file was not preserved");
        std::cout << "Stage 2 tests passed: analyzer, asynchronous import, metadata, and project round-trip.\n";
        root.deleteRecursively();
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    try { runTests(); return 0; }
    catch (const std::exception& e) { std::cerr << "Stage 2 test failure: " << e.what() << "\n"; return 1; }
}
