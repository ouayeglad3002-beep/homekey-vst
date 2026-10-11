#include "Engine.h"

using namespace Web;

//==============================================================================
//  VARIATION OUVERT MIROIR OCTAV HARM1 HARM2 ARPEG ORNEM GLISS STUTT REVER ECHO  LUMIE ESPACE ACCENT
static const Preset presets[numPresets] =
{
    { "SILK", "ameliore en douceur : ornements fins, une tierce discrete",
      { .25f, .50f, .00f, .05f, .22f, .00f, .00f, .25f, .25f, .00f, .00f, .12f, .58f, .25f, .30f }, 0.00f, 0, 0, false },
    { "ORB WEAVER", "tisse des harmonies : tierces et sixtes arpegees",
      { .30f, .50f, .00f, .00f, .60f, .40f, .50f, .15f, .20f, .00f, .00f, .20f, .60f, .40f, .25f }, 0.00f, 1, 4, false },
    { "BLACK WIDOW", "la melodie en miroir, sombre, avec reverse",
      { .30f, .60f, .85f, .30f, .30f, .35f, .00f, .20f, .30f, .10f, .45f, .30f, .15f, .50f, .40f }, 0.20f, 2, 1, false },
    { "TARANTULA", "decoupe rythmique : stutters, sauts d'octave, accents",
      { .40f, .50f, .00f, .35f, .00f, .30f, .00f, .00f, .00f, .75f, .35f, .10f, .65f, .15f, .80f }, 0.20f, 0, 0, false },
    { "DREAM CATCHER", "aerien : octave haute, glisses, echos qui montent",
      { .20f, .35f, .00f, .10f, .30f, .55f, .70f, .30f, .60f, .00f, .20f, .55f, .78f, .90f, .10f }, 0.00f, 1, 0, false },
    { "JUMPING SPIDER", "intervalles elargis, bonds et ornements joueurs",
      { .50f, .90f, .15f, .55f, .15f, .00f, .00f, .55f, .15f, .25f, .00f, .15f, .70f, .20f, .60f }, 0.05f, 0, 0, false },
    { "MIRROR WEB", "contour inverse, reverse et echos descendants",
      { .15f, .50f, 1.0f, .10f, .20f, .30f, .30f, .10f, .30f, .15f, .80f, .35f, .50f, .45f, .30f }, 0.00f, 0, 2, false },
    { "CATHEDRAL", "organum : quintes et octaves, glisses lents, immense",
      { .10f, .30f, .00f, .00f, .50f, .70f, .15f, .00f, .70f, .00f, .10f, .25f, .45f, 1.0f, .10f }, 0.00f, 5, 2, false },
    { "VENOM", "agressif : secondes et quartes, grain, stutter",
      { .55f, .70f, .30f, .30f, .50f, .40f, .20f, .30f, .10f, .50f, .30f, .25f, .80f, .20f, .90f }, 0.60f, 3, 5, false },
    { "INFINITE THREAD", "la toile se retisse a chaque boucle, sans fin",
      { .50f, .65f, .40f, .30f, .35f, .30f, .50f, .35f, .35f, .25f, .30f, .60f, .55f, .60f, .40f }, 0.05f, 0, 2, true },
};

const Preset& Web::preset (int i) { return presets[juce::jlimit (0, numPresets - 1, i)]; }

