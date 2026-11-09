#include "PluginScanJob.h"

PluginScanJob::PluginScanJob(juce::StringArray paths, juce::String xml, juce::File pedal,
                             bool rescan, Completion done, Progress update,
                             std::shared_ptr<std::atomic<bool>> token)
    : ThreadPoolJob("Scan VST3 plugin directories"), directories(std::move(paths)),
      previousXml(std::move(xml)), deadMansPedal(std::move(pedal)), forceRescan(rescan),
      completion(std::move(done)), progress(std::move(update)), cancellation(std::move(token)) {}

bool PluginScanJob::cancelled() const { return shouldExit() || (cancellation && cancellation->load()); }

void PluginScanJob::report(double value, const juce::String& path)
{
    if (!progress) return;
    auto cb = progress;
    juce::MessageManager::callAsync([cb, value, path] { cb(value, path); });
}

juce::ThreadPoolJob::JobStatus PluginScanJob::runJob()
{
    PluginScanResult result;
    juce::AudioPluginFormatManager manager;
    juce::addDefaultFormatsToManager(manager);
    juce::KnownPluginList known;
    if (auto xml = juce::XmlDocument::parse(previousXml)) known.recreateFromXml(*xml);
    juce::Array<juce::AudioPluginFormat*> formats;
    for (int i = 0; i < manager.getNumFormats(); ++i)
        if (manager.getFormat(i)->getName().containsIgnoreCase("VST3")) formats.add(manager.getFormat(i));
    if (formats.isEmpty())
    {
        result.error = "VST3 scanning is unavailable in this build. VST2 hosting is not enabled.";
    }
    else
    {
        auto pedal = deadMansPedal;
        pedal.getParentDirectory().createDirectory();
        juce::PluginDirectoryScanner::applyBlacklistingsFromDeadMansPedal(known, pedal);
        int validDirectories = 0;
        for (int d = 0; d < directories.size() && !cancelled(); ++d)
        {
            const juce::File directory(directories[d]);
            if (!directory.isDirectory()) continue;
            ++validDirectories;
            for (int f = 0; f < formats.size() && !cancelled(); ++f)
            {
                auto& format = *formats.getReference(f);
                juce::PluginDirectoryScanner scanner(known, format,
                    juce::FileSearchPath(directory.getFullPathName()), true, pedal);
                juce::String current;
                while (!cancelled() && scanner.scanNextFile(!forceRescan, current))
                {
                    const double value = (double(d) + scanner.getProgress()) / juce::jmax(1, directories.size());
                    report(value, current);
                }
            }
            report(double(d + 1) / juce::jmax(1, directories.size()), directory.getFullPathName());
        }
        if (validDirectories == 0 && result.error.isEmpty())
            result.error = "No configured VST3 plugin directories exist yet.";
    }
    result.cancelled = cancelled();
    result.plugins = known.getTypes();
    if (auto xml = known.createXml()) result.xml = xml->toString();
    if (completion)
    {
        auto cb = completion;
        juce::MessageManager::callAsync([cb, result = std::move(result)]() mutable { cb(std::move(result)); });
    }
    return jobHasFinished;
}
