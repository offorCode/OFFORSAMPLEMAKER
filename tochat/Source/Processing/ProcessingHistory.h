#pragma once
#include "EffectSettings.h"
#include "EffectProcessor.h"

struct ProcessingRecord
{
    juce::String id=juce::Uuid().toString(),sampleId,originalPath,renderedPath,timestamp,version="1";
    bool locked=false;
    std::vector<EffectSettings> effects;
    RenderOperations operations;
    juce::var toVar() const;
    static bool fromVar(const juce::var&,ProcessingRecord&);
};

class ProcessingHistory
{
public:
    void add(ProcessingRecord r){records.add(std::move(r));}
    const juce::Array<ProcessingRecord>& all()const{return records;}
    void clear(){records.clear();}
    juce::var toVar()const;
    bool fromVar(const juce::var&);
private:
    juce::Array<ProcessingRecord> records;
};
