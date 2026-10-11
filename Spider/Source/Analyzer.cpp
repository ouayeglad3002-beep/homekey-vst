#include "Analyzer.h"

void Analyzer::prepare (double sampleRate)
{
    sr = sampleRate;
    factor = juce::jmax (1, (int) std::round (sr / 11025.0));
    dsr = sr / factor;
    tauMin = juce::jmax (2, (int) (dsr / 1500.0));
    tauMax = juce::jmin (maxBuf - W - 2, (int) (dsr / 55.0));
    hop = juce::jmax (16, (int) (dsr * 0.0058));
    diff.assign ((size_t) tauMax + 2, 0.0f);

    auto c = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, juce::jmin (4400.0, dsr * 0.4), 0.707f);
    aa1.coefficients = c; aa2.coefficients = c;
    aa1.reset(); aa2.reset();
    ring.fill (0.0f);
    ringPos = hopCount = decCount = 0;
    medN = 0; curNote = pendingNote = -1.0f; pendingCount = unvoicedCount = 0;
    chroma.fill (0.0f); fftChroma.fill (0.0f);
    fftIn.assign (4096, 0.0f); fftWork.assign (8192, 0.0f); window.resize (4096);
    for (int i = 0; i < 4096; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / 4095.0f);
    fftFill = 0;
    confidence = 0.0f; keyTimer = 0; candidateTime = 0.0f;
    noteRate = 0.0f; activity = 0.0f; avgMidi = 64.0f; heard = 0;
}

float Analyzer::yin (float& clarity)
{
    // dernieres W + tauMax valeurs, dans l'ordre
    const int need = W + tauMax;
    float buf[maxBuf];
    for (int i = 0; i < need; ++i)
        buf[i] = ring[(size_t) ((ringPos - need + i + maxBuf) % maxBuf)];

    for (int tau = 1; tau <= tauMax; ++tau)
    {
        float s = 0.0f;
        for (int j = 0; j < W; ++j) { const float d = buf[j] - buf[j + tau]; s += d * d; }
        diff[(size_t) tau] = s;
    }
    // difference moyenne cumulee normalisee
    float run = 0.0f;
    diff[0] = 1.0f;
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        run += diff[(size_t) tau];
        diff[(size_t) tau] = run > 0.0f ? diff[(size_t) tau] * (float) tau / run : 1.0f;
    }
    int best = -1;
    for (int tau = tauMin; tau < tauMax; ++tau)
        if (diff[(size_t) tau] < 0.15f)
        {
            while (tau + 1 < tauMax && diff[(size_t) tau + 1] < diff[(size_t) tau]) ++tau;
            best = tau; break;
        }
    if (best < 0)
    {
        float m = 1.0e9f;
        for (int tau = tauMin; tau < tauMax; ++tau) if (diff[(size_t) tau] < m) { m = diff[(size_t) tau]; best = tau; }
    }
    clarity = 1.0f - juce::jlimit (0.0f, 1.0f, diff[(size_t) best]);
    float t = (float) best;
    if (best > 1 && best < tauMax - 1)
    {
        const float a = diff[(size_t) best - 1], b = diff[(size_t) best], c = diff[(size_t) best + 1];
        const float den = a + c - 2.0f * b;
        if (std::abs (den) > 1.0e-9f) t += 0.5f * (a - c) / den;
    }
    return (float) dsr / t;
}

void Analyzer::updateKey()
{
    std::array<float, 12> comb {};
    float s1 = 0, s2 = 0;
    for (int i = 0; i < 12; ++i) { s1 += chroma[(size_t) i]; s2 += fftChroma[(size_t) i]; }
    if (s1 + s2 < 1.0e-4f) return;
    for (int i = 0; i < 12; ++i)
        comb[(size_t) i] = (s1 > 0 ? 0.65f * chroma[(size_t) i] / s1 : 0.0f) + (s2 > 0 ? 0.35f * fftChroma[(size_t) i] / s2 : 0.0f);

    float mean = 0; for (float v : comb) mean += v; mean /= 12.0f;
    float best = -2, second = -2; Theory::Key bestK;
    for (int mode = 0; mode < 2; ++mode)
    {
        const auto& prof = Theory::profile (mode == 1);
        float pm = 0; for (float v : prof) pm += v; pm /= 12.0f;
        for (int t = 0; t < 12; ++t)
        {
            float num = 0, da = 0, db = 0;
            for (int i = 0; i < 12; ++i)
            {
                const float a = comb[(size_t) ((i + t) % 12)] - mean, b = prof[(size_t) i] - pm;
                num += a * b; da += a * a; db += b * b;
            }
            const float r = num / std::sqrt (juce::jmax (1.0e-12f, da * db));
            if (r > best) { second = best; best = r; bestK = { t, mode == 1 }; }
            else if (r > second) second = r;
        }
    }
    const float conf = juce::jlimit (0.0f, 1.0f, (best - 0.35f) * 1.7f) * juce::jlimit (0.25f, 1.0f, (best - second) * 12.0f);
    const bool same = bestK.tonic == currentKey.tonic && bestK.minor == currentKey.minor;
    if (confidence < 0.05f || same)
    {
        currentKey = bestK;
        confidence = conf;
        candidateTime = 0.0f;
    }
    else
    {
        // hysteresis : une nouvelle gamme doit tenir 1,5 s avant d'etre adoptee
        if (bestK.tonic == candidate.tonic && bestK.minor == candidate.minor) candidateTime += 0.25f;
        else { candidate = bestK; candidateTime = 0.0f; }
        if (candidateTime >= 1.5f) { currentKey = bestK; confidence = conf; candidateTime = 0.0f; }
        else confidence = 0.97f * confidence + 0.03f * conf;
    }
}