//==============================================================================
void Engine::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    L = (int) std::round (0.046 * sr);
    analyzer.prepare (sr);
    int sz = 1; while (sz < (int) (sr * 3.0)) sz <<= 1;
    hist.init (sz * 2); echoRing.init (sz); revRing.init (sz);
    for (auto& c : past) c.at = -1; pastPos = 0; echoNote = { -1, -1, -1 };
    for (auto& t : echoTap) { t.reset(); t.alwaysShift = false; }
    mono.assign ((size_t) maxBlock + 16, 0.0f);
    events.reserve (64);
    stutBuf.setSize (2, (int) sr + 16);
    stutBuf.clear();
    stutState = 0;
    for (auto* s : { &mainV, &h1V, &h2V, &echoV }) s->reset();
    h1Gain = h1Target = h2Gain = h2Target = 0.0f; accent = accentTarget = 1.0f;
    cmdHead = cmdTail = 0; hasActive = false; graceEnd = 0; h1Start = h2Start = -1;
    prevNoteSample = -10000000;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 1 };
    for (auto* f : { &lowL, &lowR, &highL, &highR }) { f->prepare (spec); f->reset(); }
    lastLight = -10.0f;
    juce::dsp::ProcessSpec spec2 { sr, (juce::uint32) maxBlock, 2 };
    reverb.prepare (spec2); reverb.reset();
    revCount = 0;
    levelIn = levelOut = 0.0f; levelGain = 1.0f; limEnv = 0.0f;
    if (morphT >= 1.0f) startMorphTo (presetIdx), morphT = 1.0f, cur = to;
}

void Engine::startMorphTo (int index)
{
    presetIdx = juce::jlimit (0, numPresets - 1, index);
    const auto& pr = preset (presetIdx);
    from = cur;
    for (int i = 0; i < NumParams; ++i) to[(size_t) i] = pr.p[i];
    to[NumParams] = pr.grit;
    nextH1 = pr.h1Type; nextH2 = pr.h2Type; nextEvolve = pr.evolve;
    morphT = 0.0f;
}

void Engine::setPreset (int index) { startMorphTo (index); }

//==============================================================================
// L'humeur : la gamme et le profil de la melodie orientent les fils
void Engine::updateMood()
{
    for (int i = 0; i < NumParams; ++i) eff[(size_t) i] = cur[(size_t) i];
    const auto k = analyzer.key();
    const float m = analyzer.keyConfidence();
    if (! k.minor)
    {
        eff[Light] += 0.12f * m;  eff[Ornament] *= 1.0f + 0.2f * m;  eff[Accent] += 0.08f * m;
    }
    else
    {
        eff[Light] -= 0.12f * m;  eff[Space] += 0.08f * m;  eff[Reverse] += 0.06f * m;
    }
    const float d = analyzer.density();
    if (d > 4.5f)
    {
        const float q = juce::jlimit (0.0f, 1.0f, (d - 4.5f) / 4.0f);
        eff[Stutter] *= 1.0f - 0.5f * q; eff[Ornament] *= 1.0f - 0.5f * q; eff[Arpeggio] *= 1.0f - 0.4f * q; eff[Glide] *= 1.0f - 0.5f * q;
    }
    else if (d < 2.0f)
    {
        const float q = juce::jlimit (0.0f, 1.0f, (2.0f - d) / 2.0f);
        eff[Echo] += 0.15f * q; eff[Glide] += 0.15f * q; eff[Space] += 0.08f * q;
    }
    for (auto& v : eff) v = juce::jlimit (0.0f, 1.0f, v);
    for (int i = 0; i < NumParams; ++i) shown[(size_t) i] = eff[(size_t) i];
    keyTonic = k.tonic; keyMinor = k.minor ? 1 : 0; keyConf = m; density = d;
}

