#pragma once
#include <JuceHeader.h>
#include "MusicTheory.h"
#include "Analyzer.h"
#include "signalsmith-stretch.h"
#include "Fx.h"

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

    // Spider II : la signature sonore de chaque preset (cachee, comme les fils)
    enum S { Formant, Spread, Detune, DriveMode, Drive, Crush, Wow, Chorus,
             DelayMode, DelayBeats, DelayFb, DelayMix,
             RevSize, RevDecay, RevDamp, RevPre, Shimmer, RevMix,
             Shaper, ShaperDepth, LowCut, HighCut, Sweep, Width, NumS };
    inline bool isDiscrete (int i) { return i == DriveMode || i == DelayMode || i == Shaper || i == DelayBeats; }

    struct Preset
    {
        const char* name;
        const char* desc;
        float p[NumParams];   // Expand : 0..1 -> x0.5 .. x2 ; Light : 0..1 -> sombre..lumineux (0.5 = neutre)
        float grit;           // (v1) inutilise depuis Spider II : remplace par Drive
        int h1Type, h2Type;   // types d'harmonie
        bool evolve;          // la toile change a chaque cycle (infini)
        float s[NumS];        // signature sonore
    };

    constexpr int numPresets = 15;
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
// Marques de periode (epoques) : un repere par cycle de l'onde, cale sur la crete du fondamental
struct Epochs
{
    std::vector<float> lp; int64_t mask = 0, w = 0;   // signal mono filtre, indexe en absolu
    float s1 = 0, s2 = 0, k = 0.1f;
    std::array<int64_t, 256> e {}; int count = 0; int64_t last = -1;
    float period = 400.0f;

    void init (int sizePow2) { lp.assign ((size_t) sizePow2, 0.0f); mask = sizePow2 - 1; w = 0; s1 = s2 = 0; count = 0; last = -1; }
    void setPeriod (float P, double sr)
    {
        period = juce::jlimit (16.0f, 2400.0f, P);
        const float fc = juce::jmin (0.4f * (float) sr, 1.4f * (float) sr / period);
        k = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) sr);
    }
    inline void push (float x) { s1 += k * (x - s1); s2 += k * (s1 - s2); lp[(size_t) (w & mask)] = s2; ++w; }
    inline float at (int64_t i) const { return lp[(size_t) (i & mask)]; }
    // pose les epoques jusqu'a 'upTo' (index absolu deja recu)
    void advance (int64_t upTo)
    {
        if (last < 0) last = upTo - (int64_t) (2 * period);
        while ((float) last + 1.25f * period <= (float) upTo)
        {
            const int64_t lo = last + (int64_t) (0.75f * period), hi = last + (int64_t) (1.25f * period);
            int64_t best = lo; float bv = -1.0e9f;
            for (int64_t i = lo; i <= hi; ++i) { const float v = at (i); if (v > bv) { bv = v; best = i; } }
            last = juce::jmax (last + 1, best);
            e[(size_t) (count++ & 255)] = last;
        }
    }
    int64_t nearest (double m) const
    {
        int64_t best = (int64_t) m; double bd = 1.0e18;
        for (int j = 0; j < juce::jmin (count, 256); ++j)
        {
            const int64_t v = e[(size_t) ((count - 1 - j) & 255)];
            const double d = std::abs ((double) v - m);
            if (d < bd) { bd = d; best = v; }
            if ((double) v < m - 4.0 * period) break;
        }
        return best;
    }
};

//==============================================================================
// Voix TD-PSOLA : des grains de 2 periodes, pris sur les epoques, reposes a la nouvelle periode.
// Les grains ne sont pas reechantillonnes : le timbre (formants) reste celui de l'original.
struct Psola
{
    float logR = 0.0f, targetLog = 0.0f, glideK = 0.05f, mix = 0.0f, P0 = 400.0f, Hmax = 1100.0f;
    float fk = 1.0f;          // formant : lecture des grains plus vite (>1 = voix plus claire) ou plus lentement
    double nextMark = -1.0;
    std::vector<float> accL, accR, wsum; int64_t mask = 0;
    std::vector<float> win;   // fenetre de Hann tabulee

