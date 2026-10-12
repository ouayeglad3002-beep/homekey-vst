#pragma once
#include <JuceHeader.h>

// =============================================================================
// SPIDER II : la chaine d'effets
// Tout ce qui bouge est cale sur la position musicale (ppq) : la lecture et l'export
// donnent le meme son.
// =============================================================================
namespace Fx
{
    constexpr float pi = juce::MathConstants<float>::pi;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    //--------------------------------------------------------------------------
    // Filtre a variable d'etat (Cytomic / Simper) : passe-bas, passe-haut, passe-bande
    struct Svf
    {
        float ic1 = 0, ic2 = 0, g = 0, k = 1.4f, a1 = 0, a2 = 0, a3 = 0;
        void set (float fc, float q, double sr)
        {
            g = std::tan (pi * juce::jlimit (10.0f, 0.45f * (float) sr, fc) / (float) sr);
            k = 1.0f / juce::jmax (0.3f, q);
            a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
        }
        void reset() { ic1 = ic2 = 0; }
        // renvoie lp ; bp et hp en sortie
        inline float tick (float v0, float& bp, float& hp)
        {
            const float v3 = v0 - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
            bp = v1; hp = v0 - k * v1 - v2;
            return v2;
        }
        inline float lp (float x) { float b, h; return tick (x, b, h); }
        inline float hp (float x) { float b, h; tick (x, b, h); return h; }
    };

    //--------------------------------------------------------------------------
    // Saturation anti-repliement (ADAA du 1er ordre) : tanh, sans aliasing aigu
    struct AdaaTanh
    {
        float x1 = 0;
        static inline float F (float x) { const float a = std::abs (x); return a > 15.0f ? a - 0.6931472f : std::log (std::cosh (x)); }
        inline float tick (float x)
        {
            const float d = x - x1;
            float y = std::abs (d) < 1.0e-4f ? std::tanh (0.5f * (x + x1)) : (F (x) - F (x1)) / d;
            x1 = x;
            return y;
        }
    };

    //--------------------------------------------------------------------------
    // Ligne a retard stereo simple, lecture fractionnaire
    struct Line
    {
        std::vector<float> l, r; int mask = 0, w = 0;
        void init (int n) { int s = 1; while (s < n) s <<= 1; l.assign ((size_t) s, 0.0f); r.assign ((size_t) s, 0.0f); mask = s - 1; w = 0; }
        void clear() { std::fill (l.begin(), l.end(), 0.0f); std::fill (r.begin(), r.end(), 0.0f); }
        inline void push (float a, float b) { l[(size_t) w] = a; r[(size_t) w] = b; w = (w + 1) & mask; }
        inline void read (float d, float& a, float& b) const
        {
            const float pos = (float) (w - 1) - juce::jmax (0.0f, d);
            const int i = (int) std::floor (pos); const float f = pos - (float) i;
            const size_t i0 = (size_t) (i & mask), i1 = (size_t) ((i + 1) & mask);
            a = l[i0] + f * (l[i1] - l[i0]); b = r[i0] + f * (r[i1] - r[i0]);
        }
    };

    //--------------------------------------------------------------------------
    // Transposeur d'octave (+12) granulaire, 2 tetes en fondu sin^2 (gain <= 1)
    struct OctaveUp
    {
        std::array<float, 4096> buf {}; int w = 0; float ph = 0;
        static constexpr float W = 1536.0f;
        void clear() { buf.fill (0.0f); w = 0; ph = 0; }
        inline float tick (float x)
        {
            buf[(size_t) (w & 4095)] = x;
            float s = 0;
            for (int h = 0; h < 2; ++h)
            {
                const float p = std::fmod (ph + (float) h * 0.5f * W, W);
                const float pos = (float) w - (W - p);              // le retard diminue : lecture 2x
                const int i0 = (int) std::floor (pos); const float f = pos - (float) i0;
                const float v = buf[(size_t) (i0 & 4095)] * (1 - f) + buf[(size_t) ((i0 + 1) & 4095)] * f;
                const float win = std::sin (pi * p / W);
                s += v * win * win;
            }
            ph += 1.0f; if (ph >= W) ph -= W;
            ++w;
            return s;
        }
    };

