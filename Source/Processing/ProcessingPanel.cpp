#include "ProcessingPanel.h"

namespace{const juce::Colour bg{0xff14161a},panel{0xff1c1f25},border{0xff333740},accent{0xffea5910},text{0xfff3f3f3},muted{0xff9ca1aa};}

ProcessingPanel::ProcessingPanel()
{
    titleLabel.setText("BUILT-IN PROCESSING",juce::dontSendNotification);titleLabel.setColour(juce::Label::textColourId,text);titleLabel.setFont(juce::Font(20.0f,juce::Font::bold));addAndMakeVisible(titleLabel);
    for(int i=0;i<5;++i){paramLabels[i].setColour(juce::Label::textColourId,muted);addAndMakeVisible(paramLabels[i]);params[i].setSliderStyle(juce::Slider::LinearHorizontal);params[i].setTextBoxStyle(juce::Slider::TextBoxRight,false,64,22);params[i].setColour(juce::Slider::thumbColourId,accent);params[i].onValueChange=[this,i]{parameterChanged(i);};addAndMakeVisible(params[i]);}
    const juce::StringArray names{"Parametric EQ","Compressor","Saturation","Reverb","Delay","Filter","Gain","Limiter"};
    for(int i=0;i<names.size();++i)effectChoice.addItem(names[i],i+1);
    for(auto*b:{&addButton,&removeButton,&upButton,&downButton,&resetEffectButton,&resetChainButton,&renderButton,&cancelButton,&originalButton,&processedButton,&restoreButton}){addAndMakeVisible(*b);b->setColour(juce::TextButton::buttonColourId,panel);b->setColour(juce::TextButton::textColourOffId,text);}
    addButton.setButtonText("Add Effect");removeButton.setButtonText("Remove");upButton.setButtonText("Move Up");downButton.setButtonText("Move Down");resetEffectButton.setButtonText("Reset Effect");resetChainButton.setButtonText("Reset Chain");renderButton.setButtonText("Render Processed Copy");cancelButton.setButtonText("Cancel");originalButton.setButtonText("Play Original");processedButton.setButtonText("Play Processed");restoreButton.setButtonText("Restore Chain");
    addButton.onClick=[this]{EffectType type; if(EffectSettings::typeFromName(effectChoice.getText(),type)){chain.push_back(EffectSettings::defaults(type));refreshChain();if(onChainChanged)onChainChanged(chain);}};
    removeButton.onClick=[this]{const int i=chainChoice.getSelectedId()-1;if(i>=0&&i<(int)chain.size()){chain.erase(chain.begin()+i);refreshChain();if(onChainChanged)onChainChanged(chain);}};
    upButton.onClick=[this]{const int i=chainChoice.getSelectedId()-1;if(i>0){std::swap(chain[(size_t)i],chain[(size_t)i-1]);refreshChain();chainChoice.setSelectedId(i,juce::dontSendNotification);if(onChainChanged)onChainChanged(chain);}};
    downButton.onClick=[this]{const int i=chainChoice.getSelectedId()-1;if(i>=0&&i+1<(int)chain.size()){std::swap(chain[(size_t)i],chain[(size_t)i+1]);refreshChain();chainChoice.setSelectedId(i+2,juce::dontSendNotification);if(onChainChanged)onChainChanged(chain);}};
    resetEffectButton.onClick=[this]{const int i=chainChoice.getSelectedId()-1;if(i>=0&&i<(int)chain.size()){const auto id=chain[(size_t)i].id;chain[(size_t)i]=EffectSettings::defaults(chain[(size_t)i].type);chain[(size_t)i].id=id;refreshChain();chainChoice.setSelectedId(i+1,juce::dontSendNotification);selectChainItem();if(onChainChanged)onChainChanged(chain);}};
    resetChainButton.onClick=[this]{chain.clear();refreshChain();if(onChainChanged)onChainChanged(chain);};
    enabledToggle.setButtonText("Enabled");enabledToggle.onClick=[this]{const int i=chainChoice.getSelectedId()-1;if(i>=0&&i<(int)chain.size()){chain[(size_t)i].enabled=enabledToggle.getToggleState();if(onChainChanged)onChainChanged(chain);}};addAndMakeVisible(enabledToggle);
    filterMode.addItem("Low-pass",1);filterMode.addItem("High-pass",2);filterMode.onChange=[this]{const int i=chainChoice.getSelectedId()-1;if(i>=0&&i<(int)chain.size()){chain[(size_t)i].highPass=filterMode.getSelectedId()==2;if(onChainChanged)onChainChanged(chain);}};addAndMakeVisible(filterMode);
    const std::array<std::pair<juce::ToggleButton*,juce::String>,5> operationButtons{{{&normalizeToggle,"Normalize"},{&reverseToggle,"Reverse"},{&trimToggle,"Trim Silence"},{&fadeInToggle,"10 ms Fade In"},{&fadeOutToggle,"10 ms Fade Out"}}};
    for(const auto& entry:operationButtons){auto* b=entry.first;b->setButtonText(entry.second);b->onClick=[this]{operations.normalize=normalizeToggle.getToggleState();operations.reverse=reverseToggle.getToggleState();operations.trimSilence=trimToggle.getToggleState();operations.fadeInMs=fadeInToggle.getToggleState()?10.0:0.0;operations.fadeOutMs=fadeOutToggle.getToggleState()?10.0:0.0;if(onOperationsChanged)onOperationsChanged(operations);};addAndMakeVisible(*b);}
    chainChoice.onChange=[this]{selectChainItem();};addAndMakeVisible(chainChoice);addAndMakeVisible(effectChoice);effectChoice.setSelectedId(1);
    historyChoice.onChange=[this]{restoreButton.setEnabled(historyChoice.getSelectedId()>0);};addAndMakeVisible(historyChoice);restoreButton.setEnabled(false);restoreButton.onClick=[this]{const int i=historyChoice.getSelectedId()-1;if(i>=0&&i<history.size()){chain=history.getReference(i).effects;setOperations(history.getReference(i).operations);refreshChain();if(onChainChanged)onChainChanged(chain);if(onOperationsChanged)onOperationsChanged(operations);}};
    renderButton.onClick=[this]{render();};cancelButton.onClick=[this]{cancelRender();};originalButton.onClick=[this]{if(sourceFile.existsAsFile()&&onPreview)onPreview(sourceFile);};processedButton.onClick=[this]{if(lastRendered.existsAsFile()&&onPreview)onPreview(lastRendered);};
    addAndMakeVisible(progressBar);progressBar.setColour(juce::ProgressBar::foregroundColourId,accent);statusLabel.setColour(juce::Label::textColourId,muted);addAndMakeVisible(statusLabel);
    refreshChain();
}

