#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
namespace AlienColours
{
    const juce::Colour bg0     { 0xff030508 };
    const juce::Colour bg1     { 0xff0a0f15 };
    const juce::Colour panel   { 0xff10171e };
    const juce::Colour edge    { 0xff2a3642 };
    const juce::Colour steel   { 0xff8d9aa7 };
    const juce::Colour text    { 0xffdfeaf2 };
    const juce::Colour dim     { 0xff6c7d8b };
    const juce::Colour acid    { 0xff3ff6ff };

    inline juce::Colour forType (int t)
    {
        switch (t)
        {
            case 0:  return juce::Colour (0xffff8ae2); // DREAMING  : lilas-rose
            case 1:  return juce::Colour (0xff3ff6ff); // NEBULA    : cyan alien
            case 2:  return juce::Colour (0xff8fc8ff); // CINEMA    : bleu glacier
            case 3:  return juce::Colour (0xffff3a5c); // ABYSS     : cramoisi
            default: return juce::Colour (0xffe9e35a); // BACKROOMS : neon jaune
        }
    }
}

// formes "cristal" : rectangle aux coins coupes
juce::Path chamfer (juce::Rectangle<float> r, float cut);

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
    juce::Label* createSliderTextBox (juce::Slider&) override;
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
class ConstellationView : public juce::Component
{
public:
    explicit ConstellationView (HomeKeysProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::Colour accent = AlienColours::acid;
    float phase = 0.0f;
    int selected = 0;
    std::function<void (int)> onSelect;
private:
    HomeKeysProcessor& proc;
    int dragging = -1;
    juce::Point<float> starPos (int i) const;
    juce::Rectangle<float> field() const { return getLocalBounds().toFloat().reduced (16.0f); }
    void showStarMenu (int star);
};

//==============================================================================
class ChopPads : public juce::Component
{
public:
    explicit ChopPads (HomeKeysProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::Colour accent = AlienColours::acid;
private:
    HomeKeysProcessor& proc;
    juce::Rectangle<float> pad (int i) const;
};

//==============================================================================
class FilterView : public juce::Component
{
public:
    explicit FilterView (juce::AudioProcessorValueTreeState& s) : apvts (s) {}
    void paint (juce::Graphics&) override;
    juce::Colour accent = AlienColours::acid;
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
    void setPage (int p);

private:
    void timerCallback() override;
    void refreshPresetBox();
    void showSaveDialog();
    void showImportMenu();
    void showCredits();
    void selectStar (int i);
    void randomSound();

    HomeKeysProcessor& proc;
    AlienLookAndFeel lnf;

    TypeSelector typeSelector;
    AlienScope scope;
    GrainDisc grainDisc;
    ConstellationView constellation;
    ChopPads chopPads;
    FilterView filterView;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
        int page = 0;
    };
    std::vector<std::unique_ptr<Knob>> knobs;
    std::map<juce::String, Knob*> knobById;
    Knob& addKnob (const juce::String& id, const juce::String& title, int page, const juce::String& suffix = {});
    void placeRow (juce::Rectangle<int> area, const juce::StringArray& ids, int maxW = 104);

    std::array<juce::TextButton, 4> tabs;
    int page = 0;

    juce::TextButton randomBtn { "RANDOM SON" }, lockBtn { "LOCK" }, starsBtn { "RANDOM ETOILES" }, soundBtn { "RANDOM SON" };
    juce::TextButton chopOn { "CHOP OFF" }, creditBtn { "i" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chopAttach;

    juce::Label starLabel;
    juce::ComboBox starTarget, starShape;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttach, shapeAttach;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SAVE" }, delBtn { "DEL" }, importBtn { "IMPORT" }, folderBtn { "DOSSIER" };
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::AlertWindow> dialog;
    std::unique_ptr<juce::FileChooser> chooser;

    int lastType = -1;
    int lastPresetCount = -1;
    int refreshTick = 0;
    juce::String lastPresetName;
    float t = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeKeysEditor)
};
