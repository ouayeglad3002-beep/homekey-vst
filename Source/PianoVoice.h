#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

//==============================================================================
// Les 5 varietes de piano HomeKeys
enum class PianoType { Doux = 0, Grave, Orchestre, Cinematique, Dreaming, Count };

// Caractere sonore de chaque variete
struct TypeProfile
{
    float tilt;          // pente spectrale (plus haut = plus sombre)
    float hammerPos;     // position du marteau sur la corde (forme du spectre)
    float detuneCents;   // desaccord entre les 2 cordes (battements)
    float sustainMul;    // longueur du sustain
    float hfDamping;     // vitesse a laquelle les aigus s'eteignent
    float noise;         // bruit du marteau / feutre
    float noiseCutoff;   // couleur du bruit (Hz)
    float padAmount;     // nappe (cordes / pad)
    float padOctave;     // octave de la nappe
    float padCutMul;     // brillance de la nappe
    float padAttack;     // attaque de la nappe (s)
    float padSine;       // 0 = cordes (scie), 1 = nappe douce (sinus)
    float subAmount;     // sub-basse
    float lpCutoff;      // filtre global (Hz)
    float chorusDepth;   // profondeur chorus
    float reverbDamp;    // amortissement de la reverb
    float gain;          // compensation de volume
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
    std::atomic<float> attack   { 0.002f }; // s
    std::atomic<float> decayMul { 1.0f };
    std::atomic<float> hammer   { 1.0f };
    std::atomic<float> sub      { 0.0f };
    std::atomic<float> unison   { 1.0f };
    std::atomic<float> fine     { 0.0f };   // cents
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
    explicit PianoVoice (EngineParams& p) : params (p) {}

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<PianoSound*> (s) != nullptr; }
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int startSample, int numSamples) override;

private:
    static constexpr int maxPartials = 32;
    static constexpr int numStrings  = 2;
    static constexpr int maxOsc      = maxPartials * numStrings;

    EngineParams& params;
    juce::Random rng;

    // Oscillateurs (rotation de phaseur = sinus tres peu couteux)
    int numOsc = 0;
    alignas (16) std::array<float, maxOsc> oc {}, os {}, cw {}, sw {}, amp {}, dec {};
    std::array<float, maxOsc> panL {}, panR {};

    // Sub-basse
    float subC = 1, subS = 0, subCw = 1, subSw = 0, subAmp = 0, subDec = 1;

    // Bruit du marteau
    float noiseAmp = 0, noiseDec = 0, nLp1 = 0, nLp2 = 0, nHp = 0, noiseCoef = 0.1f, noiseHpCoef = 0.01f;

    // Nappe (cordes / pad)
    std::array<double, 5> padPhase {};
    std::array<double, 5> padInc {};
    float padLevel = 0, padTarget = 0, padAtkCoef = 0, padRelCoef = 1, padSine = 0;
    float padLp1L = 0, padLp2L = 0, padLp1R = 0, padLp2R = 0, padCoef = 0.1f;
    float padPanL = 0.7f, padPanR = 0.7f;

    // Enveloppes
    float attackRamp = 0, attackInc = 0;
    float releaseCoef = 1.0f;
    bool  releasing = false;
    float voiceGain = 0;
    int   renormCounter = 0;

    static float polyBlep (double t, double dt);
};
