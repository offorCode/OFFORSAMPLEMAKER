#pragma once
#include <JuceHeader.h>
#include "VariationRenderer.h"
#include "VariationLockManager.h"

class VariationPanel final : public juce::Component, private juce::ListBoxModel
{
public:
    using StateChanged=std::function<void(const VariationSettings&,const juce::Array<Variation>&)>;
    using Preview=std::function<void(const juce::File&)>;
    VariationPanel(juce::AudioFormatManager&,juce::AudioThumbnailCache&);~VariationPanel()override;
    void paint(juce::Graphics&)override;void resized()override;
    void setSource(const juce::File&,const juce::String&,const juce::String&,const juce::File&,const std::vector<EffectSettings>&);
    void setData(const VariationSettings&,const juce::Array<Variation>&);
    void setCallbacks(StateChanged,Preview);
    void cancelGeneration();
private:
    int getNumRows()override;void paintListBoxItem(int,juce::Graphics&,int,int,bool)override;void selectedRowsChanged(int)override;
    void refresh();void renderTargets(juce::Array<int>);void renderAllUnlocked();void renderSelected();void cancel();void exportSelected();void save();void updateThumbnails();void notifyChanges();
    juce::AudioFormatManager& formats;juce::AudioThumbnail originalThumb,variationThumb;juce::File source,assets;juce::String sampleId,category;std::vector<EffectSettings> baseEffects;VariationSettings settings;juce::Array<Variation> variations;juce::ThreadPool pool{1};std::shared_ptr<std::atomic<bool>> cancelToken;juce::uint64 generation=0;bool rendering=false;double progressValue=0;
    StateChanged onStateChanged;Preview onPreview;
    juce::ListBox list{"Generated variations",this};juce::ComboBox count,intensity;juce::Slider strength,maxGain,maxEq,maxDrive;juce::ToggleButton allowEq,allowComp,allowSat,allowVerb,allowDelay,allowFilter,allowGain;
    juce::TextButton generate,regenSelected,regenUnlocked,cancelButton,playOriginal,playVariation,lockButton,favoriteButton,renameButton,deleteButton,compareButton,stopButton,exportButton;
    std::unique_ptr<juce::FileChooser> chooser;bool compareVariationNext=false;
    juce::Label status;juce::ProgressBar progress{progressValue};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariationPanel)
};
