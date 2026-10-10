#include "PianoVoice.h"

//==============================================================================
//                    tilt  hammer detune sustain hfDamp noise nCut   pad  padOct padCut padAtk padSine sub   lp       chorus revDamp gain
static const TypeProfile profiles[] =
{
    /* DREAMING  */ { 1.55f, 0.130f, 2.6f, 1.25f, 0.10f, 0.15f, 1500.f, 0.40f, 2.0f, 3.2f, 1.6f, 0.85f, 0.05f,  9000.f, 0.55f, 0.20f, 1.00f },
    /* NEBULA    */ { 1.30f, 0.210f, 4.0f, 1.40f, 0.05f, 0.10f, 2500.f, 0.35f, 1.0f, 2.5f, 1.0f, 0.60f, 0.10f, 12000.f, 0.45f, 0.15f, 0.95f },
    /* CINEMA    */ { 1.35f, 0.125f, 1.0f, 1.35f, 0.08f, 0.20f, 2000.f, 0.35f, 0.5f, 2.6f, 1.2f, 0.40f, 0.30f, 11000.f, 0.30f, 0.30f, 0.95f },
    /* ABYSS     */ { 1.90f, 0.110f, 1.5f, 1.60f, 0.06f, 0.30f,  900.f, 0.30f, 0.5f, 1.8f, 1.5f, 0.30f, 0.50f,  6000.f, 0.15f, 0.45f, 1.05f },
    /* BACKROOMS */ { 1.70f, 0.140f, 3.5f, 1.00f, 0.12f, 0.35f, 1200.f, 0.25f, 1.0f, 2.0f, 2.0f, 0.90f, 0.00f,  3200.f, 0.35f, 0.80f, 1.20f },
};

const TypeProfile& getProfile (PianoType t)
{
    return profiles[juce::jlimit (0, (int) PianoType::Count - 1, (int) t)];
}

juce::StringArray getTypeNames()
{
    return { "Dreaming", "Nebula", "Cinema", "Abyss", "Backrooms" };
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
    return (float) std::exp (-6.907755 / (juce::jmax (0.001, seconds) * sr));
}

void PianoVoice::setPitch (float mul)
{
    for (int k = 0; k < numOsc; ++k)
    {
        const float w = baseW[(size_t) k] * mul;
        cw[(size_t) k] = std::cos (w);
        sw[(size_t) k] = std::sin (w);
    }
    subCw = std::cos (subW * mul);
    subSw = std::sin (subW * mul);
    lastPitchMul = mul;
}

