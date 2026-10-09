#include "PianoVoice.h"

//==============================================================================
//                tilt  hammer detune sustain hfDamp noise nCut  pad  padOct padCut padAtk padSine sub  lp      chorus revDamp gain
static const TypeProfile profiles[] =
{
    /* Doux       */ { 1.85f, 0.135f, 0.6f, 0.85f, 0.11f, 0.55f, 900.f,  0.00f, 1.0f, 3.0f, 0.6f, 1.0f, 0.00f,  3800.f, 0.15f, 0.55f, 1.25f },
    /* Grave      */ { 1.45f, 0.120f, 0.9f, 1.45f, 0.07f, 0.25f, 1800.f, 0.10f, 0.5f, 3.0f, 0.9f, 1.0f, 0.40f,  7500.f, 0.10f, 0.50f, 1.05f },
    /* Orchestre  */ { 1.05f, 0.118f, 1.2f, 1.15f, 0.06f, 0.30f, 3500.f, 0.45f, 1.0f, 6.0f, 0.55f, 0.0f, 0.05f, 16000.f, 0.20f, 0.35f, 0.90f },
    /* Cinematique*/ { 1.35f, 0.125f, 1.0f, 1.35f, 0.08f, 0.20f, 2000.f, 0.35f, 0.5f, 4.0f, 1.2f, 0.4f, 0.25f, 11000.f, 0.30f, 0.30f, 0.95f },
    /* Dreaming   */ { 1.55f, 0.130f, 2.6f, 1.25f, 0.10f, 0.15f, 1500.f, 0.40f, 2.0f, 5.0f, 1.6f, 0.85f, 0.05f,  9000.f, 0.55f, 0.20f, 1.00f },
};

const TypeProfile& getProfile (PianoType t)
{
    return profiles[juce::jlimit (0, (int) PianoType::Count - 1, (int) t)];
}

juce::StringArray getTypeNames()
{
    return { "Doux", "Grave", "Orchestre", "Cinematique", "Dreaming" };
}

