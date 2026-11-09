#include "VariationExporter.h"

namespace
{
juce::File uniqueSibling(juce::File requested)
{
    auto candidate=requested;int suffix=2;
    while(candidate.existsAsFile())candidate=requested.getSiblingFile(requested.getFileNameWithoutExtension()+"_"+juce::String(suffix++)+requested.getFileExtension());
    return candidate;
}
}

bool VariationExporter::copyWavs(const juce::Array<juce::File>& files,const juce::File& directory,int& exported,juce::String& error)
{
    exported=0;if(!directory.isDirectory()){error="The export destination is not a directory.";return false;}
    for(const auto&source:files){if(!source.existsAsFile()){error="A selected variation file is missing.";return false;}const auto target=uniqueSibling(directory.getChildFile(source.getFileName()));if(!source.copyFileTo(target)){error="Could not copy "+source.getFileName()+".";return false;}++exported;}
    error.clear();return true;
}

bool VariationExporter::createZip(const juce::Array<juce::File>& files,const juce::File& requested,juce::File& created,juce::String& error)
{
    if(files.isEmpty()){error="There are no variation files to archive.";return false;}
    for(const auto&f:files)if(!f.existsAsFile()){error="A selected variation file is missing.";return false;}
    auto target=requested;if(target.getFileExtension().isEmpty())target=target.withFileExtension(".zip");created=uniqueSibling(target);
    juce::ZipFile::Builder builder;for(const auto&f:files)builder.addFile(f,6,f.getFileName());
    auto temp=created.getSiblingFile(created.getFileNameWithoutExtension()+"_"+juce::Uuid().toString()+".partial.zip");bool ok=false;
    {juce::FileOutputStream stream(temp);double progress=0;ok=stream.openedOk()&&builder.writeToStream(stream,&progress);if(ok)stream.flush();}
    if(ok)ok=temp.moveFileTo(created);
    if(!ok){temp.deleteFile();error="Could not create the ZIP archive.";created={};return false;}
    error.clear();return true;
}