//==============================================================================
void PianoVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel)
{
    const double sr  = getSampleRate();
    const auto& prof = getProfile ((PianoType) params.type.load());
    const float tone = params.tone.load();
    const float sens = params.velocity.load();
    const float width = params.width.load();
    const float layer = params.layer.load();
    const float metal = params.metal.load();
    const float var   = params.variation.load();

    bendSemis = (float) (pitchWheel - 8192) / 8192.0f * 2.0f;
    const float vel = juce::jlimit (0.0f, 1.0f, velocity);
    const int noteI = juce::jlimit (9, 120, midiNote + params.octave.load() * 12);
    const float n   = (float) noteI;

    // variation aleatoire par note (coherente : petite et continue)
    varCents = (rng.nextFloat() * 2.0f - 1.0f) * var * 14.0f;
    const float varTone = (rng.nextFloat() * 2.0f - 1.0f) * var * 0.35f;
    const float varPan  = (rng.nextFloat() * 2.0f - 1.0f) * var * 0.5f;

    const float cents = params.fine.load() + varCents;
    const float octFrom69 = (n - 69.0f) / 12.0f;
    const float stretchCents = 1.2f * octFrom69 * std::abs (octFrom69);
    const double f0 = 440.0 * std::pow (2.0, (n - 69.0) / 12.0 + (stretchCents + cents) / 1200.0);

    const float velEff = juce::jlimit (0.0f, 1.0f, 0.72f + (vel - 0.72f) * (0.25f + 0.75f * sens));
    const float keyPan = juce::jlimit (-1.0f, 1.0f, (n - 64.0f) / 44.0f * width * 0.6f + varPan);

    //---------------------------------------------------------------- echantillons
    zA = zB = zT = nullptr;
    const float pianoLvl = params.piano.load();
    if (pianoLvl > 0.001f && bank.isReady())
    {
        const float lp = velEff * (SampleBank::numLayers - 1);
        const int l0 = juce::jlimit (0, SampleBank::numLayers - 1, (int) lp);
        const int l1 = juce::jmin (l0 + 1, SampleBank::numLayers - 1);
        const float xf = lp - (float) l0;
        zA = bank.pianoZone (noteI, l0);
        zB = (l1 != l0) ? bank.pianoZone (noteI, l1) : nullptr;
        const float g = pianoLvl * 0.9f * (0.55f + 0.45f * (1.0f - sens) + 0.45f * sens * velEff) * prof.gain;
        gainA = g * (zB != nullptr ? 1.0f - xf : 1.0f);
        gainB = g * xf;
        if (zA != nullptr)
        {
            incA = std::pow (2.0, (n - zA->root + cents / 100.0) / 12.0) * zA->sampleRate / sr;
            posA = 1.0;
        }
        if (zB != nullptr)
        {
            incB = std::pow (2.0, (n - zB->root + cents / 100.0) / 12.0) * zB->sampleRate / sr;
            posB = 1.0;
        }
        const float dm = params.decayMul.load();
        sampleEnv = 1.0f;
        sampleDecay = dm >= 0.999f ? 1.0f : decayCoef (14.0 * dm * dm, sr);
        const float p = keyPan * 0.5f;
        panSL = std::sqrt (1.0f - p) ; panSR = std::sqrt (1.0f + p);
    }

    //---------------------------------------------------------------- texture
    const int texIdx = params.tex.load() - 1;
    if (texIdx >= 0)
        if ((zT = bank.texture (texIdx)) != nullptr)
        {
            incT = std::pow (2.0, (n - zT->root + cents / 100.0) / 12.0) * zT->sampleRate / sr;
            posT = 1.0;
            gainT = params.texLevel.load() * (0.4f + 0.6f * velEff) * 0.8f;
        }

    //---------------------------------------------------------------- couche harmonique
    numOsc = 0;
    harmGain = params.harm.load();
    double energy = 0.0;
    if (harmGain > 0.001f)
    {
        const double B = 0.00006 * std::pow (2.0, (n - 48.0) / 14.0) * (1.0 + metal * 60.0);
        const float tilt = juce::jlimit (0.55f, 3.0f, prof.tilt - (tone + varTone) * 0.6f - (vel - 0.5f) * 0.7f - metal * 0.5f);
        const double baseT60 = juce::jlimit (0.9, 26.0, 20.0 * std::pow (2.0, -(n - 21.0) / 20.0))
                             * prof.sustainMul * params.decayMul.load() * (1.0 + metal * 0.8);
        const int partials = juce::jlimit (1, maxPartials, (int) ((0.45 * sr) / f0));
        const float detune = prof.detuneCents * params.unison.load() * (0.8f + 0.4f * rng.nextFloat());
        const double warp = 1.0 + metal * 0.42;      // partiels de cloche

        for (int k = 1; k <= partials; ++k)
        {
            const double fk = std::pow ((double) k, warp) * f0 * std::sqrt (1.0 + B * k * k);
            if (fk >= 0.45 * sr) break;
            const double hammer = 0.25 + 0.75 * std::abs (std::sin (juce::MathConstants<double>::pi * k * prof.hammerPos));
            double a = hammer / std::pow ((double) k, (double) tilt);
            a *= std::exp (-(double) (k - 1) * (0.06 + (1.0 - vel) * 0.10) * (1.0 - tone * 0.4) * (1.0 - metal * 0.6));
            const double t60k = baseT60 / (1.0 + prof.hfDamping * (1.0 - metal * 0.85) * (k - 1) * (1.0 + (n - 21.0) / 88.0));

            for (int s = 0; s < numStrings; ++s)
            {
                const double c = (s == 0 ? -0.5 : 0.5) * detune;
                const double w = juce::MathConstants<double>::twoPi * fk * std::pow (2.0, c / 1200.0) / sr;
                const int i = numOsc++;
                const double ph = rng.nextDouble() * juce::MathConstants<double>::twoPi;
                oc[(size_t) i] = (float) std::cos (ph);
                os[(size_t) i] = (float) std::sin (ph);
                baseW[(size_t) i] = (float) w;
                const double strAmp = (s == 0 ? 0.62 : 0.38) * a;
                amp[(size_t) i] = (float) strAmp;
                dec[(size_t) i] = decayCoef (s == 0 ? t60k * 0.55 : t60k * 1.6, sr);
                const float p = juce::jlimit (-1.0f, 1.0f, keyPan + (s == 0 ? -0.18f : 0.18f) * width);
                panL[(size_t) i] = std::sqrt (0.5f * (1.0f - p));
                panR[(size_t) i] = std::sqrt (0.5f * (1.0f + p));
                energy += strAmp * strAmp;
            }
        }
        const float norm = (float) (0.22 / std::sqrt (juce::jmax (1.0e-9, energy)));
        for (int i = 0; i < numOsc; ++i) amp[(size_t) i] *= norm;
    }

    // sub
    {
        subW = (float) (juce::MathConstants<double>::twoPi * (f0 * 0.5) / sr);
        subC = 1.0f; subS = 0.0f;
        const float lowWeight = juce::jlimit (0.0f, 1.0f, (76.0f - n) / 40.0f);
        subAmp = (prof.subAmount * 0.5f + params.sub.load()) * lowWeight * 0.22f;
        subDec = decayCoef (juce::jlimit (1.5, 14.0, 9.0 * std::pow (2.0, -(n - 33.0) / 24.0)) * params.decayMul.load(), sr);
    }
    lastPitchMul = 1.0f;
    setPitch (std::pow (2.0f, (bendSemis * 100.0f + params.pitchMod.load()) / 1200.0f));

    // bruit de feutre (couche harmonique seulement)
    noiseAmp = prof.noise * params.hammer.load() * vel * vel * 0.08f * harmGain;
    noiseDec = decayCoef (0.035 + (1.0 - vel) * 0.02, sr);
    const float nc = juce::jmin (prof.noiseCutoff * (0.7f + vel * 0.6f) * (1.0f + (n - 60.0f) / 60.0f), (float) sr * 0.4f);
    noiseCoef   = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * juce::jmax (80.0f, nc) / (float) sr);
    noiseHpCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 120.0f / (float) sr);
    nLp1 = nLp2 = nHp = 0.0f;

    // anneau : porteuse inharmonique (son metallique)
    ringInc = f0 * 1.4142 / sr;
    ringPhase = 0.0;

    // nappe
    padTarget = prof.padAmount * layer * 2.0f * (0.45f + 0.55f * vel) * 0.10f;
    padSine = prof.padSine;
    padLevel = 0.0f;
    padAtkCoef = 1.0f - std::exp (-1.0f / (prof.padAttack * 0.35f * (float) sr));
    padRelCoef = 1.0f;
    {
        const double pf = f0 * prof.padOctave;
        const double dets[5] = { -6.0, -2.5, 0.0, 2.5, 6.0 };
        for (int i = 0; i < 5; ++i)
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

    voiceGain = 1.0f;
    attackRamp = 0.0f;
    attackInc  = 1.0f / (juce::jmax (0.0015f, params.attack.load()) * (float) sr);
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
        zA = zB = zT = nullptr;
    }
}