    //--------------------------------------------------------------------------
    // Reverb a reseau de retards (FDN 8 lignes, matrice de Householder), diffusion,
    // modulation lente, amortissement, pre-delay et shimmer (+12 dans la boucle).
    struct Fdn
    {
        static constexpr int N = 8;
        std::array<std::vector<float>, N> lines; std::array<int, N> wpos {}, len {};
        std::array<float, N> lp {}, gain {};
        std::array<std::vector<float>, 4> apL, apR; std::array<int, 4> apPos {}, apPosR {}, apLen {};
        Line pre; OctaveUp shift[2];
        float damp = 0.3f, size = 1.0f, decay = 2.0f, preMs = 20.0f, shimmer = 0.0f;
        double sr = 44100; int64_t t = 0;
        Svf shimHp;

        void prepare (double rate)
        {
            sr = rate;
            for (auto& v : lines) v.assign ((size_t) (0.25 * sr) + 64, 0.0f);
            static const float ap[4] = { 4.77f, 3.59f, 12.73f, 9.30f };
            for (int i = 0; i < 4; ++i) { apLen[(size_t) i] = (int) (ap[i] * 0.001 * sr); apL[(size_t) i].assign ((size_t) apLen[(size_t) i] + 1, 0.0f); apR[(size_t) i].assign ((size_t) apLen[(size_t) i] + 7, 0.0f); }
            pre.init ((int) (0.3 * sr));
            shimHp.set (300.0f, 0.7f, sr);
            setParams (1.0f, 2.0f, 0.3f, 20.0f, 0.0f);
            clear();
        }
        void clear()
        {
            for (auto& v : lines) std::fill (v.begin(), v.end(), 0.0f);
            for (auto& v : apL) std::fill (v.begin(), v.end(), 0.0f);
            for (auto& v : apR) std::fill (v.begin(), v.end(), 0.0f);
            pre.clear(); shift[0].clear(); shift[1].clear();
            lp.fill (0.0f); wpos.fill (0); apPos.fill (0); apPosR.fill (0); shimHp.reset();
        }
        void setParams (float sizeIn, float decayS, float dampIn, float preDelayMs, float shimmerIn)
        {
            static const float base[N] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.9f, 59.3f, 67.1f, 73.3f };
            size = juce::jlimit (0.3f, 2.5f, sizeIn); decay = juce::jlimit (0.2f, 30.0f, decayS);
            damp = juce::jlimit (0.0f, 0.95f, dampIn); preMs = preDelayMs; shimmer = shimmerIn;
            for (int i = 0; i < N; ++i)
            {
                len[(size_t) i] = juce::jmin ((int) lines[(size_t) i].size() - 40, (int) (base[i] * size * 0.001 * sr));
                gain[(size_t) i] = std::pow (10.0f, -3.0f * (float) len[(size_t) i] / (decay * (float) sr));
            }
        }
        inline float allpass (std::vector<float>& b, int& p, int n, float x, float g)
        {
            const float d = b[(size_t) p];
            const float y = d - g * x;
            b[(size_t) p] = x + g * y;
            p = (p + 1) % n;
            return y;
        }
        inline void tick (float inL, float inR, float& outL, float& outR)
        {
            pre.push (inL, inR);
            float a, b; pre.read (preMs * 0.001f * (float) sr, a, b);
            // diffusion d'entree (la droite a des longueurs decalees : decorrelation stereo)
            float xl = a, xr = b;
            for (int i = 0; i < 4; ++i)
            {
                xl = allpass (apL[(size_t) i], apPos[(size_t) i], apLen[(size_t) i], xl, 0.62f);
                xr = allpass (apR[(size_t) i], apPosR[(size_t) i], apLen[(size_t) i] + 7, xr, 0.62f);
            }
            // lecture des 8 lignes (2 modulees lentement)
            std::array<float, N> o {};
            const float mod1 = 6.0f * std::sin (twoPi * 0.31f * (float) t / (float) sr);
            const float mod2 = 6.0f * std::sin (twoPi * 0.47f * (float) t / (float) sr + 1.3f);
            for (int i = 0; i < N; ++i)
            {
                auto& v = lines[(size_t) i];
                const int sz = (int) v.size();
                float d = (float) len[(size_t) i] + (i == 1 ? mod1 : i == 6 ? mod2 : 0.0f);
                float pos = (float) wpos[(size_t) i] - d; while (pos < 0) pos += (float) sz;
                const int i0 = (int) pos; const float f = pos - (float) i0;
                const float val = v[(size_t) (i0 % sz)] * (1 - f) + v[(size_t) ((i0 + 1) % sz)] * f;
                lp[(size_t) i] += (1.0f - damp) * (val - lp[(size_t) i]);
                o[(size_t) i] = lp[(size_t) i] * gain[(size_t) i];
            }
            // matrice de Householder
            float sum = 0; for (float v : o) sum += v;
            sum *= 2.0f / (float) N;
            for (int i = 0; i < N; ++i)
            {
                const float in = (i % 2 == 0 ? xl : xr) * ((i & 2) ? -0.5f : 0.5f);
                auto& v = lines[(size_t) i];
                float fb = o[(size_t) i] - sum;
                // shimmer DANS la boucle : une partie du retour remonte d'une octave (gain total <= 1, stable)
                if (i < 2 && shimmer > 0.001f)
                {
                    const float up = shift[i].tick (i == 0 ? shimHp.hp (fb) : fb);
                    fb = (1.0f - shimmer) * fb + shimmer * up;
                }
                v[(size_t) wpos[(size_t) i]] = fb + in;
                wpos[(size_t) i] = (wpos[(size_t) i] + 1) % (int) v.size();
            }
            outL = o[0] - o[2] + o[4] - o[6] + 0.5f * (o[1] - o[5]);
            outR = o[1] - o[3] + o[5] - o[7] + 0.5f * (o[2] - o[6]);
            outL *= 0.5f; outR *= 0.5f;
            ++t;
        }
    };

    //--------------------------------------------------------------------------
    // Motifs rythmiques (16 pas par mesure) : volume, comme un "shaper"
    inline float shaperGain (int pattern, double beatPos)
    {
        const double bar = std::fmod (beatPos, 4.0); const double bpos = bar < 0 ? bar + 4.0 : bar;
        const int step = (int) (bpos * 4.0) & 15;
        const float frac = (float) (bpos * 4.0 - std::floor (bpos * 4.0));
        auto gate = [&] (const float* p) { const float on = p[step]; return on > 0.5f ? (frac < 0.8f ? 1.0f : 1.0f - (frac - 0.8f) * 5.0f * (1.0f - p[(step + 1) & 15])) : 0.0f; };
        static const float trance[16] = { 1,0,1,1, 1,0,1,0, 1,1,0,1, 1,0,1,1 };
        static const float chop[16]   = { 1,1,1,0, 0,0,1,1, 1,0,0,1, 1,0,1,0 };
        switch (pattern)
        {
            case 1: return gate (trance);
            case 2: { const float b = (float) (bpos - std::floor (bpos)); return 0.25f + 0.75f * std::pow (juce::jmin (1.0f, b / 0.55f), 0.7f); }   // pompe (sidechain)
            case 3: return gate (chop);
            case 4: { const float b = (float) std::fmod (bpos, 2.0) / 2.0f; return 0.15f + 0.85f * b * b; }   // montee inversee (swell) sur 2 temps
            default: return 1.0f;
        }
    }

    //--------------------------------------------------------------------------
    // Limiteur avec anticipation (pas de distorsion sur les crêtes)
    struct Limiter
    {
        static constexpr int look = 64;
        Line d; std::array<float, look> peaks {}; int pp = 0;
        float g = 1, rel = 0.999f, att = 0.9f;
        void prepare (double sr)
        {
            d.init (look + 8); d.clear(); peaks.fill (0.0f); pp = 0; g = 1;
            rel = std::exp (-1.0f / (0.12f * (float) sr));
            att = std::exp (-1.0f / ((float) look / 6.0f));     // 99 % atteint avant que la crete sorte
        }
        inline void tick (float& a, float& b, float thr)
        {
            // crete maximale de la fenetre a venir (les 'look' prochains echantillons de sortie)
            peaks[(size_t) pp] = juce::jmax (std::abs (a), std::abs (b)); pp = (pp + 1) % look;
            float pk = 0; for (float v : peaks) pk = juce::jmax (pk, v);
            const float target = pk > thr ? thr / pk : 1.0f;
            g = target < g ? att * g + (1 - att) * target : rel * g + (1 - rel) * target;
            d.push (a, b);
            float x, y; d.read ((float) (look - 1), x, y);
            a = x * g; b = y * g;
            a = juce::jlimit (-0.98f, 0.98f, a); b = juce::jlimit (-0.98f, 0.98f, b);   // securite absolue
        }
    };
}
