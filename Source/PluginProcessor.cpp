#include "PluginProcessor.h"
#include "PluginEditor.h"

static float param (juce::AudioProcessorValueTreeState& s, const char* id) { return s.getRawParameterValue (id)->load(); }

juce::AudioProcessorValueTreeState::ParameterLayout HomeKeysProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto f = [&p] (const char* id, const char* name, float lo, float hi, float def, float skew = 1.0f, const char* label = "")
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.0f, skew), def,
                                                            AudioParameterFloatAttributes().withLabel (label)));
    };

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "type", 1 }, "Monde", getTypeNames(), 0));
    f ("volume", "Volume", -24.0f, 6.0f, 0.0f, 1.0f, "dB");
    f ("tone", "Tone", -1.0f, 1.0f, 0.0f);
    f ("velocity", "Velocity", 0.0f, 1.0f, 0.75f);
    f ("release", "Release", 0.05f, 6.0f, 0.6f, 0.4f, "s");
    f ("layer", "Nappe", 0.0f, 1.0f, 0.3f);
    f ("chorus", "Chorus", 0.0f, 1.0f, 0.15f);
    f ("reverb", "Reverb", 0.0f, 1.0f, 0.35f);
    f ("size", "Size", 0.0f, 1.0f, 0.7f);
    f ("width", "Width", 0.0f, 1.0f, 0.75f);
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "octave", 1 }, "Octave", -2, 2, 0));
    f ("drive", "Drive", 0.0f, 1.0f, 0.0f);
    f ("wow", "Wow", 0.0f, 1.0f, 0.0f);
    f ("crush", "Crush", 0.0f, 1.0f, 0.0f);
    f ("vinyl", "Vinyl", 0.0f, 1.0f, 0.0f);

    // --- SOURCES ---
    f ("piano", "Piano", 0.0f, 1.0f, 1.0f);
    f ("harm", "Harmonic", 0.0f, 1.0f, 0.25f);
    f ("metal", "Metal", 0.0f, 1.0f, 0.0f);
    f ("ring", "Ring", 0.0f, 1.0f, 0.0f);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "tex", 1 }, "Texture", SampleBank::textureNames(), 0));
    f ("texlvl", "Texture Level", 0.0f, 1.0f, 0.5f);

    // --- SYNTH ---
    f ("fcut", "Filter Cutoff", 20.0f, 20000.0f, 20000.0f, 0.25f, "Hz");
    f ("fres", "Filter Reso", 0.0f, 1.0f, 0.1f);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "ftype", 1 }, "Filter Type", StringArray { "LP", "BP", "HP" }, 0));
    f ("attack", "Attack", 0.001f, 2.0f, 0.002f, 0.35f, "s");
    f ("decay", "Decay", 0.15f, 3.0f, 1.0f, 0.6f, "x");
    f ("hammer", "Hammer", 0.0f, 3.0f, 1.0f);
    f ("sub", "Sub", 0.0f, 1.0f, 0.0f);
    f ("unison", "Unison", 0.0f, 6.0f, 1.0f, 0.6f);
    f ("fine", "Fine", -50.0f, 50.0f, 0.0f, 1.0f, "ct");
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "dtime", 1 }, "Delay Time",
                    StringArray { "1/16", "1/8", "1/8 D", "1/4", "1/4 D", "1/2" }, 1));
    f ("dfb", "Delay Feedback", 0.0f, 0.9f, 0.35f);
    f ("dmix", "Delay Mix", 0.0f, 1.0f, 0.0f);

    // --- ATMOSPHERE ---
    f ("shimmer", "Shimmer", 0.0f, 1.0f, 0.0f);
    f ("air", "Air", 0.0f, 1.0f, 0.0f);
    f ("hum", "Hum", 0.0f, 1.0f, 0.0f);

    // --- CONSTELLATION ---
    f ("cdepth", "Constellation Depth", 0.0f, 1.0f, 0.0f);
    f ("crate", "Constellation Rate", 0.02f, 4.0f, 0.25f, 0.4f, "Hz");
    f ("cchaos", "Constellation Chaos", 0.0f, 1.0f, 0.3f);
    f ("corbit", "Constellation Orbit", -1.0f, 1.0f, 0.0f);
    f ("cvar", "Variation", 0.0f, 1.0f, 0.0f);
    f ("cmut", "Mutation", 0.0f, 1.0f, 0.7f);
    static const float sx[8] = { 0.50f, 0.78f, 0.86f, 0.66f, 0.38f, 0.16f, 0.24f, 0.55f };
    static const float sy[8] = { 0.18f, 0.28f, 0.58f, 0.82f, 0.78f, 0.55f, 0.30f, 0.50f };
    static const int st[8] = { Constellation::Tone, Constellation::Filter, Constellation::Drive, Constellation::Wow,
                               Constellation::Crush, Constellation::Chorus, Constellation::Reverb, Constellation::Space };
    for (int i = 0; i < 8; ++i)
    {
        const auto s = "s" + String (i);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { s + "x", 1 }, "Star " + String (i + 1) + " X", NormalisableRange<float> (0.0f, 1.0f), sx[i]));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { s + "y", 1 }, "Star " + String (i + 1) + " Y", NormalisableRange<float> (0.0f, 1.0f), sy[i]));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { s + "t", 1 }, "Star " + String (i + 1) + " Cible", Constellation::targetNames(), st[i]));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { s + "w", 1 }, "Star " + String (i + 1) + " Forme", Constellation::shapeNames(), i % 2));
    }

    // --- CHOP ---
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "chop", 1 }, "Chop", false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "choptype", 1 }, "Chop Type", chopNames(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "choprate", 1 }, "Chop Rate", StringArray { "1/4", "1/8", "1/16", "1/32" }, 2));
    f ("chopmix", "Chop Mix", 0.0f, 1.0f, 1.0f);

    return { p.begin(), p.end() };
}

