#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
namespace AlienColours
{
    const juce::Colour bg0     { 0xff05070a };
    const juce::Colour bg1     { 0xff0b1016 };
    const juce::Colour panel   { 0xff0e141b };
    const juce::Colour edge    { 0xff1c2a33 };
    const juce::Colour text    { 0xffcfe9de };
    const juce::Colour dim     { 0xff5f7a73 };
    const juce::Colour acid    { 0xff39ff8f };

    inline juce::Colour forType (int t)
    {
        switch (t)
        {
            case 0:  return juce::Colour (0xff8affc1); // Doux       : menthe
            case 1:  return juce::Colour (0xffa46bff); // Grave      : violet
            case 2:  return juce::Colour (0xff29e6ff); // Orchestre  : cyan
            case 3:  return juce::Colour (0xffc6ff3d); // Cinematique: lime toxique
            default: return juce::Colour (0xffff4fd8); // Dreaming   : magenta
        }
    }
}

//==============================================================================
class AlienLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AlienLookAndFeel();
    juce::Colour accent = AlienColours::acid;

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
};

//==============================================================================
class TypeSelector : public juce::Component
{
public:
    explicit TypeSelector (juce::AudioProcessorValueTreeState& s) : apvts (s) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    float phase = 0.0f;
private:
    juce::AudioProcessorValueTreeState& apvts;
    int hover = -1;
    juce::Rectangle<float> podBounds (int i) const;
};

//==============================================================================
class AlienScope : public juce::Component
{
public:
    explicit AlienScope (HomeKeysProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    juce::Colour accent = AlienColours::acid;
    float phase = 0.0f;
private:
    HomeKeysProcessor& proc;
};

//==============================================================================
class GrainDisc : public juce::Component
{
public:
    explicit GrainDisc (juce::AudioProcessorValueTreeState& s) : apvts (s) {}
    void paint (juce::Graphics&) override;
    juce::Colour accent = AlienColours::acid;
    float angle = 0.0f;
private:
    juce::AudioProcessorValueTreeState& apvts;
};

//==============================================================================
class HomeKeysEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HomeKeysEditor (HomeKeysProcessor&);
    ~HomeKeysEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPresetBox();
    void showSaveDialog();

    HomeKeysProcessor& proc;
    AlienLookAndFeel lnf;

    TypeSelector typeSelector;
    AlienScope scope;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    std::array<Knob, 14> knobs;
    GrainDisc grainDisc;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SAVE" }, delBtn { "DEL" }, folderBtn { "DOSSIER" };
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::AlertWindow> dialog;

    int lastType = -1;
    int lastPresetCount = -1;
    juce::String lastPresetName;
    float t = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeKeysEditor)
};