void PianoVoice::renderNextBlock (juce::AudioBuffer<float>& out, int start, int num)
{
    if (! isVoiceActive())
        return;

    auto* L = out.getWritePointer (0);
    auto* R = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;

    // hauteur : pitch bend + constellation
    const float pitchMul = std::pow (2.0f, (bendSemis * 100.0f + params.pitchMod.load()) / 1200.0f);
    if (std::abs (pitchMul - lastPitchMul) > 1.0e-5f)
        setPitch (pitchMul);
    const double dA = incA * pitchMul, dB = incB * pitchMul, dT = incT * pitchMul;
    const double dRing = ringInc * pitchMul;
    const float ring = juce::jlimit (0.0f, 1.0f, params.ring.load());

    for (int i = start; i < start + num; ++i)
    {
        float l = 0.0f, r = 0.0f;

        // --- piano echantillonne ---
        if (zA != nullptr)
        {
            float a, b; zA->read (posA, a, b);
            l += a * gainA * sampleEnv; r += b * gainA * sampleEnv;
            posA += dA;
            if (posA >= zA->length - 3) zA = nullptr;
        }
        if (zB != nullptr)
        {
            float a, b; zB->read (posB, a, b);
            l += a * gainB * sampleEnv; r += b * gainB * sampleEnv;
            posB += dB;
            if (posB >= zB->length - 3) zB = nullptr;
        }
        sampleEnv *= sampleDecay;
        l *= panSL; r *= panSR;

        // --- couche harmonique ---
        float hl = 0.0f, hr = 0.0f;
        for (int k = 0; k < numOsc; ++k)
        {
            const float c = oc[(size_t) k], s = os[(size_t) k];
            const float nc = c * cw[(size_t) k] - s * sw[(size_t) k];
            const float ns = c * sw[(size_t) k] + s * cw[(size_t) k];
            oc[(size_t) k] = nc; os[(size_t) k] = ns;
            const float v = ns * amp[(size_t) k];
            amp[(size_t) k] *= dec[(size_t) k];
            hl += v * panL[(size_t) k];
            hr += v * panR[(size_t) k];
        }
        if (noiseAmp > 1.0e-6f)
        {
            const float w = rng.nextFloat() * 2.0f - 1.0f;
            nLp1 += noiseCoef * (w - nLp1);
            nLp2 += noiseCoef * (nLp1 - nLp2);
            nHp  += noiseHpCoef * (nLp2 - nHp);
            const float v = (nLp2 - nHp) * noiseAmp;
            noiseAmp *= noiseDec;
            hl += v; hr += v;
        }
        l += hl * harmGain; r += hr * harmGain;

        // --- texture metal ---
        if (zT != nullptr)
        {
            float a, b; zT->read (posT, a, b);
            l += a * gainT; r += b * gainT;
            posT += dT;
            if (posT >= zT->length - 3) zT = nullptr;
        }

        // --- modulation en anneau (metal) ---
        if (ring > 0.001f)
        {
            const float car = std::sin ((float) ringPhase * juce::MathConstants<float>::twoPi);
            ringPhase += dRing; if (ringPhase >= 1.0) ringPhase -= 1.0;
            const float m = 1.0f - ring + ring * car * 1.4f;
            l *= m; r *= m;
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

        // --- nappe ---
        if (padTarget > 0.0f)
        {
            if (! releasing) padLevel += padAtkCoef * (padTarget - padLevel);
            else             padLevel *= padRelCoef;

            float pl = 0.0f, pr = 0.0f;
            static const float pan5[5] = { 1.0f, 0.8f, 0.6f, 0.35f, 0.15f };
            for (int p = 0; p < 5; ++p)
            {
                double& ph = padPhase[(size_t) p];
                const double dt = padInc[(size_t) p] * pitchMul;
                const float saw  = (float) (2.0 * ph - 1.0) - polyBlep (ph, dt);
                const float tri  = 2.0f * std::abs (2.0f * (float) ph - 1.0f) - 1.0f;
                const float sine = std::sin ((float) ph * juce::MathConstants<float>::twoPi);
                const float v = (saw * 0.5f + tri * 0.5f) * (1.0f - padSine) + sine * padSine;
                ph += dt; if (ph >= 1.0) ph -= 1.0;
                pl += v * pan5[p] * 0.55f;
                pr += v * pan5[4 - p] * 0.55f;
            }
            padLp1L += padCoef * (pl - padLp1L); padLp2L += padCoef * (padLp1L - padLp2L);
            padLp1R += padCoef * (pr - padLp1R); padLp2R += padCoef * (padLp1R - padLp2R);
            l += padLp2L * padLevel * padPanL;
            r += padLp2R * padLevel * padPanR;
        }

        // --- enveloppes ---
        if (attackRamp < 1.0f) attackRamp = juce::jmin (1.0f, attackRamp + attackInc);
        if (releasing) voiceGain *= releaseCoef;
        const float g = voiceGain * attackRamp;

        L[i] += l * g;
        if (R != nullptr) R[i] += r * g;
    }

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

    // la note est-elle eteinte ?
    float peak = subAmp + noiseAmp;
    for (int k = 0; k < numOsc; ++k) peak += amp[(size_t) k] * harmGain;
    if (zA != nullptr || zB != nullptr) peak += sampleEnv * (gainA + gainB);
    if (zT != nullptr) peak += gainT;
    peak *= voiceGain;
    const float padPeak = padLevel * voiceGain;

    if ((peak < 2.0e-5f && (padTarget <= 0.0f || releasing) && padPeak < 2.0e-5f) || voiceGain < 1.0e-5f)
    {
        clearCurrentNote();
        numOsc = 0;
        zA = zB = zT = nullptr;
    }
}