HomeKeysProcessor::HomeKeysProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HomeKeysState", createLayout()),
      presets (apvts)
{
    pType     = apvts.getRawParameterValue ("type");
    pVolume   = apvts.getRawParameterValue ("volume");
    pTone     = apvts.getRawParameterValue ("tone");
    pVelocity = apvts.getRawParameterValue ("velocity");
    pRelease  = apvts.getRawParameterValue ("release");
    pLayer    = apvts.getRawParameterValue ("layer");
    pChorus   = apvts.getRawParameterValue ("chorus");
    pReverb   = apvts.getRawParameterValue ("reverb");
    pSize     = apvts.getRawParameterValue ("size");
    pWidth    = apvts.getRawParameterValue ("width");
    pOctave   = apvts.getRawParameterValue ("octave");
    pDrive    = apvts.getRawParameterValue ("drive");
    pWow      = apvts.getRawParameterValue ("wow");
    pCrush    = apvts.getRawParameterValue ("crush");
    pVinyl    = apvts.getRawParameterValue ("vinyl");

    for (int i = 0; i < 48; ++i)
        synth.addVoice (new PianoVoice (engine, *bank));
    synth.addSound (new PianoSound());
    synth.setNoteStealingEnabled (true);

    presets.loadPreset (0);
}

bool HomeKeysProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void HomeKeysProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    const int maxBlock = juce::jmax (samplesPerBlock, 4096);
    workBuffer.setSize (2, maxBlock);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };
    toneFilter.prepare (spec);
    toneFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    toneFilter.setResonance (0.6f);
    chorus.prepare (spec);
    chorus.setCentreDelay (9.0f);
    chorus.setFeedback (0.05f);
    chorus.setRate (0.35f);
    reverb.prepare (spec);
    reverb.reset();

    synthFilter.prepare (spec);
    synthFilter.reset();
    synthCut.reset (sampleRate, 0.03);
    synthCut.setCurrentAndTargetValue (20000.0f);
    delayBuf.setSize (2, (int) (sampleRate * 4.5));
    delayBuf.clear();
    delayWrite = 0;
    chopBuf.setSize (2, (int) (sampleRate * 8.0));
    chopBuf.clear();
    chopWrite = 0;
    lastSlice = lastBeat = lastCycle = -1;

    shimBuf.setSize (1, 8192);
    shimBuf.clear();
    shimBlock.setSize (2, maxBlock);
    shimWrite = 0;
    shimVerb.prepare (spec);
    shimVerb.reset();
    juce::dsp::Reverb::Parameters sp;
    sp.roomSize = 0.9f; sp.damping = 0.3f; sp.wetLevel = 0.3f; sp.dryLevel = 0.0f; sp.width = 1.0f;
    shimVerb.setParameters (sp);

    wowDelay.prepare (spec);
    wowDelay.reset();
    for (auto* sv : { &wowAmt, &driveAmt, &crushAmt, &vinylAmt })
        sv->reset (sampleRate, 0.15);
    wowAmt.setCurrentAndTargetValue (pWow->load());
    driveAmt.setCurrentAndTargetValue (pDrive->load());
    crushAmt.setCurrentAndTargetValue (pCrush->load());
    vinylAmt.setCurrentAndTargetValue (pVinyl->load());

    volume.reset (sampleRate, 0.05);
    cutoff.reset (sampleRate, 0.05);
    volume.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pVolume->load()));
}

