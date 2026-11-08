#include "ProjectManager.h"

bool ProjectManager::create(const juce::File& file, const juce::String& projectName)
{
    if (projectName.trim().isEmpty() || !file.getParentDirectory().createDirectory()) return false;
    projectFile = file; name = projectName.trim(); samples.clear(); selectedSampleId.clear();
    openState = true;
    exportSettings = new juce::DynamicObject();
    if (!getAssetsDirectory().createDirectory()) return false;
    juce::String error; return save(error);
}

bool ProjectManager::open(const juce::File& file, juce::String& error)
{
    auto parsed = juce::JSON::parse(file);
    auto* root = parsed.getDynamicObject();
    if (root == nullptr || (int)root->getProperty("schemaVersion") != 1)
    { error = "Invalid project file or unsupported schema version."; return false; }
    juce::Array<SampleRecord> loaded;
    auto list = root->getProperty("samples");
    if (auto* array = list.getArray())
        for (const auto& item : *array) { SampleRecord r; if (!SampleRecord::fromVar(item, r)) { error = "Project contains an invalid sample record."; return false; } loaded.add(r); }
    else { error = "Project sample list is invalid."; return false; }
    projectFile = file; name = (root->hasProperty("name") ? root->getProperty("name") : juce::var(file.getFileNameWithoutExtension())).toString();
    openState = true;
    samples = loaded; selectedSampleId = root->getProperty("selectedSampleId").toString();
    if (auto* settings = root->getProperty("exportSettings").getDynamicObject()) exportSettings = settings;
    else exportSettings = new juce::DynamicObject();
    error.clear(); return true;
}

bool ProjectManager::save(juce::String& error)
{
    if (projectFile == juce::File()) { error = "Choose a project file with Save As first."; return false; }
    auto* root = new juce::DynamicObject(); root->setProperty("schemaVersion", 1); root->setProperty("name", name);
    root->setProperty("selectedSampleId", selectedSampleId);
    juce::Array<juce::var> array;
    for (const auto& sample : samples) array.add(sample.toVar());
    root->setProperty("samples", juce::var(array));
    root->setProperty("exportSettings", juce::JSON::parse("{}"));
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
{ projectFile = {}; name = "Untitled Sample Pack"; openState = false; samples.clear(); selectedSampleId.clear(); exportSettings = new juce::DynamicObject(); }

juce::File ProjectManager::getAssetsDirectory() const
{ return projectFile.getSiblingFile(projectFile.getFileNameWithoutExtension() + "_assets"); }