void Analyzer::process (const float* mono, int n, int64_t firstSample, std::vector<NoteEvent>& out)
{
    const float hopDur = (float) (hop / dsr);
    for (int i = 0; i < n; ++i)
    {
        const float x = mono[i];
        // --- chroma FFT (domaine plein) ---
        fftIn[(size_t) fftFill++] = x;
        if (fftFill == 4096)
        {
            std::fill (fftWork.begin(), fftWork.end(), 0.0f);
            for (int k = 0; k < 4096; ++k) fftWork[(size_t) k] = fftIn[(size_t) k] * window[(size_t) k];
            fft.performFrequencyOnlyForwardTransform (fftWork.data());
            std::array<float, 12> frame {};
            float tot = 0;
            for (int b = 4; b < 2048; ++b)
            {
                const float f = (float) (b * sr / 4096.0);
                if (f < 80.0f || f > 4000.0f) continue;
                const int pc = (((int) std::lround (12.0f * std::log2 (f / 440.0f)) + 69) % 12 + 12) % 12;
                const float m = std::sqrt (fftWork[(size_t) b]);
                frame[(size_t) pc] += m; tot += m;
            }
            const float dec = std::exp (-(4096.0f / (float) sr) / 25.0f);
            for (int k = 0; k < 12; ++k) fftChroma[(size_t) k] = fftChroma[(size_t) k] * dec + (tot > 0 ? frame[(size_t) k] / tot : 0.0f);
            fftFill = 0;
        }

        // --- decimation ---
        const float y = aa2.processSample (aa1.processSample (x));
        if (++decCount < factor) continue;
        decCount = 0;
        ring[(size_t) ringPos] = y;
        ringPos = (ringPos + 1) % maxBuf;
        if (++hopCount < hop) continue;
        hopCount = 0;

        // --- une analyse toutes les ~6 ms ---
        float rms = 0;
        for (int k = 0; k < W; ++k) { const float v = ring[(size_t) ((ringPos - W + k + maxBuf) % maxBuf)]; rms += v * v; }
        rms = std::sqrt (rms / W);
        const float rmsDb = juce::Decibels::gainToDecibels (rms, -100.0f);
        activity = activity * 0.995f + (rmsDb > -50.0f ? 0.005f : 0.0f);
        noteRate *= std::exp (-hopDur / 4.0f);
        const int64_t nowSample = firstSample + i;
        const int64_t onset = nowSample - (int64_t) ((W / 2 + hop) * factor);

        float clarity = 0.0f;
        const float f0 = rmsDb > -55.0f ? yin (clarity) : 0.0f;
        const bool voiced = f0 > 50.0f && f0 < 1600.0f && clarity > 0.78f && rmsDb > -48.0f;

        auto emit = [&] (float midi, bool v, int extraHops)
        {
            const int64_t start = onset - (int64_t) (juce::jmax (0, extraHops - 1) * hop * factor);
            if (start - lastNoteSample < (int64_t) (0.04 * sr)) return;
            NoteEvent e; e.startSample = start; e.midi = midi; e.f0 = 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
            e.level = rms; e.voiced = v;
            out.push_back (e);
            lastNoteSample = start;
            noteRate += 0.25f;
            if (v) { avgMidi = 0.9f * avgMidi + 0.1f * midi; ++heard; }
        };

        if (voiced)
        {
            unvoicedCount = 0;
            const float m = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
            medBuf[(size_t) (medN++ % 3)] = m;
            float a = medBuf[0], b = medBuf[1], c = medBuf[2];
            const float med = medN < 3 ? m : juce::jmax (juce::jmin (a, b), juce::jmin (juce::jmax (a, b), c));
            lastMidi = med;
            (void) med;
            // poids dans la gamme (note entendue)
            const int pc = (((int) std::lround (med)) % 12 + 12) % 12;
            chroma[(size_t) pc] += rms;

            const bool attack = rmsDb - lastRmsDb > 9.0f;
            if (curNote < 0.0f || std::abs (m - curNote) > 0.7f)
            {
                // nouvelle hauteur : 3 analyses concordantes, hauteur = moyenne des 3 (valeurs brutes)
                if (pendingNote < 0.0f || std::abs (m - pendingNote) > 0.5f) { pendingNote = m; pendingSum = m; pendingCount = 1; }
                else
                {
                    pendingSum += m; ++pendingCount;
                    pendingNote = pendingSum / (float) pendingCount;
                    if (pendingCount >= 3) { curNote = pendingNote; pendingNote = -1.0f; emit (curNote, true, pendingCount); }
                }
            }
            else
            {
                curNote = 0.85f * curNote + 0.15f * m;
                if (attack) emit (curNote, true, 0);       // note repetee
            }
        }
        else
        {
            if (++unvoicedCount >= 3) { curNote = -1.0f; pendingNote = -1.0f; }
            if (rmsDb > -40.0f && rmsDb - lastRmsDb > 10.0f) emit (lastMidi, false, 0);   // accord / percussion : mode sur
        }
        lastRmsDb = rmsDb;

        // fuite lente de l'histogramme de hauteur
        const float dec = std::exp (-hopDur / 25.0f);
        for (int k = 0; k < 12; ++k) chroma[(size_t) k] *= dec;
        if (++keyTimer * hopDur >= 0.25f) { keyTimer = 0; updateKey(); }
    }
}