//==============================================================================
// CONSTELLATION 2.0
//==============================================================================
void HomeKeysProcessor::updateConstellation (int n)
{
    const double sr = getSampleRate();
    const float depth = juce::jlimit (0.0f, 1.0f, param (apvts, "cdepth") + modWheel.load() * (1.0f - param (apvts, "cdepth")));
    const float rate  = param (apvts, "crate");
    const float chaos = param (apvts, "cchaos");
    const float orbit = param (apvts, "corbit");
    const float dt = (float) (n / sr);

    float oa = orbitAngle.load() + orbit * 0.35f * dt;
    if (oa > juce::MathConstants<float>::twoPi) oa -= juce::MathConstants<float>::twoPi;
    if (oa < 0.0f) oa += juce::MathConstants<float>::twoPi;
    orbitAngle = oa;

    mod.fill (0.0f);
    for (int i = 0; i < numStars; ++i)
    {
        const auto s = "s" + juce::String (i);
        const float x = apvts.getRawParameterValue (s + "x")->load() - 0.5f;
        const float y = apvts.getRawParameterValue (s + "y")->load() - 0.5f;
        const int target = (int) apvts.getRawParameterValue (s + "t")->load();
        const int shape  = (int) apvts.getRawParameterValue (s + "w")->load();

        const float dist = juce::jmin (1.0f, std::sqrt (x * x + y * y) * 2.4f);                    // loin = intense
        float ang = std::atan2 (y, x) + oa;                                                         // l'orbite fait tourner
        ang = std::fmod (ang + juce::MathConstants<float>::pi * 3.0f, juce::MathConstants<float>::twoPi) / juce::MathConstants<float>::twoPi;
        const double r = rate * (0.3 + 1.6 * ang);                                                  // angle = vitesse

        const double before = starPhase[(size_t) i];
        starPhase[(size_t) i] += juce::MathConstants<double>::twoPi * r * dt;
        const bool wrapped = std::floor (before / juce::MathConstants<double>::twoPi) != std::floor (starPhase[(size_t) i] / juce::MathConstants<double>::twoPi);
        const double ph = starPhase[(size_t) i];

        // marche aleatoire (le "hasard coherent")
        if (modRng.nextFloat() < 0.02f + chaos * 0.06f)
            starWalkTarget[(size_t) i] = modRng.nextFloat() * 2.0f - 1.0f;
        starWalk[(size_t) i] += (0.004f + 0.02f * chaos) * (starWalkTarget[(size_t) i] - starWalk[(size_t) i]);
        if (wrapped) starHold[(size_t) i] = modRng.nextFloat() * 2.0f - 1.0f;

        float v = 0.0f;
        switch (shape)
        {
            case 0:  v = (float) std::sin (ph); break;
            case 1:  v = starWalk[(size_t) i] * 1.6f; break;
            case 2:  v = starHold[(size_t) i]; break;
            case 3:  v = std::tanh (5.0f * (float) std::sin (ph)); break;
            default: v = (float) (std::sin (ph) * std::sin (ph * 0.137 + i)); break;
        }
        v = v * (1.0f - chaos * 0.5f) + starWalk[(size_t) i] * chaos;
        starSmooth[(size_t) i] += 0.35f * (juce::jlimit (-1.0f, 1.0f, v) - starSmooth[(size_t) i]);   // anti-clics
        const float m = depth * dist * starSmooth[(size_t) i];
        starMod[(size_t) i] = m;
        if (target >= 0 && target < Constellation::NumTargets)
            mod[(size_t) target] += m;
    }
}

void HomeKeysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    using namespace Constellation;
    const int n = buffer.getNumSamples();
    buffer.clear();

    // --- tempo ---
    double bpm = 120.0, ppq = -1.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying())
                if (auto pq = pos->getPpqPosition()) ppq = *pq;
        }
    const double spb = getSampleRate() * 60.0 / juce::jlimit (30.0, 300.0, bpm);
    if (ppq < 0.0) ppq = internalBeats;
    internalBeats += n / spb;

    // --- MIDI : notes de chop retirees, molette captee ---
    filteredMidi.clear();
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if ((m.isNoteOn() || m.isNoteOff()) && m.getNoteNumber() >= chopKeyLow && m.getNoteNumber() < chopKeyLow + 8)
        {
            const int t = m.getNoteNumber() - chopKeyLow;
            if (m.isNoteOn()) keyChop = t;
            else if (keyChop == t) keyChop = -1;
            continue;
        }
        if (m.isController() && m.getControllerNumber() == 1)
            modWheel = m.getControllerValue() / 127.0f;
        filteredMidi.addEvent (m, meta.samplePosition);
    }

    // --- constellation ---
    updateConstellation (n);
    auto P = [this] (const char* id) { return param (apvts, id); };
    auto M = [this] (int t) { return mod[(size_t) t]; };
    effTone    = juce::jlimit (-1.0f, 1.0f, P ("tone")   + 0.7f * M (Tone));
    effCut     = juce::jlimit (20.0f, 20000.0f, P ("fcut") * std::pow (2.0f, 3.0f * M (Filter)));
    effReso    = juce::jlimit (0.0f, 1.0f, P ("fres")   + 0.6f * M (Reso));
    effDrive   = juce::jlimit (0.0f, 1.0f, P ("drive")  + 0.5f * M (Drive));
    effWow     = juce::jlimit (0.0f, 1.0f, P ("wow")    + 0.5f * M (Wow));
    effCrush   = juce::jlimit (0.0f, 1.0f, P ("crush")  + 0.4f * std::abs (M (Crush)));
    effChorus  = juce::jlimit (0.0f, 1.0f, P ("chorus") + 0.5f * M (Chorus));
    effReverb  = juce::jlimit (0.0f, 1.0f, P ("reverb") + 0.4f * M (Constellation::Reverb));
    effLayer   = juce::jlimit (0.0f, 1.0f, P ("layer")  + 0.5f * M (Pad));
    effWidth   = juce::jlimit (0.0f, 1.0f, P ("width")  + 0.4f * M (Space));
    effShimmer = juce::jlimit (0.0f, 1.0f, P ("shimmer") + 0.5f * M (Shimmer));
    effAir     = juce::jlimit (0.0f, 1.0f, P ("air")    + 0.5f * M (Air));
    effDelay   = juce::jlimit (0.0f, 1.0f, P ("dmix")   + 0.4f * M (Delay));
    effVolume  = juce::jlimit (0.2f, 1.5f, 1.0f + 0.45f * M (Volume));

    const int typeIdx = (int) pType->load();
    const auto& prof = getProfile ((PianoType) typeIdx);

    engine.type     = typeIdx;
    engine.tone     = effTone;
    engine.velocity = pVelocity->load();
    engine.release  = pRelease->load();
    engine.layer    = effLayer;
    engine.width    = effWidth;
    engine.octave   = juce::roundToInt (pOctave->load());
    engine.attack   = P ("attack");
    engine.decayMul = P ("decay");
    engine.hammer   = P ("hammer");
    engine.sub      = P ("sub");
    engine.unison   = P ("unison");
    engine.fine     = P ("fine");
    engine.piano    = P ("piano");
    engine.harm     = P ("harm");
    engine.metal    = juce::jlimit (0.0f, 1.0f, P ("metal") + 0.5f * M (Metal));
    engine.ring     = juce::jlimit (0.0f, 1.0f, P ("ring")  + 0.5f * M (Ring));
    engine.tex      = (int) P ("tex");
    engine.texLevel = juce::jlimit (0.0f, 1.0f, P ("texlvl") + 0.5f * M (Texture));
    engine.variation = P ("cvar");
    engine.pitchMod = 35.0f * M (Pitch);

    keyboardState.processNextMidiBuffer (filteredMidi, 0, n, true);

    auto& work = workBuffer;
    work.setSize (2, n, false, false, true);
    work.clear();
    synth.renderNextBlock (work, filteredMidi, 0, n);

    // activite (pour l'atmosphere)
    {
        const float pk = juce::jmax (work.getMagnitude (0, 0, n), work.getMagnitude (1, 0, n));
        const float rel = std::exp (-(float) n / (3.0f * (float) getSampleRate()));
        activity = juce::jmax (juce::jmin (1.0f, pk * 12.0f), activity * rel);
    }

    processGrain (work, n);
    processChop (work, n, ppq, spb);

    juce::dsp::AudioBlock<float> block (work);
    auto sub = block.getSubBlock (0, (size_t) n);
    juce::dsp::ProcessContextReplacing<float> ctx (sub);

    // Filtre synth
    {
        const int ft = (int) P ("ftype");
        const bool bypass = (ft == 0 && effCut > 19000.0f && effReso < 0.15f);
        if (! bypass)
        {
            synthFilter.setType (ft == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                               : ft == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                         : juce::dsp::StateVariableTPTFilterType::highpass);
            synthFilter.setResonance (0.5f + effReso * 6.0f);
            synthCut.setTargetValue (juce::jmin (effCut, (float) getSampleRate() * 0.45f));
            synthFilter.setCutoffFrequency (synthCut.skip (n));
            synthFilter.process (ctx);
            if (ft == 1) work.applyGain (0, n, 2.4f);
        }
        else
            synthCut.setCurrentAndTargetValue (20000.0f);
    }

    // couleur du monde
    const float fc = juce::jlimit (600.0f, 20000.0f, prof.lpCutoff * std::pow (2.0f, effTone * 1.3f) * (1.0f + P ("piano") * 0.6f));
    cutoff.setTargetValue (juce::jmin (fc, (float) getSampleRate() * 0.45f));
    toneFilter.setCutoffFrequency (cutoff.skip (n));
    toneFilter.process (ctx);

    if (effChorus > 0.001f)
    {
        chorus.setDepth (juce::jlimit (0.0f, 1.0f, 0.15f + prof.chorusDepth * effChorus));
        chorus.setMix (effChorus * 0.5f);
        chorus.process (ctx);
    }

    processDelay (work, n, spb);
    processShimmer (work, n);

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize   = 0.45f + pSize->load() * 0.53f;
    rp.damping    = prof.reverbDamp;
    rp.wetLevel   = effReverb * 0.45f;
    rp.dryLevel   = 1.0f - effReverb * 0.4f;
    rp.width      = 0.5f + effWidth * 0.5f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    if (effReverb > 0.001f)
        reverb.process (ctx);

    processAtmos (work, n);
    addVinyl (work, n);

    // volume + limiteur doux
    volume.setTargetValue (juce::Decibels::decibelsToGain (pVolume->load()) * effVolume);
    auto* L = work.getWritePointer (0);
    auto* R = work.getWritePointer (1);
    float peak = 0.0f;
    int wp = scopeWritePos.load();
    for (int i = 0; i < n; ++i)
    {
        if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) { L[i] = R[i] = 0.0f; }
        const float g = volume.getNextValue();
        L[i] = std::tanh (L[i] * g * 1.1f) / 1.1f;
        R[i] = std::tanh (R[i] * g * 1.1f) / 1.1f;
        const float m = 0.5f * (L[i] + R[i]);
        peak = juce::jmax (peak, std::abs (m));
        if ((i & 1) == 0) { scope[(size_t) wp] = m; wp = (wp + 1) % scopeSize; }
    }
    scopeWritePos = wp;
    outputLevel = juce::jmax (peak, outputLevel.load() * 0.9f);

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, work, 0, 0, n);
        buffer.copyFrom (1, 0, work, 1, 0, n);
    }
    else
    {
        buffer.copyFrom (0, 0, work, 0, 0, n);
        buffer.addFrom  (0, 0, work, 1, 0, n);
        buffer.applyGain (0.5f);
    }
}