    void init (double sr)
    {
        accL.assign (8192, 0.0f); accR.assign (8192, 0.0f); wsum.assign (8192, 0.0f); mask = 8191;
        Hmax = (float) (0.025 * sr);
        win.resize (1025);
        for (int i = 0; i <= 1024; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / 1024.0f);
        reset();
    }
    void reset() { logR = targetLog = 0.0f; mix = 0.0f; nextMark = -1.0; std::fill (accL.begin(), accL.end(), 0.0f); std::fill (accR.begin(), accR.end(), 0.0f); std::fill (wsum.begin(), wsum.end(), 0.0f); }
    void setTarget (float semis) { targetLog = semis / 12.0f; }
    void setPeriod (float P) { P0 = juce::jlimit (16.0f, 2400.0f, P); }

    // o = index absolu de sortie ; t = dernier index d'entree ecrit dans 'ring'
    inline void process (const Ring& ring, const Epochs& ep, int64_t o, int64_t t, float& outL, float& outR)
    {
        logR += glideK * (targetLog - logR);
        const bool need = std::abs (targetLog) > 1.0e-4f || std::abs (logR) > 2.0e-4f || std::abs (fk - 1.0f) > 1.0e-3f;
        mix += 0.004f * ((need ? 1.0f : 0.0f) - mix);
        auto rd = [&] (int64_t idx, float& a, float& b)
        {
            const int64_t d = t - idx;
            const size_t k = (size_t) ((ring.w - 1 - d) & ring.mask);
            a = ring.l[k]; b = ring.r[k];
        };
        float dl, dr; rd (o, dl, dr);
        if (mix < 0.001f) { outL = dl; outR = dr; nextMark = -1.0; return; }

        const float r = std::exp2 (logR);
        const float P1 = P0 / r;
        const float H = juce::jlimit (32.0f, Hmax, P0);   // grain = 2 periodes d'origine (vrai PSOLA)
        if (nextMark < 0.0 || nextMark < (double) o - 4.0 * H)
        {
            std::fill (accL.begin(), accL.end(), 0.0f); std::fill (accR.begin(), accR.end(), 0.0f); std::fill (wsum.begin(), wsum.end(), 0.0f);
            nextMark = (double) o + H;
        }
        while (nextMark - H <= (double) o)
        {
            const int64_t m = (int64_t) std::llround (nextMark);
            const int64_t a = ep.nearest (nextMark);
            const int h = (int) H;
            const float step = 1024.0f / (float) (2 * h);
            const bool fshift = std::abs (fk - 1.0f) > 1.0e-3f;
            for (int j = -h; j <= h; ++j)
            {
                const float wv = win[(size_t) juce::jlimit (0, 1024, (int) ((float) (j + h) * step))];
                float xa, xb;
                if (! fshift) rd (a + j, xa, xb);
                else
                {
                    // grain relu a une autre vitesse : l'enveloppe spectrale (les formants) se deplace, la hauteur non
                    const float pos = (float) j * fk;
                    const int64_t i0 = a + (int64_t) std::floor (pos); const float f = pos - std::floor (pos);
                    float a0, b0, a1, b1; rd (i0, a0, b0); rd (i0 + 1, a1, b1);
                    xa = a0 + f * (a1 - a0); xb = b0 + f * (b1 - b0);
                }
                const size_t q = (size_t) ((m + j) & mask);
                accL[q] += xa * wv; accR[q] += xb * wv; wsum[q] += wv;
            }
            nextMark += juce::jmax (8.0f, P1);
        }
        // pas de normalisation point par point (elle deformerait les grains) : gain moyen = P1 / H
        const size_t q = (size_t) (o & mask);
        const float g = juce::jlimit (0.25f, 2.0f, P1 / H);
        const float yl = accL[q] * g, yr = accR[q] * g;
        accL[q] = accR[q] = wsum[q] = 0.0f;
        outL = dl * (1.0f - mix) + yl * mix;
        outR = dr * (1.0f - mix) + yr * mix;
    }
};

//==============================================================================
class Engine
{
public:
    struct Viz   // pour l'animation
    {
        float origMidi = 0, newMidi = 0, h1Midi = -1, h2Midi = -1;
        float step = 0;          // position dans le cycle de 4 mesures (0..64 doubles-croches)
        bool grace = false, stutter = false, voiced = true;
        int64_t at = 0;          // debut de la note (index d'entree)
    };

    void prepare (double sampleRate, int maxBlock);
    int maxBlock() const { return maxBlk; }

