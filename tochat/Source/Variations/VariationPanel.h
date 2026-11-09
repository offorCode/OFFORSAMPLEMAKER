#pragma once
#include <JuceHeader.h>
#include "VariationRenderer.h"
#include "VariationLockManager.h"

class VariationPanel final : public juce::Component, private juce::Timer
{
    class VariationCards;
public:
    using StateChanged=std::function<void(const VariationSettings&,const juce::Array<Variation>&)>;
    using Preview=std::function<void(const juce::File&)>;
    VariationPanel(juce::AudioFormatManager&,juce::AudioThumbnailCache&);~VariationPanel()override;
    void paint(juce::Graphics&)override;void resized()override;
    void setSource(const juce::File&,const juce::String&,const juce::String&,const juce::File&,const std::vector<EffectSettings>&);
    void setData(const VariationSettings&,const juce::Array<Variation>&);
    void setCallbacks(StateChanged,Preview);
    void cancelGeneration();
    void deleteSelected();
private:
    void timerCallback()override;void refresh();void renderTargets(juce::Array<int>);void renderAllUnlocked();void renderSelected();void cancel();void exportSelected();void beginExport(bool allVariations,bool zipArchive);void save();void updateThumbnails();void notifyChanges();void selectCard(int,bool);void updateSelectedCard();void previewCard(int);void toggleCardLock(int);void toggleCardFavorite(int);int getSelectedRow()const{return selectedRow;}
    juce::AudioFormatManager& formats;juce::AudioThumbnailCache& thumbnailCache;juce::AudioThumbnail originalThumb,variationThumb;juce::File source,assets;juce::String sampleId,category;std::vector<EffectSettings> baseEffects;VariationSettings settings;juce::Array<Variation> variations;juce::ThreadPool pool{1};std::shared_ptr<std::atomic<bool>> cancelToken;juce::uint64 generation=0;bool rendering=false;double progressValue=0;
    StateChanged onStateChanged;Preview onPreview;std::unique_ptr<VariationCards> cards;std::vector<std::unique_ptr<juce::AudioThumbnail>> cardThumbnails;juce::Array<int> selectedRows;int selectedRow=-1;
    juce::Viewport cardsViewport;juce::ComboBox count,intensity;juce::Slider strength,maxGain,maxEq,maxDrive;juce::ToggleButton allowEq,allowComp,allowSat,allowVerb,allowDelay,allowFilter,allowGain;
    juce::TextButton generate,regenSelected,regenUnlocked,cancelButton,playOriginal,playVariation,lockButton,favoriteButton,renameButton,deleteButton,compareButton,stopButton,exportButton;
    juce::TextButton selectAllButton,clearSelectionButton;
    juce::TextButton advancedButton { "Advanced Effect Ranges" };
    bool advancedVisible = false;
    std::unique_ptr<juce::FileChooser> chooser;bool compareVariationNext=false;
    juce::Label status;juce::ProgressBar progress{progressValue};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VariationPanel)
};