//==============================================================================
// SHIMMER : octave au-dessus + grande reverb, reinjectee (son de reve)
//==============================================================================
void HomeKeysProcessor::processShimmer (juce::AudioBuffer<float>& buf, int n)
{
    if (effShimmer < 0.002f) { shimFb = 0; return; }
    const int size = shimBuf.getNumSamples();
    const double W = 2048.0;
    auto* sb = shimBuf.getWritePointer (0);
    auto* L = buf.getWritePointer (0); auto* R = buf.getWritePointer (1);
    shimBlock.setSize (2, n, false, false, true);
    auto* oL = shimBlock.getWritePointer (0); auto* oR = shimBlock.getWritePointer (1);
    const float fbAmt = 0.45f * effShimmer;

    auto rd = [&] (double d) {
        double p = shimWrite - d; while (p < 0) p += size;
        const int i0 = (int) p; const float fr = (float) (p - i0);
        return sb[i0 % size] + fr * (sb[(i0 + 1) % size] - sb[i0 % size]);
    };
    for (int i = 0; i < n; ++i)
    {
        sb[shimWrite] = 0.5f * (L[i] + R[i]) * effShimmer + std::tanh (shimFb * fbAmt);
        // decalage d'une octave : deux tetes de lecture en fondu
        shimPhase += 1.0 / W; if (shimPhase >= 1.0) shimPhase -= 1.0;
        const double p2 = std::fmod (shimPhase + 0.5, 1.0);
        const float s1 = std::sin ((float) shimPhase * juce::MathConstants<float>::pi);
        const float s2 = std::sin ((float) p2 * juce::MathConstants<float>::pi);
        const float w1 = s1 * s1, w2 = s2 * s2;   // fenetres complementaires (somme = 1)
        const float y = rd ((1.0 - shimPhase) * W + 2.0) * w1 + rd ((1.0 - p2) * W + 2.0) * w2;
        oL[i] = y; oR[i] = y;
        shimWrite = (shimWrite + 1) % size;
    }
    juce::dsp::AudioBlock<float> b (shimBlock);
    juce::dsp::ProcessContextReplacing<float> c (b);
    shimVerb.process (c);
    for (int i = 0; i < n; ++i)
    {
        shimFb = 0.5f * (oL[i] + oR[i]);
        L[i] += oL[i] * effShimmer * 0.9f;
        R[i] += oR[i] * effShimmer * 0.9f;
    }
}

