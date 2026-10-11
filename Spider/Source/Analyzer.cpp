#include "Analyzer.h"

void Analyzer::prepare (double sampleRate)
{
    sr = sampleRate;
    factor = juce::jmax (1, (int) std::round (sr / 11025.0));
    dsr = sr / factor;
    tauMin = juce::jmax (2, (int) (dsr / 1500.0));
    tauMax = juce::jmin (maxBuf - W - 2, (int) (dsr / 55.0));
    hop = juce::jmax (16, (int) (dsr * 0.0058));
    hopDurS = (float) (hop / dsr);
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
    lastRmsDb = -100.0f; lastNoteSample = -1000000;
    recentDb.fill (-100.0f); recentPos = 0;

    // pYIN : seuils 0.01..1.00 ponderes par une loi beta(2, 11.3) (moyenne 0.15)
    float tot = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        const float s = 0.01f * (float) (i + 1);
        betaW[(size_t) i] = s * std::pow (juce::jmax (0.0f, 1.0f - s), 10.33f);
        tot += betaW[(size_t) i];
    }
    for (auto& w : betaW) w /= tot;
    frames = {};
    frameCount = 0;
}

void Analyzer::resetTracking()
{
    curNote = pendingNote = -1.0f; pendingCount = unvoicedCount = 0; medN = 0;
    lastRmsDb = -100.0f; lastNoteSample = -1000000;
    recentDb.fill (-100.0f); recentPos = 0;
    frames = {};
    frameCount = 0;
}

// difference moyenne cumulee normalisee (coeur de YIN)
void Analyzer::cmndf()
{
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
    float run = 0.0f;
    diff[0] = 1.0f;
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        run += diff[(size_t) tau];
        diff[(size_t) tau] = run > 0.0f ? diff[(size_t) tau] * (float) tau / run : 1.0f;
    }
}

// pYIN : chaque creux de la courbe recoit la probabilite d'etre "le premier creux sous le seuil",
// sommee sur toute la distribution des seuils. Le reste = probabilite "pas de note".
void Analyzer::candidates (Frame& f)
{
    struct Dip { float tau, val; };
    Dip dips[48]; int nd = 0;
    for (int tau = juce::jmax (tauMin, 2); tau < tauMax && nd < 48; ++tau)
    {
        const float a = diff[(size_t) tau - 1], b = diff[(size_t) tau], c = diff[(size_t) tau + 1];
        if (b < a && b <= c && b < 1.0f)
        {
            float t = (float) tau;
            const float den = a + c - 2.0f * b;
            if (std::abs (den) > 1.0e-9f) t += 0.5f * (a - c) / den;
            dips[nd++] = { t, b };
        }
    }
    float prob[48] {};
    for (int i = 0; i < 100; ++i)
    {
        const float s = 0.01f * (float) (i + 1);
        for (int j = 0; j < nd; ++j)
            if (dips[j].val < s) { prob[j] += betaW[(size_t) i]; break; }
    }
    // garder les meilleurs candidats ; les creux proches (meme note) sont fusionnes
    f.n = 0;
    float pv = 0.0f;
    for (int pass = 0; pass < maxCand; ++pass)
    {
        int best = -1;
        for (int j = 0; j < nd; ++j) if (prob[j] > 0.01f && (best < 0 || prob[j] > prob[best])) best = j;
        if (best < 0) break;
        const float midi = 69.0f + 12.0f * std::log2 ((float) dsr / dips[best].tau / 440.0f);
        int same = -1;
        for (int k = 0; k < f.n; ++k) if (std::abs (f.midi[k] - midi) < 0.35f) same = k;
        if (same >= 0) { f.logp[same] = std::log (std::exp (f.logp[same]) + prob[best]); }
        else if (midi > 30.0f && midi < 92.0f)
        {
            f.midi[f.n] = midi; f.logp[f.n] = std::log (prob[best]); ++f.n;
        }
        pv += prob[best];
        prob[best] = 0.0f;
    }
    // une trame trop faible n'est pas une note
    const float loud = juce::jlimit (0.0f, 1.0f, (f.rmsDb + 50.0f) / 6.0f);
    for (int k = 0; k < f.n; ++k) f.logp[k] += std::log (juce::jmax (1.0e-4f, loud));
    f.logp[maxCand] = std::log (juce::jlimit (1.0e-4f, 1.0f, 1.0f - pv * loud));
}

