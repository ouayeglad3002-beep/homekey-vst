#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include "SampleBank.h"

//==============================================================================
// Les 5 mondes de HomeKey I
enum class PianoType { Dreaming = 0, Nebula, Cinema, Abyss, Backrooms, Count };

struct TypeProfile
{
    float tilt;          // pente spectrale de la couche harmonique (plus haut = plus sombre)
    float hammerPos;     // position du marteau (forme du spectre)
    float detuneCents;   // desaccord entre les 2 cordes
    float sustainMul;    // longueur du sustain
    float hfDamping;     // extinction des aigus
    float noise;         // bruit de feutre
    float noiseCutoff;
    float padAmount;     // nappe
    float padOctave;
    float padCutMul;
    float padAttack;
    float padSine;       // 0 = cordes, 1 = nappe douce
    float subAmount;
    float lpCutoff;      // filtre global de couleur (Hz)
    float chorusDepth;
    float reverbDamp;
    float gain;
};

const TypeProfile& getProfile (PianoType t);
juce::StringArray getTypeNames();

//==============================================================================
// Valeurs partagees entre le processeur et les voix
struct EngineParams
{
    std::atomic<int>   type     { 0 };
    std::atomic<float> tone     { 0.0f };
    std::atomic<float> velocity { 0.75f };
    std::atomic<float> release  { 0.5f };
    std::atomic<float> layer    { 0.5f };
    std::atomic<float> width    { 0.7f };
    std::atomic<int>   octave   { 0 };
    std::atomic<float> attack   { 0.002f };
    std::atomic<float> decayMul { 1.0f };
    std::atomic<float> hammer   { 1.0f };
    std::atomic<float> sub      { 0.0f };
    std::atomic<float> unison   { 1.0f };
    std::atomic<float> fine     { 0.0f };
    // couches
    std::atomic<float> piano    { 1.0f };   // echantillons Salamander
    std::atomic<float> harm     { 0.3f };   // couche harmonique (synthese)
    std::atomic<float> metal    { 0.0f };   // partiels de cloche / metal
    std::atomic<float> ring     { 0.0f };   // modulation en anneau
    std::atomic<int>   tex      { 0 };      // texture (0 = aucune)
    std::atomic<float> texLevel { 0.5f };
    std::atomic<float> variation{ 0.0f };   // variation aleatoire par note
    // modulations temps reel
    std::atomic<float> pitchMod { 0.0f };   // cents
};

//==============================================================================
class PianoSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class PianoVoice : public juce::SynthesiserVoice
{
public:
    PianoVoice (EngineParams& p, SampleBank& b) : params (p), bank (b) {}

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<PianoSound*> (s) != nullptr; }
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int value) override { bendSemis = (float) (value - 8192) / 8192.0f * 2.0f; }
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int startSample, int numSamples) override;

private:
    static constexpr int maxPartials = 24;
    static constexpr int numStrings  = 2;
    static constexpr int maxOsc      = maxPartials * numStrings;

    EngineParams& params;
    SampleBank& bank;
    juce::Random rng;

    // --- couche echantillons ---
    const SampleBank::Zone* zA = nullptr;
    const SampleBank::Zone* zB = nullptr;
    double posA = 1, posB = 1, incA = 0, incB = 0;
    float gainA = 0, gainB = 0;
    float sampleEnv = 1, sampleDecay = 1;
    float panSL = 1, panSR = 1;

    // --- texture ---
    const SampleBank::Zone* zT = nullptr;
    double posT = 1, incT = 0;
    float gainT = 0;

    // --- couche harmonique (phaseurs) ---
    int numOsc = 0;
    alignas (16) std::array<float, maxOsc> oc {}, os {}, cw {}, sw {}, amp {}, dec {}, baseW {};
    std::array<float, maxOsc> panL {}, panR {};
    float harmGain = 0;
    float lastPitchMul = 1.0f;

    // sub
    float subC = 1, subS = 0, subCw = 1, subSw = 0, subAmp = 0, subDec = 1, subW = 0;
    // bruit
    float noiseAmp = 0, noiseDec = 0, nLp1 = 0, nLp2 = 0, nHp = 0, noiseCoef = 0.1f, noiseHpCoef = 0.01f;
    // anneau
    double ringPhase = 0, ringInc = 0;
    // nappe
    std::array<double, 5> padPhase {};
    std::array<double, 5> padInc {};
    float padLevel = 0, padTarget = 0, padAtkCoef = 0, padRelCoef = 1, padSine = 0;
    float padLp1L = 0, padLp2L = 0, padLp1R = 0, padLp2R = 0, padCoef = 0.1f;
    float padPanL = 0.7f, padPanR = 0.7f;

    // enveloppes
    float attackRamp = 0, attackInc = 0;
    float releaseCoef = 1.0f;
    bool  releasing = false;
    float voiceGain = 0;
    int   renormCounter = 0;
    float bendSemis = 0, varCents = 0;
    double baseRatio = 1.0;

    static float polyBlep (double t, double dt);
    void setPitch (float mul);
};
