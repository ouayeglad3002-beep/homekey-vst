#pragma once
#include <JuceHeader.h>
#include "Engine.h"

class SpiderProcessor : public juce::AudioProcessor
{
public:
    SpiderProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Spider"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return Web::numPresets; }
    int getCurrentProgram() override { return (int) apvts.getRawParameterValue ("preset")->load(); }
    void setCurrentProgram (int index) override { selectPreset (index); }
    const juce::String getProgramName (int index) override { return Web::preset (index).name; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void selectPreset (int index);
    juce::AudioProcessorValueTreeState apvts;
    Engine engine;

private:
    int64_t sampleCounter = 0;
    double internalPpq = 0.0;
    int lastPreset = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpiderProcessor)
};