//==============================================================================
// Recomposition d'une note : garder le deroule, changer le chemin
Engine::Cmd Engine::decide (const Analyzer::NoteEvent& e, double ppqAtNote, double spb)
{
    Cmd c;
    c.at = e.startSample;
    c.voiced = e.voiced;
    const auto key = analyzer.key();
    const bool major = ! key.minor;

    const long long step16 = (long long) std::floor (ppqAtNote * 4.0 + 0.25);   // double-croche la plus proche
    const int stepInCycle = (int) (((step16 % 32) + 32) % 32);
    const uint32_t cycle = evolve ? (uint32_t) (step16 / 32) : 0u;
    const bool downbeat = stepInCycle % 16 == 0, onBeat = stepInCycle % 4 == 0;
    const auto deg = Theory::degreeOf (e.midi, key);
    auto rnd = [&] (uint32_t salt) { return Theory::hash01 ((uint32_t) presetIdx * 977u + 13u, cycle, (uint32_t) stepInCycle * 131u + (uint32_t) (deg.deg & 0xff), salt); };

    c.accent = downbeat ? 1.0f + 0.35f * eff[Accent] : onBeat ? 1.0f + 0.12f * eff[Accent] : 1.0f - 0.25f * eff[Accent];
    c.glideMs = 4.0f + eff[Glide] * 40.0f;   // portamento court : la note se pose toujours juste
    c.window = juce::jlimit ((float) (0.015 * sr), (float) (0.045 * sr), 2.6f * (float) (sr / juce::jmax (40.0f, e.f0)));
    const int dir = (major ? 1 : -1) * (presetIdx == 6 ? -1 : 1);
    Viz v; v.origMidi = e.midi; v.step = (float) stepInCycle + (float) (ppqAtNote * 4.0 - (double) step16);
    v.voiced = e.voiced;
    v.at = e.startSample;

    if (! e.voiced)
    {
        // accords / percussions : seulement ce qui reste juste
        c.mainSemis = 0.0f;
        c.h1Gain = 0.0f;
        const bool oct = h2Type <= 1;
        c.h2Semis = oct ? (h2Type == 0 ? 12.0f : -12.0f) : 0.0f;
        c.h2Gain = oct ? eff[Harmony2] * 0.5f : 0.0f;
        if (rnd (12) < eff[Stutter] * 0.7f) { c.stutRepeats = 2 + (int) (rnd (13) * 2.0f); c.stutLen = (int) (spb / 4.0); c.stutReverse = rnd (14) < eff[Reverse]; }
        v.newMidi = e.midi;
        pushViz (v);
        return c;
    }

    ++notesSeen;
    const bool keySure = analyzer.keyConfidence() >= 0.3f && analyzer.notesHeard() >= 6;
    const bool phraseStart = (e.startSample - prevNoteSample) > (int64_t) (1.5 * spb);
    int newDeg = deg.deg;
    if (keySure && ! phraseStart && ! downbeat)
    {
        const int origInt = deg.deg - prevOrigDeg;
        const float expand = std::pow (2.0f, (eff[Expand] - 0.5f) * 2.0f);          // x0.5 .. x2
        int t = (int) std::lround (origInt * expand);
        if (rnd (1) < eff[Mirror]) t = -t;                                            // miroir du contour
        const float varP = eff[Variation] * (onBeat ? 0.5f : 1.0f);
        if (t == 0 && rnd (2) < varP * 0.6f) t = rnd (3) < 0.5f ? 1 : -1;             // casse les notes repetees
        else if (t != 0 && rnd (4) < varP)
        {
            const int s = t > 0 ? 1 : -1;
            t += (rnd (5) < 0.6f) ? s : (std::abs (t) > 1 ? -s : s);                   // garde la direction
        }
        newDeg = prevNewDeg + t;
        if (rnd (7) < eff[Octave] * (onBeat ? 0.6f : 1.0f))
            newDeg += (analyzer.registerMidi() > 66.0f ? -7 : 7);
        // gravite : ne pas s'eloigner du deroule d'origine
        newDeg = juce::jlimit (deg.deg - 7, deg.deg + 7, newDeg);
        if (std::abs (newDeg - deg.deg) > 4 && std::abs (newDeg - deg.deg) != 7) newDeg = deg.deg + (newDeg > deg.deg ? 4 : -4);
    }
    prevOrigDeg = deg.deg;
    prevNewDeg = newDeg;
    prevNoteSample = e.startSample;

    const float targetMidi = Theory::midiOfDegree (newDeg, key) + deg.offset;
    c.mainSemis = juce::jlimit (-14.0f, 14.0f, targetMidi - e.midi);
    if (newDeg != deg.deg) ++notesChanged;
    {
        // controle : la note produite reste dans la gamme (meme ecart d'intonation qu'a l'origine)
        const auto chk = Theory::degreeOf (e.midi + c.mainSemis, key);
        if (std::abs (chk.offset - deg.offset) > 0.05f) ++notesOutOfKey;
    }
    v.newMidi = e.midi + c.mainSemis;

    // harmonie I
    static const int h1d[6] = { 2, 2, -5, 1, 3, -7 };
    int d1 = h1d[h1Type];
    if (h1Type == 0) d1 = major ? 2 : -2;
    if (rnd (9) < 0.55f + 0.45f * eff[Harmony1] && eff[Harmony1] > 0.02f)
    {
        c.h1Semis = Theory::midiOfDegree (newDeg + d1, key) + deg.offset - e.midi;
        c.h1Gain = eff[Harmony1] * 0.7f;
        v.h1Midi = e.midi + c.h1Semis;
    }
    // harmonie II
    static const int h2d[6] = { 7, -7, 4, -3, 5, 3 };
    if (rnd (10) < 0.55f + 0.45f * eff[Harmony2] && eff[Harmony2] > 0.02f)
    {
        c.h2Semis = Theory::midiOfDegree (newDeg + h2d[h2Type], key) + deg.offset - e.midi;
        c.h2Gain = eff[Harmony2] * 0.6f;
        v.h2Midi = e.midi + c.h2Semis;
    }
    c.arp1 = (int) (eff[Arpeggio] * spb / 4.0);
    c.arp2 = (int) (eff[Arpeggio] * spb / 2.0);

    // ornement (appoggiature dans la gamme)
    if (rnd (11) < eff[Ornament] && analyzer.density() < 7.0f)
    {
        c.graceSemis = Theory::midiOfDegree (newDeg + (major ? 1 : -1), key) + deg.offset - e.midi;
        c.graceLen = (int) juce::jmin (spb / 8.0, 0.075 * sr);
        v.grace = true;
    }
    // stutter
    if (rnd (12) < eff[Stutter] * (onBeat ? 1.0f : 0.6f))
    {
        c.stutRepeats = 2 + (int) (rnd (13) * 3.0f);
        c.stutLen = (int) ((eff[Stutter] > 0.5f && rnd (15) < 0.5f) ? spb / 8.0 : spb / 4.0);
        c.stutReverse = rnd (14) < eff[Reverse];
        v.stutter = true;
    }
    // echo en spirale : chaque repetition monte (ou descend) d'un degre
    c.echoSemis = Theory::midiOfDegree (newDeg + dir, key) - Theory::midiOfDegree (newDeg, key);
    for (int k = 0; k < 3; ++k)
        c.echoK[k] = Theory::midiOfDegree (newDeg + dir * (k + 1), key) + deg.offset - e.midi;
    pushViz (v);
    return c;
}

