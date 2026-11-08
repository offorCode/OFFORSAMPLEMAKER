#include "SampleImporter.h"
#include "AudioAnalyzer.h"

SampleImporter::SampleImporter(juce::File folder, juce::Array<SampleRecord> existing, Completion done, Progress reportProgress)
    : ThreadPoolJob("Sample folder import"), root(folder), current(std::move(existing)), completion(std::move(done)), progress(std::move(reportProgress)) {}

juce::ThreadPoolJob::JobStatus SampleImporter::runJob()
{
    juce::AudioFormatManager fm; fm.registerBasicFormats(); juce::Array<juce::File> files;
    juce::Array<juce::File> directories; directories.add(root);
    while (!directories.isEmpty())
    {
        if (shouldExit()) return jobHasFinished;
        auto directory = directories.removeAndReturn(directories.size() - 1);
        for (const auto& entry : juce::RangedDirectoryIterator(
                 directory, false, "*", juce::File::findFilesAndDirectories,
                 juce::File::FollowSymlinks::no))
        {
            if (shouldExit()) return jobHasFinished;
            auto item = entry.getFile();
            if (item.isDirectory()) directories.add(item); else files.add(item);
        }
    }
    int unsupported=0, added=0; juce::Array<SampleRecord> result=current;
    int index=0;
    for (const auto& file : files)
    {
        if (shouldExit()) return jobHasFinished;
        ++index;
        if (progress && (index % 32 == 0 || index == files.size()))
            juce::MessageManager::callAsync([cb=progress, value=(double)index/(double)juce::jmax(1,files.size())] { cb(value); });
        const auto ext=file.getFileExtension().toLowerCase();
        if (ext!=".wav" && ext!=".aif" && ext!=".aiff" && ext!=".mp3" && ext!=".flac" && ext!=".ogg") { ++unsupported; continue; }
        bool duplicate=false; for (const auto& r: result) if (juce::File(r.path).getFullPathName().equalsIgnoreCase(file.getFullPathName())) { duplicate=true; break; }
        if (duplicate) continue;
        std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file)); if (!reader) { ++unsupported; continue; }
        SampleRecord r; r.id=juce::Uuid().toString(); r.path=file.getFullPathName(); r.fileSize=file.getSize();
        r.durationSeconds=(double)reader->lengthInSamples/reader->sampleRate; r.sampleRate=reader->sampleRate;
        r.channels=(int)reader->numChannels; r.bitDepth=reader->bitsPerSample; r.format=ext.trimCharactersAtStart(".").toUpperCase();
        r.importDate=juce::Time::getCurrentTime().toISO8601(true); r.features=AudioAnalyzer::analyze(fm,file);
        if (r.features.classificationStatus=="Provisional") r.category=r.features.predictedCategory;
        result.add(r); ++added;
    }
    if (completion) juce::MessageManager::callAsync([callback=std::move(completion), result=std::move(result), unsupported, added]() mutable { callback(std::move(result),unsupported,added); });
    return jobHasFinished;
}
