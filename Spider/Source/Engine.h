#pragma once
#include <JuceHeader.h>
#include "MusicTheory.h"
#include "Analyzer.h"

//==============================================================================
// Les 15 fils de la toile
namespace Web
{
    enum P { Variation, Expand, Mirror, Octave, Harmony1, Harmony2, Arpeggio, Ornament, Glide,
             Stutter, Reverse, Echo, Light, Space, Accent, NumParams };

    inline const char* paramName (int i)
    {
        static const char* n[NumParams] = { "VARIATION", "OUVERTURE", "MIROIR", "OCTAVES", "HARMONIE I", "HARMONIE II",
                                            "ARPEGE", "ORNEMENTS", "GLISSES", "STUTTER", "REVERSE", "ECHO SPIRALE",
                                            "LUMIERE", "ESPACE", "ACCENTS" };
        return n[i];
    }

    struct Preset
    {
        const char* name;
        const char* desc;
        float p[NumParams];   // Expand : 0..1 -> x0.5 .. x2 ; Light : 0..1 -> sombre..lumineux (0.5 = neutre)
        float grit;           // saturation cachee
        int h1Type, h2Type;   // types d'harmonie
        bool evolve;          // la toile change a chaque cycle (infini)
    };

    constexpr int numPresets = 10;
    const Preset& preset (int i);
}

//==============================================================================
// Tampon circulaire stereo indexe en absolu
struct Ring
{
    std::vector<float> l, r;
    int64_t mask = 0, w = 0;
    void init (int sizePow2) { l.assign ((size_t) sizePow2, 0.0f); r.assign ((size_t) sizePow2, 0.0f); mask = sizePow2 - 1; w = 0; }
    inline void push (float a, float b) { l[(size_t) (w & mask)] = a; r[(size_t) (w & mask)] = b; ++w; }
    // lecture a "delay" echantillons derriere la tete d'ecriture (interpolation Hermite)
    inline void read (double delay, float& a, float& b) const
    {
        const double pos = (double) (w - 1) - delay;
        const int64_t i = (int64_t) std::floor (pos);
        const float f = (float) (pos - (double) i);
        auto h = [f] (float y0, float y1, float y2, float y3)
        {
            const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            return ((c3 * f + c2) * f + c1) * f + y1;
        };
        const size_t i0 = (size_t) ((i - 1) & mask), i1 = (size_t) (i & mask), i2 = (size_t) ((i + 1) & mask), i3 = (size_t) ((i + 2) & mask);
        a = h (l[i0], l[i1], l[i2], l[i3]);
        b = h (r[i0], r[i1], r[i2], r[i3]);
    }
};

//==============================================================================
// Transposeur granulaire synchrone a la hauteur (2 tetes en fondu complementaire)
struct Shifter
{
    float p = 0.0f, W = 1024.0f, Wnext = 1024.0f;
    float logR = 0.0f, targetLog = 0.0f, glideK = 0.05f;
    float mix = 0.0f;                 // 0 = lecture directe (transparent), 1 = transposition
    bool  alwaysShift = false;

    void setTarget (float semis) { targetLog = semis / 12.0f; }
    void setWindow (float samples) { Wnext = samples; }
    void reset() { p = 0.0f; logR = targetLog = 0.0f; mix = alwaysShift ? 1.0f : 0.0f; }

    inline void process (const Ring& ring, double center, float& outL, float& outR)
    {
        logR += glideK * (targetLog - logR);
        const bool need = alwaysShift || std::abs (targetLog) > 1.0e-4f || std::abs (logR) > 2.0e-4f;
        mix += 0.005f * ((need ? 1.0f : 0.0f) - mix);
        float dl = 0, dr = 0;
        if (mix < 0.999f) ring.read (center, dl, dr);
        if (mix > 0.001f)
        {
            const float r = std::exp2 (logR);
            p += (1.0f - r) / W;
            if (p >= 1.0f || p < 0.0f) { p -= std::floor (p); W = Wnext; }
            const float p2 = p + 0.5f >= 1.0f ? p - 0.5f : p + 0.5f;
            float a1, b1, a2, b2;
            ring.read (center - W * 0.5f + p * W, a1, b1);
            ring.read (center - W * 0.5f + p2 * W, a2, b2);
            const float s1 = std::sin (juce::MathConstants<float>::pi * p), s2 = std::sin (juce::MathConstants<float>::pi * p2);
            const float g1 = s1 * s1, g2 = s2 * s2;
            outL = dl * (1.0f - mix) + (a1 * g1 + a2 * g2) * mix;
            outR = dr * (1.0f - mix) + (b1 * g1 + b2 * g2) * mix;
        }
        else { outL = dl; outR = dr; }
    }
};