//==============================================================================
// ATMOSPHERE : vent spatial (AIR) + neon des Backrooms (HUM)
//==============================================================================
void HomeKeysProcessor::processAtmos (juce::AudioBuffer<float>& buf, int n)
{
    const float hum = param (apvts, "hum");
    if ((effAir < 0.002f && hum < 0.002f) || activity < 1.0e-4f) return;
    const float sr = (float) getSampleRate();
    auto* L = buf.getWritePointer (0); auto* R = buf.getWritePointer (1);

    if (modRng.nextFloat() < 0.02f) airTarget = 250.0f + modRng.nextFloat() * 2600.0f;
    if (modRng.nextFloat() < 0.04f) humFlickerTarget = modRng.nextFloat() < 0.15f ? 0.25f + 0.5f * modRng.nextFloat() : 1.0f;
    const float humInc = 60.0f / sr;
    const float hlp = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 900.0f / sr);

    for (int i = 0; i < n; ++i)
    {
        airCenter += 0.00005f * (airTarget - airCenter);
        const float g = 2.0f * std::sin (juce::MathConstants<float>::pi * juce::jmin (airCenter, sr * 0.2f) / sr);
        float o[2];
        for (int c = 0; c < 2; ++c)
        {
            const float x = modRng.nextFloat() * 2.0f - 1.0f;
            // filtre d'etat (passe-bande) tres simple
            airLp[c] += g * airBp[c];
            const float hp = x - airLp[c] - 0.35f * airBp[c];
            airBp[c] += g * hp;
            o[c] = airBp[c] * effAir * 0.06f * activity;
        }

        float h = 0.0f;
        if (hum > 0.002f)
        {
            humPhase += humInc; if (humPhase >= 1.0) humPhase -= 1.0;
            const float ph = (float) humPhase * juce::MathConstants<float>::twoPi;
            const float raw = std::sin (ph) + 0.6f * std::sin (2 * ph) + 0.35f * std::sin (3 * ph) + 0.25f * std::sin (4 * ph);
            const float buzz = juce::jlimit (-1.0f, 1.0f, raw * 1.8f);       // ballast qui grésille
            humLp += hlp * (buzz - humLp);
            humFlicker += 0.002f * (humFlickerTarget - humFlicker);
            h = humLp * hum * 0.035f * humFlicker * juce::jmax (0.25f, activity);
        }
        L[i] += o[0] + h;
        R[i] += o[1] + h * 0.9f;
    }
}

//==============================================================================
// DELAY (ping-pong, synchronise)
//==============================================================================
void HomeKeysProcessor::processDelay (juce::AudioBuffer<float>& buf, int n, double spb)
{
    const float mix = effDelay;
    if (mix < 0.001f) return;
    static const double beats[6] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
    const int size = delayBuf.getNumSamples();
    const int d = juce::jlimit (1, size - 1, (int) (beats[(int) param (apvts, "dtime")] * spb));
    const float fb = param (apvts, "dfb");
    const float lp = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 4500.0f / (float) getSampleRate());
    auto* L = buf.getWritePointer (0);  auto* R = buf.getWritePointer (1);
    auto* dl = delayBuf.getWritePointer (0); auto* dr = delayBuf.getWritePointer (1);
    for (int i = 0; i < n; ++i)
    {
        const int rd = (delayWrite - d + size) % size;
        float yl = dl[rd], yr = dr[rd];
        delayLpL += lp * (yl - delayLpL); delayLpR += lp * (yr - delayLpR);
        dl[delayWrite] = 0.5f * (L[i] + R[i]) + delayLpR * fb;   // ping
        dr[delayWrite] = delayLpL * fb;                          // pong
        delayWrite = (delayWrite + 1) % size;
        L[i] += yl * mix * 0.7f;
        R[i] += yr * mix * 0.7f;
    }
}