ProcessingPanel::~ProcessingPanel(){++renderGeneration;renderPool.removeAllJobs(true,5000);}
void ProcessingPanel::setCallbacks(ChainChanged c,Rendered r,Preview p,OperationsChanged o){onChainChanged=std::move(c);onRendered=std::move(r);onPreview=std::move(p);onOperationsChanged=std::move(o);}
void ProcessingPanel::setSource(const juce::File& f,const juce::String& id,const juce::File& assets){sourceFile=f;sourceId=id;assetsDirectory=assets;renderButton.setEnabled(f.existsAsFile()&&assets.isDirectory());originalButton.setEnabled(f.existsAsFile());statusLabel.setText(f.existsAsFile()?"Original ready. Renders are saved as new project assets.":"Select an available project sample to process.",juce::dontSendNotification);}
void ProcessingPanel::setChain(const std::vector<EffectSettings>& s){chain=s;refreshChain();}
std::vector<EffectSettings> ProcessingPanel::getChain()const{return chain;}
void ProcessingPanel::setHistory(const juce::Array<ProcessingRecord>& h){history=h;historyChoice.clear();for(int i=0;i<h.size();++i)historyChoice.addItem(juce::File(h.getReference(i).renderedPath).getFileName()+"  •  "+h.getReference(i).timestamp,i+1);if(!h.isEmpty()){const auto& latest=h.getReference(h.size()-1);lastRendered=latest.renderedPath;historyChoice.setSelectedId(h.size(),juce::dontSendNotification);}restoreButton.setEnabled(false);processedButton.setEnabled(lastRendered.existsAsFile());}
void ProcessingPanel::setOperations(const RenderOperations& o){operations=o;normalizeToggle.setToggleState(o.normalize,juce::dontSendNotification);reverseToggle.setToggleState(o.reverse,juce::dontSendNotification);trimToggle.setToggleState(o.trimSilence,juce::dontSendNotification);fadeInToggle.setToggleState(o.fadeInMs>0,juce::dontSendNotification);fadeOutToggle.setToggleState(o.fadeOutMs>0,juce::dontSendNotification);}

void ProcessingPanel::refreshChain(){const int old=chainChoice.getSelectedId();chainChoice.clear();for(size_t i=0;i<chain.size();++i)chainChoice.addItem(juce::String((int)i+1)+". "+chain[i].typeName()+(chain[i].enabled?"":" (bypassed)"),(int)i+1);if(!chain.empty())chainChoice.setSelectedId(juce::jlimit(1,(int)chain.size(),old>0?old:1),juce::dontSendNotification);else chainChoice.setSelectedId(0,juce::dontSendNotification);selectChainItem();}