//==============================================================================
class Engine
{
public:
    struct Viz   // pour l'animation
    {
        float origMidi = 0, newMidi = 0, h1Midi = -1, h2Midi = -1;
        float step = 0;          // position dans le cycle de 2 mesures (0..32 doubles-croches)
        bool grace = false, stutter = false, voiced = true;
        int64_t at = 0;          // debut de la note (index d'entree)
    };

    void prepare (double sampleRate, int maxBlock);
    int latency() const { return L; }
    void setPreset (int index);       // lance le morphing (3 s)
    int  presetIndex() const { return presetIdx; }

    // buffer stereo en place ; firstSample = index absolu d'entree du 1er echantillon
    void process (juce::AudioBuffer<float>& buf, int64_t firstSample, double ppq, double spb, bool playing);

    // etat visible (lu par l'interface)
    std::array<std::atomic<float>, Web::NumParams> shown {};
    std::atomic<int> keyTonic { 0 }, keyMinor { 1 };
    std::atomic<float> keyConf { 0.0f }, cyclePos { 0.0f }, morph { 1.0f }, density { 0.0f };
    std::atomic<int> notesSeen { 0 }, notesChanged { 0 }, notesOutOfKey { 0 };
    juce::AbstractFifo vizFifo { 256 };
    std::array<Viz, 256> vizBuf;

private:
    double sr = 44100.0;
    int L = 2048;
    int presetIdx = 0;
    Analyzer analyzer;
    Ring hist, echoRing, revRing;
    std::vector<float> mono;
    std::vector<Analyzer::NoteEvent> events;

    // morphing des fils
    std::array<float, Web::NumParams + 1> cur {}, from {}, to {};
    float morphT = 1.0f;
    int h1Type = 0, h2Type = 0; bool evolve = false;
    int nextH1 = 0, nextH2 = 0; bool nextEvolve = false;
    std::array<float, Web::NumParams> eff {};   // fils effectifs (apres humeur)

    // commandes de notes (appliquees L echantillons plus tard)
    struct Cmd
    {
        int64_t at = 0;
        bool voiced = true;
        float mainSemis = 0, graceSemis = 0; int graceLen = 0;
        float h1Semis = 0, h2Semis = 0, h1Gain = 0, h2Gain = 0; int arp1 = 0, arp2 = 0;
        float window = 1024;
        int stutRepeats = 0, stutLen = 0; bool stutReverse = false;
        float accent = 1.0f, echoSemis = 0.0f, glideMs = 10.0f;
        float echoK[3] = { 0, 0, 0 };   // repetitions de l'echo : +1, +2, +3 degres (dans la gamme)
    };
    std::array<Cmd, 64> cmds; int cmdHead = 0, cmdTail = 0;
    Cmd active; bool hasActive = false;
    std::array<Cmd, 32> past; int pastPos = 0;     // notes deja jouees (pour l'echo en spirale)
    std::array<Shifter, 3> echoTap; std::array<int, 3> echoNote { -1, -1, -1 };
public:
    bool testMainOnly = false;   // tests : voix principale seule
private:
    int64_t graceEnd = 0, h1Start = 0, h2Start = 0;

    // decisions de melodie
    int prevOrigDeg = 0, prevNewDeg = 0; int64_t prevNoteSample = -10000000;
    float lastSpb = 22050;

    // voix
    Shifter mainV, h1V, h2V, echoV;
    float h1Gain = 0, h1Target = 0, h2Gain = 0, h2Target = 0, accent = 1, accentTarget = 1;

    // stutter
    juce::AudioBuffer<float> stutBuf;
    int stutState = 0, stutPos = 0, stutLen = 0, stutRepeatsLeft = 0, stutRepeatIdx = 0; bool stutRev = false;
    float stutFade = 0.0f;

    // reverse delay
    int64_t revCount = 0;
    float echoLpState[2] = { 0, 0 };
    float levelIn = 0, levelOut = 0, levelGain = 1, limEnv = 0;

    // ton, espace
    juce::dsp::IIR::Filter<float> lowL, lowR, highL, highR;
    float lastLight = -10.0f;
    juce::dsp::Reverb reverb;

    void startMorphTo (int index);
    void updateMood();
    Cmd decide (const Analyzer::NoteEvent& e, double ppqAtNote, double spb);
    void pushViz (const Viz& v);
};