//==============================================================================
float PianoVoice::polyBlep (double t, double dt)
{
    if (t < dt)       { t /= dt;               return (float) (t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt;    return (float) (t * t + t + t + 1.0); }
    return 0.0f;
}

static float decayCoef (double seconds, double sr)
{
    // coefficient par echantillon pour une chute de 60 dB en "seconds"
    return (float) std::exp (-6.907755 / (juce::jmax (0.001, seconds) * sr));
}

//==============================================================================
void PianoVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int)
{
    const double sr  = getSampleRate();
    const auto& prof = getProfile ((PianoType) params.type.load());
    const float tone = params.tone.load();
    const float sens = params.velocity.load();
    const float width = params.width.load();
    const float layer = params.layer.load();

    const float vel = juce::jlimit (0.0f, 1.0f, velocity);
    const float n   = (float) juce::jlimit (9, 120, midiNote + params.octave.load() * 12);

    // Accordage "stretch" comme un vrai piano (aigus legerement hauts, graves legerement bas)
    const float octFrom69 = (n - 69.0f) / 12.0f;
    const float stretchCents = 1.2f * octFrom69 * std::abs (octFrom69);
    const double f0 = 440.0 * std::pow (2.0, (n - 69.0) / 12.0 + stretchCents / 1200.0);

    // Inharmonicite (cordes plus raides dans les aigus)
    const double B = 0.00006 * std::pow (2.0, (n - 48.0) / 14.0);

    // Brillance : variete + bouton Tone + velocite
    const float tilt = juce::jlimit (0.55f, 3.0f, prof.tilt - tone * 0.6f - (vel - 0.5f) * 0.7f);

    // Duree de resonance selon la hauteur
    const double baseT60 = juce::jlimit (0.9, 26.0, 20.0 * std::pow (2.0, -(n - 21.0) / 20.0)) * prof.sustainMul;

    // Panoramique selon la touche (comme assis devant le piano)
    const float keyPan = juce::jlimit (-1.0f, 1.0f, (n - 64.0f) / 44.0f) * width * 0.6f;

    const int partials = juce::jlimit (1, maxPartials, (int) ((0.45 * sr) / f0));
    const float detune = prof.detuneCents * (0.8f + 0.4f * rng.nextFloat());

    numOsc = 0;
    double energy = 0.0;

    for (int k = 1; k <= partials; ++k)
    {
        const double fk = k * f0 * std::sqrt (1.0 + B * k * k);
        if (fk >= 0.47 * sr) break;

        // Forme du spectre : pente + position du marteau
        const double hammer = 0.25 + 0.75 * std::abs (std::sin (juce::MathConstants<double>::pi * k * prof.hammerPos));
        double a = hammer / std::pow ((double) k, (double) tilt);
        // les partiels hauts n'apparaissent qu'avec une frappe forte
        a *= std::exp (-(double) (k - 1) * (0.06 + (1.0 - vel) * 0.10) * (1.0 - tone * 0.4));

        const double t60k = baseT60 / (1.0 + prof.hfDamping * (k - 1) * (1.0 + (n - 21.0) / 88.0));

        for (int s = 0; s < numStrings; ++s)
        {
            const double cents = (s == 0 ? -0.5 : 0.5) * detune;
            const double f = fk * std::pow (2.0, cents / 1200.0);
            const double w = juce::MathConstants<double>::twoPi * f / sr;
            const int i = numOsc++;

            const double startPhase = rng.nextDouble() * juce::MathConstants<double>::twoPi;
            oc[(size_t) i] = (float) std::cos (startPhase);
            os[(size_t) i] = (float) std::sin (startPhase);
            cw[(size_t) i] = (float) std::cos (w);
            sw[(size_t) i] = (float) std::sin (w);

            // corde 1 : son "attaque" plus fort mais plus court ; corde 2 : "aftersound" plus long
            const double strAmp = (s == 0 ? 0.62 : 0.38) * a;
            const double strT60 = (s == 0 ? t60k * 0.55 : t60k * 1.6);
            amp[(size_t) i] = (float) strAmp;
            dec[(size_t) i] = decayCoef (strT60, sr);

            const float p = juce::jlimit (-1.0f, 1.0f, keyPan + (s == 0 ? -0.18f : 0.18f) * width);
            panL[(size_t) i] = std::sqrt (0.5f * (1.0f - p));
            panR[(size_t) i] = std::sqrt (0.5f * (1.0f + p));

            energy += strAmp * strAmp;
        }
    }

    // normalisation pour un volume constant sur tout le clavier
    const float norm = (float) (0.22 / std::sqrt (juce::jmax (1.0e-9, energy)));
    for (int i = 0; i < numOsc; ++i)
        amp[(size_t) i] *= norm;

    // Sub-basse (surtout dans les graves)
    {
        const double w = juce::MathConstants<double>::twoPi * (f0 * 0.5) / sr;
        subC = 1.0f; subS = 0.0f;
        subCw = (float) std::cos (w); subSw = (float) std::sin (w);
        const float lowWeight = juce::jlimit (0.0f, 1.0f, (72.0f - n) / 40.0f);
        subAmp = prof.subAmount * lowWeight * 0.20f;
        subDec = decayCoef (baseT60 * 0.7, sr);
    }

    // Bruit du marteau / feutre
    noiseAmp = prof.noise * vel * vel * 0.10f * (1.0f + juce::jmax (0.0f, tone) * 0.5f);
    noiseDec = decayCoef (0.035 + (1.0 - vel) * 0.02, sr);
    const float nc = juce::jmin (prof.noiseCutoff * (0.7f + vel * 0.6f) * (1.0f + (n - 60.0f) / 60.0f), (float) sr * 0.4f);
    noiseCoef   = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmax (80.0f, nc) / (float) sr);
    noiseHpCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 120.0f / (float) sr);
    nLp1 = nLp2 = nHp = 0.0f;

    // Nappe
    padTarget = prof.padAmount * layer * 2.0f * (0.45f + 0.55f * vel) * 0.10f;
    padSine = prof.padSine;
    padLevel = 0.0f;
    padAtkCoef = 1.0f - std::exp (-1.0f / (prof.padAttack * 0.35f * (float) sr));
    padRelCoef = 1.0f;
    {
        const double pf = f0 * prof.padOctave;
        const double dets[3] = { -7.0, 0.0, 7.0 };
        for (int i = 0; i < 3; ++i)
        {
            padInc[(size_t) i] = pf * std::pow (2.0, dets[i] / 1200.0) / sr;
            padPhase[(size_t) i] = rng.nextDouble();
        }
        const float pc = (float) juce::jmin (pf * prof.padCutMul + 300.0, sr * 0.4);
        padCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * pc / (float) sr);
        padLp1L = padLp2L = padLp1R = padLp2R = 0.0f;
        const float pp = keyPan * 0.5f;
        padPanL = std::sqrt (0.5f * (1.0f - pp));
        padPanR = std::sqrt (0.5f * (1.0f + pp));
    }

    // Volume selon velocite (sensibilite reglable)
    voiceGain = ((1.0f - sens) * 0.75f + sens * std::pow (vel, 1.7f)) * prof.gain;

    attackRamp = 0.0f;
    attackInc  = 1.0f / (0.0015f * (float) sr);
    releasing = false;
    releaseCoef = 1.0f;
    renormCounter = 0;
}

void PianoVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        const double sr = getSampleRate();
        const double rel = params.release.load();
        releasing = true;
        releaseCoef = decayCoef (rel, sr);
        padRelCoef  = decayCoef (juce::jmax (rel * 1.5, 0.6), sr);
    }
    else
    {
        clearCurrentNote();
        numOsc = 0;
    }
}

void PianoVoice::renderNextBlock (juce::AudioBuffer<float>& out, int start, int num)
{
    if (! isVoiceActive())
        return;

    auto* L = out.getWritePointer (0);
    auto* R = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;

    for (int i = start; i < start + num; ++i)
    {
        float l = 0.0f, r = 0.0f;

        // --- cordes ---
        for (int k = 0; k < numOsc; ++k)
        {
            const float c = oc[(size_t) k], s = os[(size_t) k];
            const float nc = c * cw[(size_t) k] - s * sw[(size_t) k];
            const float ns = c * sw[(size_t) k] + s * cw[(size_t) k];
            oc[(size_t) k] = nc; os[(size_t) k] = ns;
            const float v = ns * amp[(size_t) k];
            amp[(size_t) k] *= dec[(size_t) k];
            l += v * panL[(size_t) k];
            r += v * panR[(size_t) k];
        }

        // --- sub ---
        if (subAmp > 1.0e-6f)
        {
            const float nc = subC * subCw - subS * subSw;
            const float ns = subC * subSw + subS * subCw;
            subC = nc; subS = ns;
            const float v = ns * subAmp;
            subAmp *= subDec;
            l += v * 0.707f; r += v * 0.707f;
        }

        // --- marteau ---
        if (noiseAmp > 1.0e-6f)
        {
            const float w = rng.nextFloat() * 2.0f - 1.0f;
            nLp1 += noiseCoef * (w - nLp1);
            nLp2 += noiseCoef * (nLp1 - nLp2);
            nHp  += noiseHpCoef * (nLp2 - nHp);
            const float v = (nLp2 - nHp) * noiseAmp;
            noiseAmp *= noiseDec;
            l += v; r += v;
        }

        // --- nappe ---
        if (padTarget > 0.0f)
        {
            if (! releasing) padLevel += padAtkCoef * (padTarget - padLevel);
            else             padLevel *= padRelCoef;

            float pl = 0.0f, pr = 0.0f;
            for (int p = 0; p < 3; ++p)
            {
                double& ph = padPhase[(size_t) p];
                const double dt = padInc[(size_t) p];
                const float saw  = (float) (2.0 * ph - 1.0) - polyBlep (ph, dt);
                const float sine = std::sin ((float) ph * juce::MathConstants<float>::twoPi);
                const float v = saw * (1.0f - padSine) + sine * padSine;
                ph += dt; if (ph >= 1.0) ph -= 1.0;
                if (p == 0) pl += v; else if (p == 2) pr += v; else { pl += v * 0.7f; pr += v * 0.7f; }
            }
            padLp1L += padCoef * (pl - padLp1L); padLp2L += padCoef * (padLp1L - padLp2L);
            padLp1R += padCoef * (pr - padLp1R); padLp2R += padCoef * (padLp1R - padLp2R);
            l += padLp2L * padLevel * padPanL;
            r += padLp2R * padLevel * padPanR;
        }

        // --- enveloppes globales ---
        if (attackRamp < 1.0f) attackRamp = juce::jmin (1.0f, attackRamp + attackInc);
        float g = voiceGain * attackRamp;
        if (releasing)
        {
            voiceGain *= releaseCoef;
            g = voiceGain * attackRamp;
        }

        L[i] += l * g;
        if (R != nullptr) R[i] += r * g;
    }

    // Re-normalise les phaseurs (evite la derive numerique)
    if (++renormCounter >= 16)
    {
        renormCounter = 0;
        for (int k = 0; k < numOsc; ++k)
        {
            const float m = 1.0f / std::sqrt (oc[(size_t) k] * oc[(size_t) k] + os[(size_t) k] * os[(size_t) k]);
            oc[(size_t) k] *= m; os[(size_t) k] *= m;
        }
        const float ms = 1.0f / std::sqrt (subC * subC + subS * subS);
        subC *= ms; subS *= ms;
    }

    // La note s'est eteinte ?
    float peak = subAmp + noiseAmp;
    for (int k = 0; k < numOsc; ++k) peak += amp[(size_t) k];
    peak *= voiceGain;
    const float padPeak = padLevel * voiceGain;

    if ((peak < 2.0e-5f && (padTarget <= 0.0f || releasing) && padPeak < 2.0e-5f) || voiceGain < 1.0e-6f)
    {
        clearCurrentNote();
        numOsc = 0;
    }
}