//==============================================================================
// CHOP en temps reel
//==============================================================================
void HomeKeysProcessor::processChop (juce::AudioBuffer<float>& buf, int n, double ppqStart, double spb)
{
    int type = keyChop;
    if (type < 0 && apvts.getRawParameterValue ("chop")->load() > 0.5f)
        type = (int) param (apvts, "choptype");
    chopActive = type;

    static const double rates[4] = { 1.0, 0.5, 0.25, 0.125 };
    const double sliceBeats = rates[(int) param (apvts, "choprate")];
    const float mix = param (apvts, "chopmix");
    const int size = chopBuf.getNumSamples();
    const double R = juce::jmin ((double) size / 4.0, sliceBeats * spb);
    const float fade = (float) (getSampleRate() * 0.003);
    const float gainStep = 1.0f / (float) (getSampleRate() * 0.006);
    auto* L = buf.getWritePointer (0); auto* Rch = buf.getWritePointer (1);
    auto* cl = chopBuf.getWritePointer (0); auto* cr = chopBuf.getWritePointer (1);
    static const int pattern[16] = { 1,0,1,1, 0,1,0,1, 1,0,1,0, 1,1,0,1 };

    auto readAt = [&] (double pos, int ch) -> float
    {
        while (pos < 0) pos += size;
        const int i0 = (int) pos % size;
        const int i1 = (i0 + 1) % size;
        const float f = (float) (pos - std::floor (pos));
        const float* b = ch == 0 ? cl : cr;
        return b[i0] + f * (b[i1] - b[i0]);
    };

    int effType = type;
    for (int i = 0; i < n; ++i)
    {
        const float inL = L[i], inR = Rch[i];
        cl[chopWrite] = inL; cr[chopWrite] = inR;

        const double beatPos = ppqStart + i / spb;
        const long long slice = (long long) std::floor (beatPos / sliceBeats);
        const double t = (beatPos - (double) slice * sliceBeats) * spb;         // echantillons dans la tranche
        const long long beat = (long long) std::floor (beatPos);
        const long long cycle = (long long) std::floor (beatPos / 2.0);

        if (slice != lastSlice)
        {
            lastSlice = slice;
            sliceStartWrite = chopWrite;
            glitchMode = juce::Random::getSystemRandom().nextInt (6);
        }
        if (beat != lastBeat) { lastBeat = beat; beatAnchor = chopWrite; halfRead = chopWrite; }
        if (cycle != lastCycle) { lastCycle = cycle; tapeRead = chopWrite; }

        const float target = type >= 0 ? 1.0f : 0.0f;
        chopGain += juce::jlimit (-gainStep, gainStep, target - chopGain);
        if (type >= 0) effType = type;

        float oL = inL, oR = inR;
        if (chopGain > 0.0001f && effType >= 0)
        {
            const float edge = (float) juce::jmin (1.0, t / fade, (R - t) / fade);
            const float w = juce::jlimit (0.0f, 1.0f, edge);
            int mode = effType;
            if (mode == 7) // GLITCH : un mode au hasard par tranche
            {
                static const int g[6] = { -1, 2, 3, 0, 6, 2 };
                mode = g[glitchMode];
            }
            switch (mode)
            {
                case -1: break;
                case 0: { const float gt = t < R * 0.5 ? w : 0.0f; oL = inL * gt; oR = inR * gt; break; }               // GATE
                case 1: { const float gt = pattern[(int) (slice & 15)] ? w : 0.0f; oL = inL * gt; oR = inR * gt; break; } // PATTERN
                case 2: { const double posInBeat = (beatPos - (double) beat) * spb;                                       // STUTTER
                          const double rp = beatAnchor - R + std::fmod (posInBeat, R);
                          const float ww = (float) juce::jlimit (0.0, 1.0, juce::jmin (std::fmod (posInBeat, R) / fade, (R - std::fmod (posInBeat, R)) / fade));
                          oL = readAt (rp, 0) * ww; oR = readAt (rp, 1) * ww; break; }
                case 3: { const double rp = sliceStartWrite - 1.0 - t;                                                   // REVERSE
                          oL = readAt (rp, 0) * w; oR = readAt (rp, 1) * w; break; }
                case 4: { const double el = (beatPos - (double) cycle * 2.0);                                            // TAPE STOP
                          const double speed = juce::jmax (0.0, 1.0 - el / 1.5);
                          tapeRead += speed;
                          const float a = (float) juce::jmin (1.0, speed * 4.0);
                          oL = readAt (tapeRead, 0) * a; oR = readAt (tapeRead, 1) * a; break; }
                case 5: { halfRead += 0.5;                                                                               // HALF SPEED
                          const double posInBeat = (beatPos - (double) beat) * spb;
                          const float ww = (float) juce::jlimit (0.0, 1.0, juce::jmin (posInBeat / fade, (spb - posInBeat) / fade));
                          oL = readAt (halfRead, 0) * ww; oR = readAt (halfRead, 1) * ww; break; }
                case 6: { const double rp = sliceStartWrite - R + std::fmod (2.0 * t, R);                                 // OCTAVE UP
                          oL = readAt (rp, 0) * w; oR = readAt (rp, 1) * w; break; }
                default: break;
            }
            const float g = chopGain * mix;
            oL = inL * (1.0f - g) + oL * g;
            oR = inR * (1.0f - g) + oR * g;
        }
        L[i] = oL; Rch[i] = oR;
        chopWrite = (chopWrite + 1) % size;
    }
}

