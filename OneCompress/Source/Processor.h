#pragma once
#include <JuceHeader.h>

//==============================================================================
// Les 5 caracteres de One Compress
struct Style
{
    const char* name;
    const char* hint;
    float thrAtMax;     // seuil (dB) quand SQUEEZE = 1
    float ratioLo, ratioHi;
    float attackMs, releaseMs;
    float kneeDb;
    bool  rms;          // detecteur RMS (doux) ou crete (rapide)
    float scHpfHz;      // filtre du detecteur : evite que les basses fassent pomper
    bool  programRelease; // relachement en 2 temps (style opto)
    bool  upward;       // compression vers le haut (BLOOM)
    // preset conseille
    float squeeze, color, mix;
};

const Style& getStyle (int i);
constexpr int numStyles = 5;

//==============================================================================
class OneCompressProcessor : public juce::AudioProcessor
{
public:
    OneCompressProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "One Compress"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return numStyles; }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override { return getStyle (index).name; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    void loadStylePreset (int style);   // choisit le style + ses 3 reglages conseilles

    // vumetres (lus par l'interface)
    std::atomic<float> grDb { 0.0f };       // reduction (negatif) ou gain (positif, BLOOM)
    std::atomic<float> inDb { -100.0f }, outDb { -100.0f };

private:
    int currentProgram = 0;
    double sr = 44100.0;

    // detecteur
    juce::dsp::IIR::Filter<float> scHpfL, scHpfR;
    float rmsState = 0.0f;
    float envDb = 0.0f;          // gain lisse (dB)
    float slowDb = 0.0f;         // memoire lente (VELVET)
    float avgGr = 0.0f;          // reduction moyenne (compensation auto)
    bool  resetMakeup = true;
    float powIn = 0.0f, powOut = 0.0f, matchGain = 1.0f;
    juce::SmoothedValue<float> squeezeS, colorS, mixS;

    // couleur
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 4096 };
    juce::AudioBuffer<float> dryBuf;
    std::array<float, 2> lowState {}, highState {}, hpState {}, dcX {}, dcY {};
    int lastStyle = -1;

    float computeGain (const Style&, float levelDb, float squeeze) const;
    void applyColor (juce::dsp::AudioBlock<float>& block, int style, float color);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneCompressProcessor)
};