// Viterbi : la hauteur la plus coherente dans le temps ; decision prise 'lag' trames plus tard
bool Analyzer::viterbiPush (Frame& f, Frame& decided, int& state)
{
    const int cur = (int) (frameCount % slots);
    Frame& pf = frames[(size_t) ((frameCount + slots - 1) % slots)];
    auto stateMidi = [] (const Frame& fr, int s) { return s < fr.n ? fr.midi[s] : -1.0f; };
    auto allStates = [] (const Frame& fr, int* list) { int c = 0; for (int s = 0; s < fr.n; ++s) list[c++] = s; list[c++] = maxCand; return c; };

    int lc[maxCand + 1], lp[maxCand + 1];
    const int nc = allStates (f, lc);
    const int np = frameCount > 0 ? allStates (pf, lp) : 0;
    float bestAll = -1.0e30f;
    for (int a = 0; a < nc; ++a)
    {
        const int s = lc[a];
        const float ms = stateMidi (f, s);
        float best = -1.0e30f; int arg = maxCand;
        if (np == 0) best = 0.0f;
        for (int b = 0; b < np; ++b)
        {
            const int q = lp[b];
            const float mq = stateMidi (pf, q);
            float tr;
            if (ms < 0.0f && mq < 0.0f) tr = 0.0f;
            else if (ms < 0.0f || mq < 0.0f) tr = -2.5f;
            else tr = -0.35f * juce::jmin (12.0f, std::abs (ms - mq));
            const float v = pf.score[q] + tr;
            if (v > best) { best = v; arg = q; }
        }
        f.score[s] = best + f.logp[s];
        f.back[s] = arg;
        bestAll = juce::jmax (bestAll, f.score[s]);
    }
    for (int a = 0; a < nc; ++a) f.score[lc[a]] -= bestAll;     // normalisation
    frames[(size_t) cur] = f;
    ++frameCount;
    if (frameCount <= lag) return false;

    // retour en arriere de 'lag' trames depuis le meilleur etat courant
    int s = maxCand; float bs = -1.0e30f;
    for (int a = 0; a < nc; ++a) if (f.score[lc[a]] > bs) { bs = f.score[lc[a]]; s = lc[a]; }
    int64_t idx = frameCount - 1;
    for (int k = 0; k < lag; ++k)
    {
        s = frames[(size_t) (idx % slots)].back[s];
        --idx;
    }
    decided = frames[(size_t) (idx % slots)];
    state = s;
    return true;
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
    // chaque gamme est jugee par 2 profils (Krumhansl, EDMA) : on garde le meilleur des deux
    std::array<float, 24> score; score.fill (-2.0f);
    for (int set = 0; set < 2; ++set)
        for (int mode = 0; mode < 2; ++mode)
        {
            const auto& prof = set == 0 ? Theory::profile (mode == 1) : Theory::profileEdma (mode == 1);
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
                auto& sc = score[(size_t) (mode * 12 + t)];
                sc = juce::jmax (sc, r);
            }
        }
    float best = -2, second = -2; Theory::Key bestK;
    for (int k = 0; k < 24; ++k)
    {
        const float r = score[(size_t) k];
        if (r > best) { second = best; best = r; bestK = { k % 12, k >= 12 }; }
        else if (r > second) second = r;
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
        if (bestK.tonic == candidate.tonic && bestK.minor == candidate.minor) candidateTime += 0.25f;
        else { candidate = bestK; candidateTime = 0.0f; }
        if (candidateTime >= 1.5f) { currentKey = bestK; confidence = conf; candidateTime = 0.0f; }
        else confidence = 0.97f * confidence + 0.03f * conf;
    }
}