    // --- la toile apprise : sauvegardee dans le projet ---
    struct Learned { bool valid = false; int tonic = 0; bool minor = true; float reg = 64.0f, dens = 3.0f, gain = 1.0f; };
    static uint64_t pack (const Learned& l);
    static Learned unpack (uint64_t v);
    Learned learned() const { return unpack (learnedPacked.load()); }      // fil de l'interface
    void restoreLearned (const Learned& l) { restorePacked = pack (l); restorePending = true; }
    void requestRelearn() { relearnPending = true; }
    void onTransportJump();                    // saut / boucle de la tete de lecture
    std::atomic<int> learnCount { 0 };
    static constexpr int learnNotes = 16;
    int latency() const { return L + Fx::Limiter::look; }
    void setPreset (int index);       // lance le morphing (3 s)
    void setPresetImmediate (int index);   // sans morphing (rechargement du projet)
    std::atomic<float> currentGain { 1.0f };
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
    static constexpr int NumMorph = Web::NumParams + 1 + Web::NumS;
    std::array<float, NumMorph> cur {}, from {}, to {};
    inline float snd (int i) const { return cur[(size_t) (Web::NumParams + 1 + i)]; }
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
        float window = 1024, period = 400;
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
    bool testChordOnly = false;  // tests : voix d'accords seule
private:
    int64_t graceEnd = 0, h1Start = 0, h2Start = 0;

    // decisions de melodie
    int prevOrigDeg = 0, prevNewDeg = 0; int64_t prevNoteSample = -10000000;
    float lastSpb = 22050;

    // voix
    Psola mainV, h1V, h2V;
    Shifter echoV;
    Epochs epochs;
    std::array<std::pair<int64_t, float>, 32> periods {}; int periodHead = 0;   // (debut de note, periode) pour les epoques
    float periodAt (int64_t pos) const;
    int64_t Dt = 2822;          // retard du suivi d'epoques (les notes sont decidees avant)
    // accords : transposition diatonique polyphonique (Signalsmith Stretch)
    signalsmith::stretch::SignalsmithStretch<float> chord { 7 };
    std::vector<float> chIn[2], chOut[2];
    float chordHp[2][2] {}; float chordGain = 0.0f, chordTarget = 0.0f; int chordDeg = 2; int chordDelay = 0; bool chordOn = false;
    float chordMap (float f) const;
    float h1Gain = 0, h1Target = 0, h2Gain = 0, h2Target = 0, accent = 1, accentTarget = 1;

    // stutter
    juce::AudioBuffer<float> stutBuf;
    int stutState = 0, stutPos = 0, stutLen = 0, stutRepeatsLeft = 0, stutRepeatIdx = 0; bool stutRev = false;
    float stutFade = 0.0f; int stutRecover = 0;

    // reverse delay
    int64_t revCount = 0;
    float echoLpState[2] = { 0, 0 };
    float levelIn = 0, levelOut = 0, levelGain = 1, limEnv = 0;

    // ton, espace
    juce::dsp::IIR::Filter<float> lowL, lowR, highL, highR;
    float lastLight = -10.0f;
    // --- Spider II : chaine d'effets ---
    Fx::Fdn fdn; float fdnLast[5] = { -1, -1, -1, -1, -1 };
    Fx::Line dly, chor, wowLine;
    Fx::Svf dlyLp[2], dlyHp[2], toneHp[2], toneLp[2], sweepF[2];
    Fx::AdaaTanh sat[2], dlySat[2];
    Fx::Limiter limiter;
    float dlyTime = 0.0f, crushHold[2] = { 0, 0 }, crushCount = 0, dlyFbState[2] = { 0, 0 };
    float lastLow = -1, lastHigh = -1; int sweepCounter = 0;
    float revWet[2] = { 0, 0 }; float shapeG = 1.0f;

    // toile verrouillee : la transformation est choisie une fois pour toutes
    bool locked = false;
    Theory::Key lockKey;
    int axisDeg = 0; float lockRegister = 64.0f, lockDensity = 3.0f;
    uint32_t seed = 1;
    std::array<float, Web::NumParams> moodDelta {};
    std::array<float, learnNotes> learnMidi {};
    std::array<int64_t, learnNotes> learnAt {};
    float lockGain = 1.0f;
    int learnN = 0, learnE = 0; int64_t learnFirst = 0, learnLast = 0;   // learnN = notes seules, learnE = tous les evenements
    std::atomic<uint64_t> learnedPacked { 0 }, restorePacked { 0 };
    std::atomic<bool> restorePending { false }, relearnPending { false };
    int maxBlk = 4096;
    void applyLearned();          // axe, graine, humeur a partir de lockKey / lockRegister / lockDensity
    void publishLearned();

    void startMorphTo (int index);
    void updateMood (int n);
    Cmd decide (const Analyzer::NoteEvent& e, double ppqAtNote, double spb);
    void pushViz (const Viz& v);
};