void Engine::pushViz (const Viz& v)
{
    int s1, n1, s2, n2;
    vizFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) vizBuf[(size_t) s1] = v;
    vizFifo.finishedWrite (n1 + n2);
}

//==============================================================================
void Engine::process (juce::AudioBuffer<float>& buf, int64_t firstSample, double ppq, double spb, bool)
{
    const int n = buf.getNumSamples();
    const int nch = juce::jmin (2, buf.getNumChannels());
    auto* L0 = buf.getWritePointer (0);
    auto* R0 = buf.getWritePointer (nch > 1 ? 1 : 0);
    lastSpb = (float) spb;

    // --- morphing des fils (3 s) ---
    if (morphT < 1.0f)
    {
        morphT = juce::jmin (1.0f, morphT + (float) n / (float) (3.0 * sr));
        const float s = morphT * morphT * (3.0f - 2.0f * morphT);
        for (size_t i = 0; i < cur.size(); ++i) cur[i] = from[i] + (to[i] - from[i]) * s;
        if (morphT >= 0.5f) { h1Type = nextH1; h2Type = nextH2; evolve = nextEvolve; }
    }
    morph = morphT;
    updateMood();

    // --- ecoute ---
    if ((int) mono.size() < n) mono.resize ((size_t) n);
    for (int i = 0; i < n; ++i) mono[(size_t) i] = 0.5f * (L0[i] + R0[i]);
    events.clear();
    analyzer.process (mono.data(), n, firstSample, events);
    for (auto& e : events)
    {
        const double ppqAt = ppq + (double) (e.startSample - firstSample) / spb;
        auto c = decide (e, ppqAt, spb);
        if (((cmdTail + 1) & 63) != cmdHead) { cmds[(size_t) cmdTail] = c; cmdTail = (cmdTail + 1) & 63; }
    }

    // --- reglages de bloc ---
    const float light = eff[Light];
    if (std::abs (light - lastLight) > 0.01f)
    {
        lastLight = light;
        const float tilt = (light - 0.5f) * 14.0f;
        auto lo = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, 250.0, 0.7f, juce::Decibels::decibelsToGain (-tilt * 0.5f));
        auto hi = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 3500.0, 0.7f, juce::Decibels::decibelsToGain (tilt * 0.5f));
        lowL.coefficients = lo; lowR.coefficients = lo; highL.coefficients = hi; highR.coefficients = hi;
    }
    const float grit = cur[NumParams];
    const float gDrive = 1.0f + 5.0f * grit;
    const float echoMix = eff[Echo] * 0.6f, echoFb = 0.3f + 0.35f * eff[Echo];
    const double echoDelay = juce::jlimit (0.08 * sr, 1.2 * sr, spb * 0.75);
    const int revChunk = (int) juce::jlimit (0.06 * sr, 0.9 * sr, spb / 2.0);
    const float revMix = eff[Reverse] * 0.45f;
    const float kA = 1.0f - std::exp (-1.0f / (0.004f * (float) sr));
    const float kH = 1.0f - std::exp (-1.0f / (0.012f * (float) sr));
    float echoLp[2] = { echoLpState[0], echoLpState[1] };
    const float lpK = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 5000.0f / (float) sr);
    const int fade = 64;
    double blockInPow = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const int64_t t = firstSample + i;
        const int64_t outIdx = t - L;
        hist.push (L0[i], R0[i]);
        blockInPow += L0[i] * L0[i] + R0[i] * R0[i];

        // commandes arrivees a echeance
        while (cmdHead != cmdTail && cmds[(size_t) cmdHead].at <= outIdx)
        {
            active = cmds[(size_t) cmdHead]; hasActive = true;
            past[(size_t) pastPos] = active; pastPos = (pastPos + 1) % (int) past.size();
            cmdHead = (cmdHead + 1) & 63;
            const float gk = 1.0f - std::exp (-1.0f / (0.001f * active.glideMs * (float) sr));
            mainV.glideK = gk;
            for (auto* s : { &mainV, &h1V, &h2V }) s->setWindow (active.window);
            if (active.graceLen > 0) { mainV.setTarget (active.graceSemis); mainV.glideK = 0.02f; graceEnd = active.at + active.graceLen; }
            else { mainV.setTarget (active.mainSemis); graceEnd = 0; }
            h1Start = active.at + active.arp1; h2Start = active.at + active.arp2;
            if (active.h1Gain <= 0.0f) { h1Target = 0.0f; h1Start = -1; }
            if (active.h2Gain <= 0.0f) { h2Target = 0.0f; h2Start = -1; }
            accentTarget = active.accent;
            echoV.setTarget (active.echoSemis);
            echoV.glideK = 0.002f;
            if (active.stutRepeats > 0 && active.stutLen > 64 && active.stutLen < stutBuf.getNumSamples())
            {
                stutState = 1; stutPos = 0; stutLen = active.stutLen; stutRepeatsLeft = active.stutRepeats - 1;
                stutRepeatIdx = 0; stutRev = active.stutReverse;
            }
        }
        if (graceEnd > 0 && outIdx >= graceEnd) { mainV.setTarget (active.mainSemis); mainV.glideK = 1.0f - std::exp (-1.0f / (0.015f * (float) sr)); graceEnd = 0; }
        if (h1Start >= 0 && outIdx >= h1Start) { h1V.setTarget (active.h1Semis); h1V.logR = active.h1Semis / 12.0f; h1Target = active.h1Gain; h1Start = -1; }
        if (h2Start >= 0 && outIdx >= h2Start) { h2V.setTarget (active.h2Semis); h2V.logR = active.h2Semis / 12.0f; h2Target = active.h2Gain; h2Start = -1; }

        // --- voix ---
        float ml, mr, al = 0, ar = 0, bl = 0, br = 0;
        mainV.process (hist, (double) L, ml, mr);
        h1Gain += kH * (h1Target - h1Gain);
        h2Gain += kH * (h2Target - h2Gain);
        if (h1Gain > 1.0e-4f) h1V.process (hist, (double) L, al, ar);
        if (h2Gain > 1.0e-4f) h2V.process (hist, (double) L, bl, br);
        accent += kA * (accentTarget - accent);
        float vl = (ml + al * h1Gain * 0.8f + bl * h2Gain * 0.8f) * accent;
        float vr = (mr + ar * h1Gain * 0.8f + br * h2Gain * 0.8f) * accent;

        // --- stutter ---
        if (stutState == 1)
        {
            stutBuf.setSample (0, stutPos, vl); stutBuf.setSample (1, stutPos, vr);
            if (++stutPos >= stutLen) { stutState = stutRepeatsLeft > 0 ? 2 : 0; stutPos = 0; }
        }
        else if (stutState == 2)
        {
            const int idx = (stutRev && (stutRepeatIdx % 2 == 0)) ? stutLen - 1 - stutPos : stutPos;
            const float env = juce::jmin (1.0f, juce::jmin ((float) stutPos, (float) (stutLen - 1 - stutPos)) / (float) fade);
            vl = stutBuf.getSample (0, idx) * env;
            vr = stutBuf.getSample (1, idx) * env;
            if (++stutPos >= stutLen) { stutPos = 0; ++stutRepeatIdx; if (--stutRepeatsLeft <= 0) stutState = 0; }
        }

        // --- reverse delay (le passe joue a l'envers) ---
        float rl = 0, rr = 0;
        revRing.push (vl, vr);
        if (revMix > 0.002f)
        {
            for (int k = 0; k < 2; ++k)
            {
                const int64_t ph = (revCount + (int64_t) (k * revChunk / 2)) % revChunk;
                const float env = std::sin (juce::MathConstants<float>::pi * (float) ph / (float) revChunk);
                float a, b; revRing.read ((double) (2 * ph + 1), a, b);
                rl += a * env * env; rr += b * env * env;
            }
        }
        ++revCount;

        // --- echo en spirale : chaque repetition rejoue la note un degre plus loin, dans la gamme ---
        float el = 0, er = 0;
        if (echoMix > 0.002f)
        {
            for (int k = 0; k < 3; ++k)
            {
                const int64_t tapTime = outIdx - (int64_t) ((k + 1) * echoDelay);
                // quelle note jouait a ce moment-la ?
                int found = -1; int64_t bestAt = -1;
                for (int q = 0; q < (int) past.size(); ++q)
                    if (past[(size_t) q].at >= 0 && past[(size_t) q].at <= tapTime && past[(size_t) q].at > bestAt) { bestAt = past[(size_t) q].at; found = q; }
                if (found < 0 || ! past[(size_t) found].voiced) continue;
                if (found != echoNote[(size_t) k])
                {
                    echoNote[(size_t) k] = found;
                    echoTap[(size_t) k].setTarget (past[(size_t) found].echoK[k]);
                    echoTap[(size_t) k].logR = past[(size_t) found].echoK[k] / 12.0f;
                    echoTap[(size_t) k].setWindow (past[(size_t) found].window);
                }
                float a, b;
                echoTap[(size_t) k].process (hist, (double) L + (k + 1) * echoDelay, a, b);
                const float gk = std::pow (echoFb, (float) (k + 1)) * 1.6f;
                el += a * gk; er += b * gk;
            }
            echoLp[0] += lpK * (el - echoLp[0]); echoLp[1] += lpK * (er - echoLp[1]);
        }
        else { echoLp[0] *= 0.99f; echoLp[1] *= 0.99f; }

        float yl = vl + rl * revMix + echoLp[0] * echoMix;
        float yr = vr + rr * revMix + echoLp[1] * echoMix;
        if (testMainOnly) { yl = ml; yr = mr; }

        // --- lumiere ---
        yl = highL.processSample (lowL.processSample (yl));
        yr = highR.processSample (lowR.processSample (yr));
        // --- grain ---
        if (grit > 0.01f)
        {
            const float tn = std::tanh (gDrive);
            yl = yl + grit * (std::tanh (gDrive * yl) / tn - yl);
            yr = yr + grit * (std::tanh (gDrive * yr) / tn - yr);
        }
        L0[i] = yl; R0[i] = yr;
    }
    echoLpState[0] = echoLp[0]; echoLpState[1] = echoLp[1];

    // --- espace ---
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.55f + 0.42f * eff[Space];
    rp.damping = 0.35f + 0.3f * (1.0f - eff[Light]);
    rp.wetLevel = eff[Space] * 0.33f;
    rp.dryLevel = 1.0f - eff[Space] * 0.25f;
    rp.width = 1.0f;
    reverb.setParameters (rp);
    if (eff[Space] > 0.01f && ! testMainOnly)
    {
        juce::dsp::AudioBlock<float> blk (buf.getArrayOfWritePointers(), (size_t) nch, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (blk);
        reverb.process (ctx);
    }
    // --- meme volume qu'a l'entree (mesure lente ~2 s) ---
    {
        double po = 0.0;
        for (int i = 0; i < n; ++i) po += L0[i] * L0[i] + R0[i] * R0[i];
        const float k = std::exp (-(float) n / (2.0f * (float) sr));
        if (blockInPow / n > 1.0e-6 && po / n > 1.0e-6)
        {
            levelIn  = k * levelIn  + (1.0f - k) * (float) (blockInPow / n);
            levelOut = k * levelOut + (1.0f - k) * (float) (po / n);
        }
        const float target = levelOut > 1.0e-9f ? juce::jlimit (0.25f, 2.0f, std::sqrt (levelIn / levelOut)) : 1.0f;
        const float g0 = levelGain;
        levelGain += 0.05f * (target - levelGain);
        if (! testMainOnly)
            for (int ch = 0; ch < nch; ++ch) buf.applyGainRamp (ch, 0, n, g0, levelGain);
    }
    // --- limiteur de securite (-1 dB, relache 80 ms) ---
    const float limRel = std::exp (-1.0f / (0.08f * (float) sr));
    for (int i = 0; i < n; ++i)
    {
        float a = std::isfinite (L0[i]) ? L0[i] : 0.0f, b = std::isfinite (R0[i]) ? R0[i] : 0.0f;
        const float pk = juce::jmax (std::abs (a), std::abs (b));
        limEnv = juce::jmax (pk, limEnv * limRel);
        const float g = limEnv > 0.89f ? 0.89f / limEnv : 1.0f;
        L0[i] = a * g;
        if (nch > 1) R0[i] = b * g;
    }
    cyclePos = (float) std::fmod (std::fmod (ppq - (double) L / spb, 8.0) + 8.0, 8.0) / 8.0f;
}
