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

void HomeKeysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    const int typeIdx = (int) pType->load();
    const auto& prof = getProfile ((PianoType) typeIdx);

    engine.type     = typeIdx;
    engine.tone     = pTone->load();
    engine.velocity = pVelocity->load();
    engine.release  = pRelease->load();
    engine.layer    = pLayer->load();
    engine.width    = pWidth->load();
    engine.octave   = juce::roundToInt (pOctave->load());

    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    // le synthe travaille toujours en stereo
    auto& work = workBuffer;
    work.setSize (2, n, false, false, true);
    work.clear();
    synth.renderNextBlock (work, midi, 0, n);

    // Grain (cassette, bande, lo-fi)
    processGrain (work, n);

    juce::dsp::AudioBlock<float> block (work);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // Couleur (filtre)
    const float fc = juce::jlimit (600.0f, 20000.0f, prof.lpCutoff * std::pow (2.0f, pTone->load() * 1.3f));
    cutoff.setTargetValue (juce::jmin (fc, (float) getSampleRate() * 0.45f));
    toneFilter.setCutoffFrequency (cutoff.skip (n));
    toneFilter.process (ctx);

    // Chorus
    const float ch = pChorus->load();
    if (ch > 0.001f)
    {
        chorus.setDepth (juce::jlimit (0.0f, 1.0f, 0.15f + prof.chorusDepth * ch));
        chorus.setMix (ch * 0.5f);
        chorus.process (ctx);
    }

    // Reverb
    juce::dsp::Reverb::Parameters rp;
    const float rv = pReverb->load();
    rp.roomSize   = 0.45f + pSize->load() * 0.53f;
    rp.damping    = prof.reverbDamp;
    rp.wetLevel   = rv * 0.45f;
    rp.dryLevel   = 1.0f - rv * 0.4f;
    rp.width      = 0.5f + pWidth->load() * 0.5f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    if (rv > 0.001f)
        reverb.process (ctx);

    // Vinyle (apres la reverb : le disque "joue" la piece)
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
// GRAIN
//==============================================================================
void HomeKeysProcessor::processGrain (juce::AudioBuffer<float>& buf, int n)
{
    const float sr = (float) getSampleRate();
    wowAmt.setTargetValue (pWow->load());
    driveAmt.setTargetValue (pDrive->load());
    crushAmt.setTargetValue (pCrush->load());

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
