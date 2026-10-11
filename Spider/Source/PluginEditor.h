#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

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
};

class SpiderEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpiderEditor (SpiderProcessor&);
    ~SpiderEditor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> presetRow (int i) const;
    SpiderProcessor& proc;
    WebView web;
    int hover = -1;
    std::array<float, Web::NumParams> shownVals {};
    float t = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpiderEditor)
};