// decoupage en notes, a partir de la hauteur lissee
void Analyzer::segment (const Frame& f, bool voiced, float m, std::vector<NoteEvent>& out)
{
    const int64_t onset = f.sample - (int64_t) ((W / 2 + hop) * factor);
    auto emit = [&] (float midi, bool v, int extraHops)
    {
        const int64_t start = onset - (int64_t) (juce::jmax (0, extraHops - 1) * hop * factor);
        if (start - lastNoteSample < (int64_t) (0.04 * sr)) return;
        if (out.size() >= out.capacity()) return;          // jamais d'allocation dans le fil audio
        NoteEvent e; e.startSample = start; e.midi = midi; e.f0 = 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
        e.level = f.rms; e.voiced = v;
        out.push_back (e);
        lastNoteSample = start;
        noteRate += 0.25f;
        if (v) { avgMidi = 0.9f * avgMidi + 0.1f * midi; ++heard; }
    };

    voiced = voiced && f.rmsDb > -48.0f;
    if (voiced)
    {
        unvoicedCount = 0;
        lastMidi = m;
        const int pc = (((int) std::lround (m)) % 12 + 12) % 12;
        chroma[(size_t) pc] += f.rms;                       // poids = duree x intensite

        const bool attack = f.rmsDb - lastRmsDb > 9.0f;
        if (curNote < 0.0f || std::abs (m - curNote) > 0.7f)
        {
            // nouvelle hauteur : 3 trames lissees concordantes ; la 1re (a cheval sur l'ancienne note) est ecartee
            if (pendingNote < 0.0f || std::abs (m - pendingNote) > 0.6f) { pendingNote = m; pendingSum = 0.0f; pendingCount = 1; }
            else
            {
                pendingSum += m; ++pendingCount;
                pendingNote = pendingSum / (float) (pendingCount - 1);
                if (pendingCount >= 3) { curNote = pendingNote; pendingNote = -1.0f; emit (curNote, true, pendingCount); }
            }
        }
        else
        {
            curNote = 0.85f * curNote + 0.15f * m;
            if (attack) emit (curNote, true, 0);
        }
    }
    else
    {
        if (++unvoicedCount >= 3) { curNote = -1.0f; pendingNote = -1.0f; }
        // attaque d'un accord / d'un son complexe : montee de 9 dB sur les ~30 dernieres ms
        float lo = 0.0f; for (float v : recentDb) lo = juce::jmin (lo, v);
        if (f.rmsDb > -40.0f && f.rmsDb - lo > 9.0f) emit (lastMidi, false, 0);
    }
    lastRmsDb = f.rmsDb;
    recentDb[(size_t) (recentPos++ % (int) recentDb.size())] = f.rmsDb;
}

void Analyzer::process (const float* mono, int n, int64_t firstSample, std::vector<NoteEvent>& out)
{
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
        Frame f;
        float rms = 0;
        for (int k = 0; k < W; ++k) { const float v = ring[(size_t) ((ringPos - W + k + maxBuf) % maxBuf)]; rms += v * v; }
        f.rms = std::sqrt (rms / W);
        f.rmsDb = juce::Decibels::gainToDecibels (f.rms, -100.0f);
        f.sample = firstSample + i;
        activity = activity * 0.995f + (f.rmsDb > -50.0f ? 0.005f : 0.0f);
        noteRate *= std::exp (-hopDurS / 4.0f);

        if (f.rmsDb > -55.0f) { cmndf(); candidates (f); }
        else { f.n = 0; f.logp[maxCand] = 0.0f; }

        Frame d; int st = maxCand;
        if (viterbiPush (f, d, st))
            segment (d, st < d.n, st < d.n ? d.midi[st] : 0.0f, out);

        // fuite lente de l'histogramme de hauteur
        const float dec = std::exp (-hopDurS / 25.0f);
        for (int k = 0; k < 12; ++k) chroma[(size_t) k] *= dec;
        if (++keyTimer * hopDurS >= 0.25f) { keyTimer = 0; updateKey(); }
    }
}
