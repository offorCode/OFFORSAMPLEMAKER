#pragma once
#include <JuceHeader.h>

class OfforLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    OfforLookAndFeel();
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool)override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool)override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&)override;
    static void setIcon(juce::Button&, const juce::String& iconName);
    static void drawIcon(juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour);
};
