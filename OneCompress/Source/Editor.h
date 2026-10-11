#pragma once
#include <JuceHeader.h>
#include "Processor.h"

namespace OC
{
    inline juce::Colour styleColour (int s)
    {
        switch (s)
        {
            case 0:  return juce::Colour (0xff3ff6ff); // GLUE   cyan
            case 1:  return juce::Colour (0xffff8a3d); // PUNCH  orange
            case 2:  return juce::Colour (0xffb48cff); // VELVET violet
            case 3:  return juce::Colour (0xffff3a5c); // SMASH  rouge
            default: return juce::Colour (0xff7dffc4); // BLOOM  menthe
        }
    }
}

class OneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OneLookAndFeel();
    juce::Colour accent { 0xff3ff6ff };
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

class GrMeter : public juce::Component
{
public:
    explicit GrMeter (OneCompressProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    juce::Colour accent { 0xff3ff6ff };
    float shown = 0.0f;
private:
    OneCompressProcessor& proc;
};

class StyleBar : public juce::Component
{
public:
    explicit StyleBar (OneCompressProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
private:
    OneCompressProcessor& proc;
    int hover = -1;
    juce::Rectangle<float> cell (int i) const;
};

class OneCompressEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OneCompressEditor (OneCompressProcessor&);
    ~OneCompressEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    OneCompressProcessor& proc;
    OneLookAndFeel lnf;
    GrMeter meter;
    StyleBar styleBar;
    struct Knob { juce::Slider s; juce::Label l; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a; };
    std::array<Knob, 3> knobs;
    int lastStyle = -1;
    float inShown = -60, outShown = -60;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneCompressEditor)
};
