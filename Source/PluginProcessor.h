#pragma once
#include <JuceHeader.h>
#include "PianoVoice.h"
#include "PresetManager.h"

class HomeKeysProcessor : public juce::AudioProcessor
{
public:
    HomeKeysProcessor();
    ~HomeKeysProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HomeKeys"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    PresetManager presets;

    // Oscilloscope pour l'interface
    static constexpr int scopeSize = 1024;
    std::array<float, scopeSize> scope {};
    std::atomic<int> scopeWritePos { 0 };
    std::atomic<float> outputLevel { 0.0f };

private:
    EngineParams engine;
    juce::Synthesiser synth;
    juce::AudioBuffer<float> workBuffer;

    juce::dsp::StateVariableTPTFilter<float> toneFilter;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    // --- GRAIN ---
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wowDelay { 8192 };
    juce::SmoothedValue<float> wowAmt, driveAmt, crushAmt, vinylAmt;
    double wowPhase = 0, flutterPhase = 0;
    float wowDrift = 0, wowDriftTarget = 0;
    std::array<float, 2> holdValue {}, dcX {}, dcY {};
    float holdPhase = 1.0f;
    float hissLp = 0, crackleEnv = 0, crackleLp = 0, crackleSign = 1;
    juce::Random grainRng;
    void processGrain (juce::AudioBuffer<float>&, int n);
    void addVinyl (juce::AudioBuffer<float>&, int n);

    juce::SmoothedValue<float> volume;
    juce::SmoothedValue<float> cutoff;

    std::atomic<float>* pType = nullptr;
    std::atomic<float>* pVolume = nullptr;
    std::atomic<float>* pTone = nullptr;
    std::atomic<float>* pVelocity = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pLayer = nullptr;
    std::atomic<float>* pChorus = nullptr;
    std::atomic<float>* pReverb = nullptr;
    std::atomic<float>* pSize = nullptr;
    std::atomic<float>* pWidth = nullptr;
    std::atomic<float>* pOctave = nullptr;
    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pWow = nullptr;
    std::atomic<float>* pCrush = nullptr;
    std::atomic<float>* pVinyl = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HomeKeysProcessor)
};
