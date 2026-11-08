#include "ProjectManager.h"

bool ProjectManager::create(const juce::File& file, const juce::String& projectName)
{
    if (projectName.trim().isEmpty() || !file.getParentDirectory().createDirectory()) return false;
    projectFile = file; name = projectName.trim(); samples.clear(); selectedSampleId.clear(); processingChainSettings.clear(); processingOperations = {}; processingHistory.clear();
    openState = true;
    exportSettings = new juce::DynamicObject();
    if (!getAssetsDirectory().createDirectory()) return false;
    juce::String error; return save(error);
}

bool ProjectManager::open(const juce::File& file, juce::String& error)
{
    auto parsed = juce::JSON::parse(file);
    auto* root = parsed.getDynamicObject();
    const int schema = root == nullptr ? 0 : (int)root->getProperty("schemaVersion");
    if (root == nullptr || (schema != 1 && schema != 2))
    { error = "Invalid project file or unsupported schema version."; return false; }
    juce::Array<SampleRecord> loaded;
    auto list = root->getProperty("samples");
    if (auto* array = list.getArray())
        for (const auto& item : *array) { SampleRecord r; if (!SampleRecord::fromVar(item, r)) { error = "Project contains an invalid sample record."; return false; } loaded.add(r); }
    else { error = "Project sample list is invalid."; return false; }
    std::vector<EffectSettings> loadedChain;
    if (root->hasProperty("processingChain"))
    {
        auto* chain = root->getProperty("processingChain").getArray();
        if (chain == nullptr) { error = "Project processing chain is invalid."; return false; }
        for (const auto& item : *chain) { EffectSettings setting; if (!EffectSettings::fromVar(item, setting)) { error = "Project contains invalid effect settings."; return false; } loadedChain.push_back(setting); }
    }
    ProcessingHistory loadedHistory;
    if (root->hasProperty("processingHistory") && !loadedHistory.fromVar(root->getProperty("processingHistory"))) { error = "Project processing history is invalid."; return false; }
    auto loadedOperations = root->hasProperty("processingOperations") ? RenderOperations::fromVar(root->getProperty("processingOperations")) : RenderOperations {};
    projectFile = file; name = (root->hasProperty("name") ? root->getProperty("name") : juce::var(file.getFileNameWithoutExtension())).toString();
    openState = true; samples = loaded; selectedSampleId = root->getProperty("selectedSampleId").toString();
    if (auto* settings = root->getProperty("exportSettings").getDynamicObject()) exportSettings = settings; else exportSettings = new juce::DynamicObject();
    processingChainSettings = std::move(loadedChain); processingOperations = loadedOperations; processingHistory = std::move(loadedHistory);
    error.clear(); return true;
}

bool ProjectManager::save(juce::String& error)
{
    if (projectFile == juce::File()) { error = "Choose a project file with Save As first."; return false; }
    auto* root = new juce::DynamicObject(); root->setProperty("schemaVersion", 2); root->setProperty("name", name);
    root->setProperty("selectedSampleId", selectedSampleId);
    juce::Array<juce::var> array;
    for (const auto& sample : samples) array.add(sample.toVar());
    root->setProperty("samples", juce::var(array));
    juce::Array<juce::var> chain; for (const auto& setting : processingChainSettings) chain.add(setting.toVar());
    root->setProperty("processingChain", juce::var(chain));
    root->setProperty("processingHistory", processingHistory.toVar());
    root->setProperty("processingOperations", processingOperations.toVar());
    root->setProperty("exportSettings", juce::var(exportSettings));
    auto json = juce::JSON::toString(juce::var(root), true);
    if (!projectFile.getParentDirectory().createDirectory() || !projectFile.replaceWithText(json))
    { error = "Could not write project file."; return false; }
    getAssetsDirectory().createDirectory(); error.clear(); return true;
}

bool ProjectManager::saveAs(const juce::File& file, juce::String& error)
{
    const auto old = projectFile; projectFile = file;
    if (!save(error)) { projectFile = old; return false; }
    openState = true;
    return true;
}

bool ProjectManager::rename(const juce::String& projectName)
{ if (projectName.trim().isEmpty()) return false; name = projectName.trim(); return true; }

void ProjectManager::close()
{ projectFile = {}; name = "Untitled Sample Pack"; openState = false; samples.clear(); selectedSampleId.clear(); exportSettings = new juce::DynamicObject(); processingChainSettings.clear(); processingOperations = {}; processingHistory.clear(); }

juce::File ProjectManager::getAssetsDirectory() const
{ return projectFile.getSiblingFile(projectFile.getFileNameWithoutExtension() + "_assets"); }
