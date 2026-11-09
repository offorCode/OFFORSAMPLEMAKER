#include "OfforLookAndFeel.h"
#include <map>
namespace
{
const juce::Colour control{0xff262a33},border{0xff343943},accent{0xffea5910};
void line(juce::Path& p,float x1,float y1,float x2,float y2){p.startNewSubPath(x1,y1);p.lineTo(x2,y2);}
void drawIcon(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, juce::Colour colour)
{
    const float s=juce::jmin(r.getWidth(),r.getHeight()), x=r.getCentreX()-s*.5f, y=r.getCentreY()-s*.5f;
    juce::Path p; const float a=s*.22f,b=s*.78f,c=s*.5f;
    if(name=="home"){p.startNewSubPath(x+a,y+c);p.lineTo(x+c,y+a);p.lineTo(x+b,y+c);p.lineTo(x+b,y+b);p.lineTo(x+c+s*.06f,y+b);p.lineTo(x+c+s*.06f,y+c+s*.03f);p.lineTo(x+c-s*.06f,y+c+s*.03f);p.lineTo(x+c-s*.06f,y+b);p.lineTo(x+a,y+b);p.closeSubPath();}
    else if(name=="library"){p.addRoundedRectangle(x+a,y+a,s*.20f,s*.60f,2);p.addRoundedRectangle(x+c-s*.10f,y+a,s*.20f,s*.60f,2);p.addRoundedRectangle(x+b-s*.20f,y+a,s*.20f,s*.60f,2);}
    else if(name=="processing"){for(int i=0;i<3;++i){const float yy=y+a+i*s*.22f;line(p,x+a,yy,x+b,yy);p.addEllipse(x+(i==1?c-s*.09f:a+s*.08f),yy-s*.07f,s*.14f,s*.14f);}}
    else if(name=="variations"){p.addEllipse(x+a,y+a,s*.17f,s*.17f);p.addEllipse(x+b-s*.17f,y+a,s*.17f,s*.17f);p.addEllipse(x+c-s*.085f,y+b-s*.17f,s*.17f,s*.17f);line(p,x+a+s*.17f,y+a+s*.08f,x+b-s*.17f,y+a+s*.08f);line(p,x+a+s*.1f,y+a+s*.17f,x+c,y+b-s*.17f);line(p,x+b-s*.1f,y+a+s*.17f,x+c,y+b-s*.17f);}
    else if(name=="plugins"){p.addRoundedRectangle(x+a,y+c-s*.12f,s*.56f,s*.30f,2);line(p,x+c,y+c-s*.12f,x+c,y+a);line(p,x+c-s*.14f,y+a,x+c+s*.14f,y+a);line(p,x+a,y+c+s*.04f,x+a-s*.08f,y+c+s*.04f);line(p,x+b,y+c+s*.04f,x+b+s*.08f,y+c+s*.04f);}
    else if(name=="export"||name=="download"){line(p,x+c,y+a,x+c,y+c+s*.05f);line(p,x+c-s*.18f,y+c-s*.02f,x+c,y+c+s*.16f);line(p,x+c+s*.18f,y+c-s*.02f,x+c,y+c+s*.16f);line(p,x+a,y+c+s*.19f,x+a,y+b);line(p,x+a,y+b,x+b,y+b);line(p,x+b,y+b,x+b,y+c+s*.19f);}
    else if(name=="import"){line(p,x+c,y+b,x+c,y+a);line(p,x+c-s*.18f,y+a+s*.18f,x+c,y+a);line(p,x+c+s*.18f,y+a+s*.18f,x+c,y+a);line(p,x+a,y+c+s*.10f,x+a,y+b);line(p,x+a,y+b,x+b,y+b);line(p,x+b,y+b,x+b,y+c+s*.10f);}
    else if(name=="settings"){p.addEllipse(x+a,y+a,s*.56f,s*.56f);p.addEllipse(x+c-s*.10f,y+c-s*.10f,s*.20f,s*.20f);for(int i=0;i<8;++i){const float ang=i*juce::MathConstants<float>::twoPi/8;line(p,x+c+std::cos(ang)*s*.30f,y+c+std::sin(ang)*s*.30f,x+c+std::cos(ang)*s*.39f,y+c+std::sin(ang)*s*.39f);}}
    else if(name=="open"){line(p,x+a,y+c,x+a,y+b);line(p,x+a,y+b,x+b,y+b);line(p,x+b,y+b,x+b,y+c+s*.07f);line(p,x+a,y+c,x+a,y+a+s*.10f);line(p,x+a,y+a+s*.10f,x+c-s*.03f,y+a+s*.10f);line(p,x+c-s*.03f,y+a+s*.10f,x+c+s*.10f,y+c-s*.02f);line(p,x+c+s*.10f,y+c-s*.02f,x+b,y+c-s*.02f);}
    else if(name=="save"){p.addRoundedRectangle(x+a,y+a,s*.56f,s*.56f,2);p.addRectangle(x+a+s*.12f,y+a,s*.32f,s*.18f);p.addRectangle(x+a+s*.12f,y+c+s*.06f,s*.34f,s*.18f);}
    else if(name=="play"){p.startNewSubPath(x+a,y+a);p.lineTo(x+b,y+c);p.lineTo(x+a,y+b);p.closeSubPath();}
    else if(name=="stop"){p.addRoundedRectangle(x+a,y+a,s*.56f,s*.56f,2);}
    else if(name=="pause"){p.addRoundedRectangle(x+a,y+a,s*.16f,s*.56f,2);p.addRoundedRectangle(x+b-s*.16f,y+a,s*.16f,s*.56f,2);}
    else if(name=="search"){p.addEllipse(x+a,y+a,s*.43f,s*.43f);line(p,x+c+s*.08f,y+c+s*.08f,x+b,y+b);}
    else if(name=="delete"){p.addRoundedRectangle(x+a+s*.05f,y+c-s*.08f,s*.46f,s*.42f,2);line(p,x+a,y+c-s*.15f,x+b,y+c-s*.15f);line(p,x+c-s*.10f,y+a+s*.08f,x+c+s*.10f,y+a+s*.08f);line(p,x+c-s*.10f,y+c,x+c-s*.10f,y+b-s*.06f);line(p,x+c+s*.10f,y+c,x+c+s*.10f,y+b-s*.06f);}
    else if(name=="favorite"){p.startNewSubPath(x+c,y+b);p.cubicTo(x+a,y+c+s*.05f,x+a,y+a,x+c,y+c-s*.05f);p.cubicTo(x+b,y+a,x+b,y+c+s*.05f,x+c,y+b);p.closeSubPath();}
    else if(name=="lock"||name=="unlock"){p.addRoundedRectangle(x+a,y+c-s*.02f,s*.56f,s*.40f,2);p.startNewSubPath(x+a+s*.12f,y+c-s*.02f);if(name=="unlock"){p.lineTo(x+a+s*.12f,y+a+s*.10f);p.lineTo(x+c,y+a+s*.02f);}else{p.lineTo(x+a+s*.12f,y+a+s*.10f);p.cubicTo(x+a+s*.12f,y+a-s*.05f,x+b-s*.12f,y+a-s*.05f,x+b-s*.12f,y+a+s*.10f);} }
    else if(name=="regenerate"){p.addArc(x+a,y+a,s*.56f,s*.56f,-.7f,4.8f);line(p,x+b-s*.06f,y+a+s*.04f,x+b,y+a+s*.22f);line(p,x+b,y+a+s*.22f,x+b-s*.18f,y+a+s*.25f);}
    else if(name=="compare"){line(p,x+a,y+a+s*.12f,x+b,y+a+s*.12f);line(p,x+b-s*.16f,y+a,x+b,y+a+s*.12f);line(p,x+b-s*.16f,y+a+s*.24f,x+b,y+a+s*.12f);line(p,x+b,y+b-s*.12f,x+a,y+b-s*.12f);line(p,x+a,y+b-s*.12f,x+a+s*.16f,y+b-s*.24f);line(p,x+a,y+b-s*.12f,x+a+s*.16f,y+b);}
    if(name=="play")g.setColour(colour);else{g.setColour(colour);g.strokePath(p,juce::PathStrokeType(1.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
    if(name=="play")g.fillPath(p);
}
}
void OfforLookAndFeel::drawIcon(juce::Graphics&g,juce::Rectangle<float>r,const juce::String&name,juce::Colour colour){::drawIcon(g,r,name,colour);}
OfforLookAndFeel::OfforLookAndFeel(){setColour(juce::Label::textColourId,juce::Colour(0xfff3f3f5));setColour(juce::TextEditor::backgroundColourId,control);setColour(juce::TextEditor::textColourId,juce::Colour(0xfff3f3f5));setColour(juce::TextEditor::outlineColourId,border);setColour(juce::ComboBox::backgroundColourId,control);setColour(juce::ComboBox::textColourId,juce::Colour(0xfff3f3f5));setColour(juce::ComboBox::outlineColourId,border);setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff1d2027));setColour(juce::PopupMenu::textColourId,juce::Colour(0xfff3f3f5));}
void OfforLookAndFeel::drawButtonBackground(juce::Graphics&g,juce::Button&b,const juce::Colour&,bool over,bool down){auto r=b.getLocalBounds().toFloat().reduced(.5f);auto c=b.findColour(b.getToggleState()?juce::TextButton::buttonOnColourId:juce::TextButton::buttonColourId);if(!b.isEnabled())c=c.withAlpha(.4f);else if(down)c=c.brighter(.14f);else if(over)c=c.brighter(.07f);g.setColour(c);g.fillRoundedRectangle(r,7.0f);if(!b.getToggleState()&&c.getBrightness()>.35f){g.setColour(border.withAlpha(.7f));g.drawRoundedRectangle(r,7.0f,1.0f);}if(b.getToggleState()){g.setColour(accent);g.fillRoundedRectangle(0.0f,4.0f,3.0f,juce::jmax(4.0f,r.getHeight()-8.0f),1.5f);}}
void OfforLookAndFeel::setIcon(juce::Button&b,const juce::String&name)
{
    static const std::map<juce::String,juce::String> help{{"home","Go to the project dashboard"},{"library","Browse, search and preview imported samples"},{"processing","Open the offline audio processing workspace"},{"variations","Generate and manage sample variations"},{"plugins","Scan installed VST3 plugins"},{"export","Export selected audio files"},{"settings","Open application preferences"},{"open","Open an existing project"},{"save","Save project changes"},{"play","Play this audio"},{"stop","Stop preview playback"},{"import","Import audio from a folder"},{"delete","Delete the selected item"},{"favorite","Toggle favorite status"},{"lock","Protect this variation from regeneration"},{"unlock","Allow this variation to be regenerated"},{"regenerate","Generate a new audio result"},{"compare","Compare the original and processed audio"},{"download","Render or export audio"},{"search","Search the current list"}};
    b.getProperties().set("offorIcon",name);b.setTooltip(help.count(name)?help.at(name):b.getButtonText());
}
void OfforLookAndFeel::drawButtonText(juce::Graphics&g,juce::TextButton&b,bool over,bool down){const auto name=b.getProperties().getWithDefault("offorIcon",{}).toString();if(name.isEmpty()){juce::LookAndFeel_V4::drawButtonText(g,b,over,down);return;}auto r=b.getLocalBounds().toFloat().reduced(6,0);auto icon=r.removeFromLeft(20).reduced(3);const auto colour=b.findColour(b.getToggleState()?juce::TextButton::textColourOnId:juce::TextButton::textColourOffId).withAlpha(b.isEnabled()?1.0f:.45f);drawIcon(g,icon,name,colour);g.setColour(colour);g.setFont(getTextButtonFont(b,b.getHeight()));g.drawFittedText(b.getButtonText(),r.toNearestInt(),juce::Justification::centredLeft,1);}
void OfforLookAndFeel::drawComboBox(juce::Graphics&g,int w,int h,bool over,int,int,int,int,juce::ComboBox&box){auto r=juce::Rectangle<float>(0,0,(float)w,(float)h).reduced(.5f);g.setColour(box.findColour(juce::ComboBox::backgroundColourId));g.fillRoundedRectangle(r,6);g.setColour(over?accent:box.findColour(juce::ComboBox::outlineColourId));g.drawRoundedRectangle(r,6,1);g.setColour(juce::Colour(0xffaab0bb));juce::Path p;p.addTriangle((float)w-18,(float)h*.42f,(float)w-9,(float)h*.42f,(float)w-13.5f,(float)h*.62f);g.fillPath(p);}