void ProcessingPanel::selectChainItem(){const int i=chainChoice.getSelectedId()-1;const bool valid=i>=0&&i<(int)chain.size();enabledToggle.setEnabled(valid);removeButton.setEnabled(valid);upButton.setEnabled(valid&&i>0);downButton.setEnabled(valid&&i+1<(int)chain.size());resetEffectButton.setEnabled(valid);for(auto& p:params)p.setEnabled(valid);if(!valid)return;enabledToggle.setToggleState(chain[(size_t)i].enabled,juce::dontSendNotification);updateParameterLabels();}

void ProcessingPanel::updateParameterLabels()
{
    const int i=chainChoice.getSelectedId()-1;if(i<0||i>=(int)chain.size())return;const auto&s=chain[(size_t)i];juce::StringArray labels;std::array<double,5> values{},mins{},maxs{};
    switch(s.type){case EffectType::parametricEq:labels={"Frequency (Hz)","Gain (dB)","Q","Wet (%)",""};values={(double)s.frequency,s.gainDb,s.q,s.wet*100.0,0};mins={20,-18,.1,0,0};maxs={20000,18,18,100,1};break;
    case EffectType::compressor:labels={"Threshold (dB)","Ratio","Attack (ms)","Release (ms)","Wet (%)"};values={s.thresholdDb,s.ratio,s.attackMs,s.releaseMs,s.wet*100.0};mins={-60,1,.1,1,0};maxs={0,20,200,2000,100};break;
    case EffectType::saturation:labels={"Drive (dB)","Tone (Hz)","Output (dB)","Wet (%)",""};values={s.driveDb,s.toneHz,s.levelDb,s.wet*100.0,0};mins={0,100,-24,0,0};maxs={36,20000,12,100,1};break;
    case EffectType::reverb:labels={"Room size","Damping","Width","Wet (%)",""};values={s.roomSize,s.damping,s.width,s.wet*100.0,0};mins={0,0,0,0,0};maxs={1,1,1,100,1};break;
    case EffectType::delay:labels={"Delay (ms)","Feedback","Output (dB)","Wet (%)",""};values={s.delayMs,s.feedback,s.levelDb,s.wet*100.0,0};mins={1,0,-24,0,0};maxs={2000,.95,12,100,1};break;
    case EffectType::filter:labels={"Cutoff (Hz)","Resonance (Q)","Wet (%)","",""};values={s.cutoffHz,s.q,s.wet*100.0,0,0};mins={20,.1,0,0,0};maxs={20000,18,100,1,1};break;
    case EffectType::gain:labels={"Gain (dB)","Wet (%)","","",""};values={s.levelDb,s.wet*100.0,0,0,0};mins={-60,0,0,0,0};maxs={24,100,1,1,1};break;
    case EffectType::limiter:labels={"Ceiling (dB)","Release (ms)","Wet (%)","",""};values={s.ceilingDb,s.limiterReleaseMs,s.wet*100.0,0,0};mins={-24,1,0,0,0};maxs={0,1000,100,1,1};break;}
    for(int p=0;p<5;++p){paramLabels[p].setText(labels[p],juce::dontSendNotification);const bool show=labels[p].isNotEmpty();paramLabels[p].setVisible(show);params[p].setVisible(show);params[p].setRange(mins[(size_t)p],maxs[(size_t)p],p==0&&s.type==EffectType::parametricEq?1.0:0.01);params[p].setValue(values[(size_t)p],juce::dontSendNotification);}
    filterMode.setVisible(s.type==EffectType::filter);filterMode.setSelectedId(s.highPass?2:1,juce::dontSendNotification);
}

void ProcessingPanel::parameterChanged(int p)
{
    const int i=chainChoice.getSelectedId()-1;if(i<0||i>=(int)chain.size())return;auto&s=chain[(size_t)i];const float v=(float)params[p].getValue();
    switch(s.type){case EffectType::parametricEq:if(p==0)s.frequency=v;else if(p==1)s.gainDb=v;else if(p==2)s.q=v;else if(p==3)s.wet=v/100;break;
    case EffectType::compressor:if(p==0)s.thresholdDb=v;else if(p==1)s.ratio=v;else if(p==2)s.attackMs=v;else if(p==3)s.releaseMs=v;else s.wet=v/100;break;
    case EffectType::saturation:if(p==0)s.driveDb=v;else if(p==1)s.toneHz=v;else if(p==2)s.levelDb=v;else if(p==3)s.wet=v/100;break;
    case EffectType::reverb:if(p==0)s.roomSize=v;else if(p==1)s.damping=v;else if(p==2)s.width=v;else if(p==3)s.wet=v/100;break;
    case EffectType::delay:if(p==0)s.delayMs=v;else if(p==1)s.feedback=v;else if(p==2)s.levelDb=v;else if(p==3)s.wet=v/100;break;
    case EffectType::filter:if(p==0)s.cutoffHz=v;else if(p==1)s.q=v;else if(p==2)s.wet=v/100;break;
    case EffectType::gain:if(p==0)s.levelDb=v;else if(p==1)s.wet=v/100;break;
    case EffectType::limiter:if(p==0)s.ceilingDb=v;else if(p==1)s.limiterReleaseMs=v;else if(p==2)s.wet=v/100;break;}
    if(onChainChanged)onChainChanged(chain);
}