//==============================================================================
// GRAIN
//==============================================================================
void HomeKeysProcessor::processGrain (juce::AudioBuffer<float>& buf, int n)
{
    const float sr = (float) getSampleRate();
    wowAmt.setTargetValue (effWow);
    driveAmt.setTargetValue (effDrive);
    crushAmt.setTargetValue (effCrush);

    auto* L = buf.getWritePointer (0);
    auto* R = buf.getWritePointer (1);
    const double wowInc  = juce::MathConstants<double>::twoPi * 0.55 / sr;  // pleurage lent
    const double flutInc = juce::MathConstants<double>::twoPi * 6.5 / sr;   // scintillement rapide
    const float dcCoef = 1.0f - juce::MathConstants<float>::twoPi * 12.0f / sr;

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { L[i], R[i] };

        // --- WOW : bande / cassette qui ondule ---
        const float w = wowAmt.getNextValue();
        wowPhase += wowInc; if (wowPhase > juce::MathConstants<double>::twoPi) wowPhase -= juce::MathConstants<double>::twoPi;
        flutterPhase += flutInc; if (flutterPhase > juce::MathConstants<double>::twoPi) flutterPhase -= juce::MathConstants<double>::twoPi;
        if ((i & 255) == 0) wowDriftTarget = grainRng.nextFloat() * 2.0f - 1.0f;
        wowDrift += 0.0005f * (wowDriftTarget - wowDrift);
        const float mod = (float) (0.0032 * std::sin (wowPhase) + 0.00035 * std::sin (flutterPhase)) + 0.0012f * wowDrift;
        const float delay = 2.0f + w * (0.0058f + mod) * sr;
        for (int c = 0; c < 2; ++c)
        {
            wowDelay.pushSample (c, x[c]);
            x[c] = wowDelay.popSample (c, delay);
        }

        // --- DRIVE : saturation chaude de bande ---
        const float d = driveAmt.getNextValue();
        if (d > 0.001f)
        {
            const float k = 1.0f + d * 7.0f;
            const float makeup = 1.0f / std::pow (k, 0.78f);
            for (int c = 0; c < 2; ++c)
                x[c] = std::tanh (k * (x[c] + 0.15f * d * x[c] * x[c])) * makeup;
        }

        // --- CRUSH : grain numerique lo-fi (bits + frequence d'echantillonnage) ---
        const float cr = crushAmt.getNextValue();
        if (cr > 0.001f)
        {
            const float factor = 1.0f + cr * cr * 9.0f;
            holdPhase += 1.0f / factor;
            const bool take = holdPhase >= 1.0f;
            if (take) holdPhase -= 1.0f;
            const float steps = std::pow (2.0f, 15.0f - cr * 10.5f);
            for (int c = 0; c < 2; ++c)
            {
                if (take) holdValue[(size_t) c] = std::round (x[c] * steps) / steps;
                x[c] = x[c] + (holdValue[(size_t) c] - x[c]) * juce::jmin (1.0f, cr * 4.0f);
            }
        }

        // anti-continu
        for (int c = 0; c < 2; ++c)
        {
            const float y = x[c] - dcX[(size_t) c] + dcCoef * dcY[(size_t) c];
            dcX[(size_t) c] = x[c]; dcY[(size_t) c] = y;
            x[c] = y;
        }

        L[i] = x[0]; R[i] = x[1];
    }
}

void HomeKeysProcessor::addVinyl (juce::AudioBuffer<float>& buf, int n)
{
    vinylAmt.setTargetValue (pVinyl->load());
    if (vinylAmt.getCurrentValue() < 0.0005f && vinylAmt.getTargetValue() < 0.0005f)
        return;

    const float sr = (float) getSampleRate();
    auto* L = buf.getWritePointer (0);
    auto* R = buf.getWritePointer (1);
    const float hissCoef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 5000.0f / sr);
    const float crkCoef  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 2800.0f / sr);

    for (int i = 0; i < n; ++i)
    {
        const float v = vinylAmt.getNextValue();
        // souffle
        hissLp += hissCoef * ((grainRng.nextFloat() * 2.0f - 1.0f) - hissLp);
        float s = hissLp * v * 0.010f;
        // craquements
        if (grainRng.nextFloat() < v * 28.0f / sr)
        {
            crackleEnv = (0.05f + 0.25f * grainRng.nextFloat()) * v;
            crackleSign = grainRng.nextBool() ? 1.0f : -1.0f;
        }
        crackleLp += crkCoef * (crackleEnv * crackleSign - crackleLp);
        crackleEnv *= 0.55f;
        s += crackleLp;
        L[i] += s;
        R[i] += s * 0.92f + hissLp * v * 0.002f;
    }
}

//==============================================================================
int HomeKeysProcessor::getNumPrograms()                    { return juce::jmax (1, presets.getNumPresets()); }
int HomeKeysProcessor::getCurrentProgram()                 { return juce::jmax (0, presets.getCurrentIndex()); }
void HomeKeysProcessor::setCurrentProgram (int index)      { presets.loadPreset (index); }
const juce::String HomeKeysProcessor::getProgramName (int index) { return presets.getPresetName (index); }

void HomeKeysProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presets.getCurrentName(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HomeKeysProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            presets.setCurrentName (tree.getProperty ("presetName", "Init").toString());
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* HomeKeysProcessor::createEditor()
{
    return new HomeKeysEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HomeKeysProcessor();
}
