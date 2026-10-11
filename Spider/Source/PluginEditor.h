#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Dessin "venom" : fils noirs vernis, effiles, organiques
namespace Ink
{
    using Pts = std::vector<juce::Point<float>>;
    // polygone effile le long d'une ligne (epais au milieu, pointu aux bouts)
    juce::Path taper (const Pts& p, float wMax, float taperIn, float taperOut, float wMin);
    // trait noir + reflet de vernis
    void strand (juce::Graphics& g, const Pts& p, float wMax, float taperIn, float taperOut, float wMin, float alpha, float gloss = 0.45f);
    // goutte noire brillante
    void droplet (juce::Graphics& g, juce::Point<float> c, float r, float alpha);
    // aplatit un juce::Path en points
    Pts flatten (const juce::Path& p, const juce::AffineTransform& t);
}

//==============================================================================
class WebView : public juce::Component
{
public:
    explicit WebView (SpiderProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void tick (float dt);

    struct Note { Engine::Viz v; float age = 0.0f; };
    std::deque<Note> notes;
    float time = 0.0f;
private:
    SpiderProcessor& proc;
    float radiusFor (float midi, float centreMidi) const;
    void drawSpider (juce::Graphics&, juce::Point<float> at, float angle, float size);
    juce::Point<float> flow (juce::Point<float> p, float amp) const;
    std::vector<std::pair<juce::Point<float>, float>> attractors;
};

//==============================================================================
class SpiderEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpiderEditor (SpiderProcessor&);
    ~SpiderEditor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; }

    WebView web;
    void timerCallback() override;

private:
    juce::Rectangle<float> presetRow (int i) const;
    void drawLogo (juce::Graphics&, juce::Rectangle<float> area);
    SpiderProcessor& proc;
    int hover = -1;
    float t = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpiderEditor)
};