void ProcessingPanel::render()
{
    if(!sourceFile.existsAsFile()||!assetsDirectory.isDirectory()){statusLabel.setText("Open a project and select a source sample first.",juce::dontSendNotification);return;}
    ++renderGeneration;const auto generation=renderGeneration;progressValue=0;renderButton.setEnabled(false);cancelButton.setEnabled(true);statusLabel.setText("Rendering a new processed copy…",juce::dontSendNotification);
    auto dir=assetsDirectory.getChildFile("Processed");auto dest=dir.getChildFile(juce::File::createLegalFileName(sourceId)+"_"+juce::Uuid().toString()+".wav");juce::Component::SafePointer<ProcessingPanel>safe(this);
    auto*job=new AudioRenderJob(sourceFile,dest,sourceId,chain,[safe,generation](juce::File file,const juce::String&error){if(!safe||safe->renderGeneration!=generation)return;safe->renderButton.setEnabled(safe->sourceFile.existsAsFile());safe->cancelButton.setEnabled(false);if(file.existsAsFile()){safe->lastRendered=file;ProcessingRecord r;r.sampleId=safe->sourceId;r.originalPath=safe->sourceFile.getFullPathName();r.renderedPath=file.getFullPathName();r.effects=safe->chain;r.operations=safe->operations;r.timestamp=juce::Time::getCurrentTime().toISO8601(true);if(safe->onRendered)safe->onRendered(r);safe->processedButton.setEnabled(true);safe->statusLabel.setText("Rendered: "+file.getFileName(),juce::dontSendNotification);}else safe->statusLabel.setText(error,juce::dontSendNotification);},[safe,generation](double p){if(safe&&safe->renderGeneration==generation){safe->progressValue=p;safe->repaint();}},operations);
    renderPool.addJob(job,true);
}
void ProcessingPanel::cancelRender(){if(auto*job=renderPool.getJob(0)){++renderGeneration;job->signalJobShouldExit();renderButton.setEnabled(sourceFile.existsAsFile());cancelButton.setEnabled(false);statusLabel.setText("Cancelling render…",juce::dontSendNotification);}}

void ProcessingPanel::paint(juce::Graphics&g){g.fillAll(bg);auto box=getLocalBounds().reduced(12);g.setColour(panel);g.fillRoundedRectangle(box.toFloat(),10);g.setColour(border);g.drawRoundedRectangle(box.toFloat(),10,1);g.setColour(text);g.setFont(14);g.drawText("EFFECT CHAIN",20,58,240,20,juce::Justification::left);g.drawText("PARAMETERS",20,178,240,20,juce::Justification::left);g.drawText("AUDIO OPERATIONS",20,412,240,20,juce::Justification::left);g.drawText("RENDER HISTORY",20,getHeight()-92,getWidth()-40,20,juce::Justification::left);}

void ProcessingPanel::resized()
{
    auto r=getLocalBounds().reduced(20);titleLabel.setBounds(r.removeFromTop(34));r.removeFromTop(10);
    effectChoice.setBounds(r.removeFromTop(30).removeFromLeft(190));addButton.setBounds(220,52,110,32);chainChoice.setBounds(20,88,310,30);removeButton.setBounds(345,88,88,30);upButton.setBounds(440,88,82,30);downButton.setBounds(529,88,100,30);enabledToggle.setBounds(640,88,100,30);resetEffectButton.setBounds(750,88,120,30);resetChainButton.setBounds(879,88,110,30);
    filterMode.setBounds(20,129,160,28);
    for(int i=0;i<5;++i){const int y=177+i*47;paramLabels[i].setBounds(20,y,150,22);params[i].setBounds(170,y,getWidth()-210,28);}
    const std::array<juce::ToggleButton*,5> operationButtons{{&normalizeToggle,&reverseToggle,&trimToggle,&fadeInToggle,&fadeOutToggle}};
    for(int i=0;i<5;++i)operationButtons[(size_t)i]->setBounds(20+i*(getWidth()-40)/5,438,(getWidth()-40)/5-4,28);
    renderButton.setBounds(20,475,220,36);cancelButton.setBounds(248,475,100,36);originalButton.setBounds(360,475,130,36);processedButton.setBounds(500,475,140,36);progressBar.setBounds(20,520,getWidth()-40,22);statusLabel.setBounds(20,545,getWidth()-40,24);
    historyChoice.setBounds(20,getHeight()-54,getWidth()-190,30);restoreButton.setBounds(getWidth()-155,getHeight()-54,130,30);
}


