#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout HomeKeysProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "type", 1 }, "Piano", getTypeNames(), 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume",
                    NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tone",
                    NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "velocity", 1 }, "Velocity",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.75f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release",
                    NormalisableRange<float> (0.05f, 5.0f, 0.01f, 0.4f), 0.5f, AudioParameterFloatAttributes().withLabel ("s")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "layer", 1 }, "Layer",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "chorus", 1 }, "Chorus",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "reverb", 1 }, "Reverb",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "size", 1 }, "Size",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.6f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "width", 1 }, "Width",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    // --- GRAIN ---
    p.push_back (std::make_unique<AudioParameterInt> (ParameterID { "octave", 1 }, "Octave", -2, 2, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "wow", 1 }, "Wow",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "crush", 1 }, "Crush",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "vinyl", 1 }, "Vinyl",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    // --- SYNTH ---
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "fcut", 1 }, "Filter Cutoff",
                    NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 20000.0f, AudioParameterFloatAttributes().withLabel ("Hz")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "fres", 1 }, "Filter Reso",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.1f));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "ftype", 1 }, "Filter Type", StringArray { "LP", "BP", "HP" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack",
                    NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.35f), 0.002f, AudioParameterFloatAttributes().withLabel ("s")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 }, "Decay",
                    NormalisableRange<float> (0.15f, 3.0f, 0.01f, 0.6f), 1.0f, AudioParameterFloatAttributes().withLabel ("x")));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "hammer", 1 }, "Hammer",
                    NormalisableRange<float> (0.0f, 3.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sub", 1 }, "Sub",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "unison", 1 }, "Unison",
                    NormalisableRange<float> (0.0f, 6.0f, 0.01f, 0.6f), 1.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "fine", 1 }, "Fine",
                    NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("ct")));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "dtime", 1 }, "Delay Time",
                    StringArray { "1/16", "1/8", "1/8 D", "1/4", "1/4 D", "1/2" }, 1));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "dfb", 1 }, "Delay Feedback",
                    NormalisableRange<float> (0.0f, 0.9f, 0.01f), 0.35f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "dmix", 1 }, "Delay Mix",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

    // --- CONSTELLATION (LFO) ---
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "cdepth", 1 }, "Constellation Depth",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "crate", 1 }, "Constellation Rate",
                    NormalisableRange<float> (0.02f, 4.0f, 0.01f, 0.4f), 0.25f, AudioParameterFloatAttributes().withLabel ("Hz")));
    static const float sx[8] = { 0.50f, 0.78f, 0.86f, 0.66f, 0.38f, 0.16f, 0.24f, 0.55f };
    static const float sy[8] = { 0.18f, 0.28f, 0.58f, 0.82f, 0.78f, 0.55f, 0.30f, 0.50f };
    for (int i = 0; i < 8; ++i)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "s" + String (i) + "x", 1 }, "Star " + String (i + 1) + " X",
                        NormalisableRange<float> (0.0f, 1.0f, 0.001f), sx[i]));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "s" + String (i) + "y", 1 }, "Star " + String (i + 1) + " Y",
                        NormalisableRange<float> (0.0f, 1.0f, 0.001f), sy[i]));
    }

    // --- CHOP ---
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "chop", 1 }, "Chop", false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "choptype", 1 }, "Chop Type", chopNames(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "choprate", 1 }, "Chop Rate", StringArray { "1/4", "1/8", "1/16", "1/32" }, 2));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "chopmix", 1 }, "Chop Mix",
                    NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

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

    for (int i = 0; i < 24; ++i)
        synth.addVoice (new PianoVoice (engine));
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
    workBuffer.setSize (2, juce::jmax (samplesPerBlock, 4096));

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
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

static float param (juce::AudioProcessorValueTreeState& s, const char* id) { return s.getRawParameterValue (id)->load(); }

void HomeKeysProcessor::updateConstellation (int n)
{
    const float depth = param (apvts, "cdepth");
    const float rate  = param (apvts, "crate");
    const double sr = getSampleRate();
    sharedPhase += juce::MathConstants<double>::twoPi * rate * 0.31 * n / sr;

    for (int i = 0; i < numStars; ++i)
    {
        const float x = apvts.getRawParameterValue ("s" + juce::String (i) + "x")->load() - 0.5f;
        const float y = apvts.getRawParameterValue ("s" + juce::String (i) + "y")->load() - 0.5f;
        const float dist = juce::jmin (1.0f, std::sqrt (x * x + y * y) * 2.4f);       // eloignement = intensite
        const float ang  = (std::atan2 (y, x) + juce::MathConstants<float>::pi) / juce::MathConstants<float>::twoPi; // angle = vitesse
        const double r = rate * (0.35 + 1.4 * ang);
        starPhase[(size_t) i] += juce::MathConstants<double>::twoPi * r * n / sr;

        // marche aleatoire douce (aleatoire mais coherente)
        if (juce::Random::getSystemRandom().nextFloat() < 0.02f)
            starWalkTarget[(size_t) i] = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        starWalk[(size_t) i] += 0.01f * (starWalkTarget[(size_t) i] - starWalk[(size_t) i]);

        float v = 0.55f * (float) std::sin (starPhase[(size_t) i])
                + 0.25f * (float) std::sin (sharedPhase + i * 0.9)
                + 0.30f * starWalk[(size_t) i];
        v = juce::jlimit (-1.0f, 1.0f, v);
        starMod[(size_t) i] = depth * dist * v;
    }
}

void HomeKeysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    // --- tempo de l'hote ---
    double bpm = 120.0, ppq = -1.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying())
                if (auto pq = pos->getPpqPosition()) ppq = *pq;
        }
    const double spb = getSampleRate() * 60.0 / juce::jlimit (30.0, 300.0, bpm);
    if (ppq < 0.0) { ppq = internalBeats; }
    internalBeats += n / spb;

    // --- notes de chop (12..19) : retirees du piano ---
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
        filteredMidi.addEvent (m, meta.samplePosition);
    }

    // --- constellation ---
    updateConstellation (n);
    auto sm = [this] (int i) { return starMod[(size_t) i].load(); };
    effTone   = juce::jlimit (-1.0f, 1.0f, param (apvts, "tone") + 0.7f * sm (0));
    effCut    = juce::jlimit (20.0f, 20000.0f, param (apvts, "fcut") * std::pow (2.0f, 2.5f * sm (1)));
    effDrive  = juce::jlimit (0.0f, 1.0f, param (apvts, "drive")  + 0.5f * sm (2));
    effWow    = juce::jlimit (0.0f, 1.0f, param (apvts, "wow")    + 0.5f * sm (3));
    effCrush  = juce::jlimit (0.0f, 1.0f, param (apvts, "crush")  + 0.4f * std::abs (sm (4)));
    effChorus = juce::jlimit (0.0f, 1.0f, param (apvts, "chorus") + 0.5f * sm (5));
    effReverb = juce::jlimit (0.0f, 1.0f, param (apvts, "reverb") + 0.4f * sm (6));
    effLayer  = juce::jlimit (0.0f, 1.0f, param (apvts, "layer")  + 0.5f * sm (7));
    effWidth  = juce::jlimit (0.0f, 1.0f, param (apvts, "width")  + 0.4f * sm (7));

    const int typeIdx = (int) pType->load();
    const auto& prof = getProfile ((PianoType) typeIdx);

    engine.type     = typeIdx;
    engine.tone     = effTone;
    engine.velocity = pVelocity->load();
    engine.release  = pRelease->load();
    engine.layer    = effLayer;
    engine.width    = effWidth;
    engine.octave   = juce::roundToInt (pOctave->load());
    engine.attack   = param (apvts, "attack");
    engine.decayMul = param (apvts, "decay");
    engine.hammer   = param (apvts, "hammer");
    engine.sub      = param (apvts, "sub");
    engine.unison   = param (apvts, "unison");
    engine.fine     = param (apvts, "fine");

    keyboardState.processNextMidiBuffer (filteredMidi, 0, n, true);

    auto& work = workBuffer;
    work.setSize (2, n, false, false, true);
    work.clear();
    synth.renderNextBlock (work, filteredMidi, 0, n);

    // Grain (cassette, bande, lo-fi)
    processGrain (work, n);

    // Chop (avant les effets : la reverb reste fluide)
    processChop (work, n, ppq, spb);

    juce::dsp::AudioBlock<float> block (work);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // Filtre synth (LP / BP / HP)
    {
        const int ft = (int) param (apvts, "ftype");
        const bool bypass = (ft == 0 && effCut > 19000.0f);
        if (! bypass)
        {
            synthFilter.setType (ft == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                               : ft == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                         : juce::dsp::StateVariableTPTFilterType::highpass);
            synthFilter.setResonance (0.5f + param (apvts, "fres") * 6.0f);
            synthCut.setTargetValue (juce::jmin (effCut, (float) getSampleRate() * 0.45f));
            synthFilter.setCutoffFrequency (synthCut.skip (n));
            synthFilter.process (ctx);
            if (ft == 1) work.applyGain (1.6f);
        }
        else
            synthCut.setCurrentAndTargetValue (20000.0f);
    }

    // Couleur (filtre du piano)
    const float fc = juce::jlimit (600.0f, 20000.0f, prof.lpCutoff * std::pow (2.0f, effTone * 1.3f));
    cutoff.setTargetValue (juce::jmin (fc, (float) getSampleRate() * 0.45f));
    toneFilter.setCutoffFrequency (cutoff.skip (n));
    toneFilter.process (ctx);

    // Chorus
    if (effChorus > 0.001f)
    {
        chorus.setDepth (juce::jlimit (0.0f, 1.0f, 0.15f + prof.chorusDepth * effChorus));
        chorus.setMix (effChorus * 0.5f);
        chorus.process (ctx);
    }

    // Delay synchronise au tempo
    processDelay (work, n, spb);

    // Reverb
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

    // Vinyle
    addVinyl (work, n);

    // Volume + limiteur doux
    volume.setTargetValue (juce::Decibels::decibelsToGain (pVolume->load()));
    auto* L = work.getWritePointer (0);
    auto* R = work.getWritePointer (1);
    float peak = 0.0f;
    int wp = scopeWritePos.load();

    for (int i = 0; i < n; ++i)
    {
        const float g = volume.getNextValue();
        L[i] = std::tanh (L[i] * g * 1.1f) / 1.1f;
        R[i] = std::tanh (R[i] * g * 1.1f) / 1.1f;
        const float m = 0.5f * (L[i] + R[i]);
        peak = juce::jmax (peak, std::abs (m));
        if ((i & 1) == 0)
        {
            scope[(size_t) wp] = m;
            wp = (wp + 1) % scopeSize;
        }
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
// DELAY (ping-pong, synchronise)
//==============================================================================
void HomeKeysProcessor::processDelay (juce::AudioBuffer<float>& buf, int n, double spb)
{
    const float mix = param (apvts, "dmix");
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
