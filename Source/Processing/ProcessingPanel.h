#pragma once
#include <JuceHeader.h>
#include "EffectSettings.h"
#include "ProcessingHistory.h"
#include "AudioRenderJob.h"

class ProcessingPanel final:public juce::Component
{
public:
    using ChainChanged=std::function<void(const std::vector<EffectSettings>&)>;
    using Rendered=std::function<void(const ProcessingRecord&)>;
    using Preview=std::function<void(const juce::File&)>;
    using OperationsChanged=std::function<void(const RenderOperations&)>;
    ProcessingPanel(juce::AudioFormatManager&, juce::AudioThumbnailCache&);~ProcessingPanel()override;
    void paint(juce::Graphics&)override;void resized()override;
    void setSource(const juce::File&,const juce::String&,const juce::File& assets);
    void setChain(const std::vector<EffectSettings>&);
    std::vector<EffectSettings> getChain()const;
    void setHistory(const juce::Array<ProcessingRecord>&);
    void setOperations(const RenderOperations&);
    RenderOperations getOperations()const{return operations;}
    void setCallbacks(ChainChanged,Rendered,Preview,OperationsChanged);
private:
    void refreshChain();void refreshHistory();void selectChainItem();void updateParameterLabels();void parameterChanged(int);void render();void cancelRender();
    juce::AudioFormatManager& formats;juce::AudioThumbnailCache& thumbnailCache;juce::AudioThumbnail sourceThumbnail,processedThumbnail;
    std::vector<EffectSettings> chain;juce::Array<ProcessingRecord> history,visibleHistory;RenderOperations operations;
    juce::File sourceFile,assetsDirectory,lastRendered;juce::String sourceId;juce::ThreadPool renderPool{1};juce::uint64 renderGeneration=0;double progressValue=0.0;
    ChainChanged onChainChanged;Rendered onRendered;Preview onPreview;OperationsChanged onOperationsChanged;
    juce::ComboBox effectChoice,chainChoice,historyChoice,filterMode;
    juce::TextButton addButton,removeButton,upButton,downButton,resetEffectButton,resetChainButton,renderButton,cancelButton,originalButton,processedButton,restoreButton;
    juce::ToggleButton enabledToggle;
    juce::ToggleButton normalizeToggle,reverseToggle,trimToggle,fadeInToggle,fadeOutToggle;
    juce::Slider params[5];juce::Label paramLabels[5],titleLabel,statusLabel;
    juce::ProgressBar progressBar{progressValue};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessingPanel)
};
