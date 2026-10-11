#include "Processor.h"
#include "Editor.h"

//==============================================================================
//                  nom       conseil                                   seuil  ratio      att   rel    knee rms   scHpf prog  up     squeeze color mix
static const Style styles[numStyles] =
{
    { "GLUE",   "colle le mix, bus et instruments",   -42.f, 2.f, 4.f,   10.f, 150.f,  6.f, true,  90.f, false, false, 0.50f, 0.35f, 1.00f },
    { "PUNCH",  "attaque qui claque : drums, 808",    -34.f, 4.f, 16.f,   3.f,  60.f,  2.f, false, 40.f, false, false, 0.60f, 0.50f, 1.00f },
    { "VELVET", "doux et chaud : voix, piano, pads",  -36.f, 3.f, 4.f,   12.f, 120.f, 10.f, true,  60.f, true,  false, 0.55f, 0.45f, 1.00f },
    { "SMASH",  "ecrase tout, sale et enorme",        -45.f, 20.f, 60.f, 0.1f,  45.f,  0.f, false, 0.f,  false, false, 0.70f, 0.60f, 0.60f },
    { "BLOOM",  "fait remonter les details : reve",   -40.f, 2.f, 4.f,   30.f, 350.f,  8.f, true,  0.f,  false, true,  0.60f, 0.45f, 0.85f },
};

const Style& getStyle (int i) { return styles[juce::jlimit (0, numStyles - 1, i)]; }

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout OneCompressProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    StringArray names;
    for (int i = 0; i < numStyles; ++i) names.add (styles[i].name);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style", names, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "squeeze", 1 }, "Squeeze", NormalisableRange<float> (0.0f, 1.0f), 0.45f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "color", 1 }, "Color", NormalisableRange<float> (0.0f, 1.0f), 0.30f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.0f, 1.0f), 1.0f));
    return { p.begin(), p.end() };
}

OneCompressProcessor::OneCompressProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OneCompress", createLayout())
{
}

bool OneCompressProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
}

void OneCompressProcessor::loadStylePreset (int s)
{
    const auto& st = getStyle (s);
    auto set = [this] (const char* id, float v) { if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set ("style", (float) s);
    set ("squeeze", st.squeeze);
    set ("color", st.color);
    set ("mix", st.mix);
    currentProgram = s;
}

void OneCompressProcessor::setCurrentProgram (int index) { loadStylePreset (index); }

void OneCompressProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) samplesPerBlock);
    const int lat = juce::roundToInt (os->getLatencyInSamples());
    setLatencySamples (lat);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    dryDelay.prepare (spec);
    dryDelay.setMaximumDelayInSamples (juce::jmax (8, lat + 8));
    dryDelay.setDelay ((float) lat);
    dryBuf.setSize (2, samplesPerBlock);

    scHpfL.reset(); scHpfR.reset();
    rmsState = 0; envDb = 0; slowDb = 0; resetMakeup = true;
    powIn = powOut = 0.0f; matchGain = 1.0f;
    lowState = highState = hpState = dcX = dcY = {};
    for (auto* s : { &squeezeS, &colorS, &mixS }) s->reset (sampleRate, 0.05);
    squeezeS.setCurrentAndTargetValue (apvts.getRawParameterValue ("squeeze")->load());
    colorS.setCurrentAndTargetValue (apvts.getRawParameterValue ("color")->load());
    mixS.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load());
    lastStyle = -1;
}

//==============================================================================
float OneCompressProcessor::computeGain (const Style& st, float L, float squeeze) const
{
    if (st.upward)
    {
        // BLOOM : remonte ce qui est faible (queues de notes, reverbs, details)
        const float T = -18.0f + (st.thrAtMax + 18.0f) * squeeze;
        const float R = st.ratioLo + (st.ratioHi - st.ratioLo) * squeeze;
        const float maxBoost = 3.0f + 13.0f * squeeze;
        float g = 0.0f;
        if (L < T) g = juce::jmin (maxBoost, (T - L) * (1.0f - 1.0f / R));
        // pas de remontee du bruit de fond (silence)
        g *= juce::jlimit (0.0f, 1.0f, (L + 72.0f) / 18.0f);
        // pointes legerement tenues
        if (L > T + 10.0f) g -= (L - T - 10.0f) * 0.33f;
        return g;
    }
    const float T = st.thrAtMax * squeeze;
    const float R = st.ratioLo + (st.ratioHi - st.ratioLo) * squeeze;
    const float W = st.kneeDb;
    const float x = L - T;
    if (W > 0.0f && std::abs (x) <= W * 0.5f)
        return (1.0f / R - 1.0f) * (x + W * 0.5f) * (x + W * 0.5f) / (2.0f * W);
    return x > 0.0f ? (1.0f / R - 1.0f) * x : 0.0f;
}

//==============================================================================
// COULEUR (2x sur-echantillonnee : saturation propre, sans aliasing)
//==============================================================================
void OneCompressProcessor::applyColor (juce::dsp::AudioBlock<float>& block, int style, float c)
{
    if (c < 0.001f && style != 3) return;
    const float osr = (float) (sr * 2.0);
    auto onePole = [osr] (float hz) { return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hz / osr); };
    const float kLow = onePole (160.0f), kHigh = onePole (11000.0f), kEx = onePole (3500.0f);
    const float dcK = 1.0f - juce::MathConstants<float>::twoPi * 8.0f / osr;

    for (size_t ch = 0; ch < juce::jmin<size_t> (2, block.getNumChannels()); ++ch)
    {
        auto* d = block.getChannelPointer (ch);
        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            const float x = d[i];
            float y = x;
            switch (style)
            {
                case 0: // GLUE : console, rondeur douce
                {
                    const float g = 2.0f, b = 0.08f;
                    const float s = (std::tanh (g * x + b) - std::tanh (b)) / g;
                    y = x + c * 0.8f * (s - x);
                    break;
                }
                case 1: // PUNCH : transistor FET, harmoniques impaires mordantes
                {
                    const float g = 2.5f + 3.0f * c;
                    const float u = g * x;
                    const float s = u / std::pow (1.0f + std::pow (std::abs (u), 2.5f), 0.4f) / g;
                    y = x + c * (s - x) * 1.1f;
                    break;
                }
                case 2: // VELVET : lampe, harmoniques paires + chaleur
                {
                    const float g = 1.8f;
                    const float u = g * x;
                    const float s = (std::tanh (u + 0.3f * u * u) ) / g;
                    y = x + c * 0.9f * (s - x);
                    lowState[ch]  += kLow * (y - lowState[ch]);
                    highState[ch] += kHigh * (y - highState[ch]);
                    y = y + c * 0.35f * lowState[ch];                        // bas chauds
                    y = y + c * 0.45f * (highState[ch] - y);                  // aigus adoucis
                    break;
                }
                case 3: // SMASH : saturation lourde, presque fuzz
                {
                    const float g = 1.5f + 9.0f * c;
                    float u = g * x;
                    u = u / (1.0f + std::abs (u));                           // ecrasement
                    u = std::tanh (2.2f * u) / std::tanh (2.2f);
                    y = x + (0.35f + 0.65f * c) * (u * 0.85f - x);
                    break;
                }
                default: // BLOOM : excitateur, air brillant
                {
                    hpState[ch] += kEx * (x - hpState[ch]);
                    const float hi = x - hpState[ch];
                    const float ex = std::tanh (4.0f * hi) * 0.25f;
                    const float s = std::tanh (1.3f * x) / 1.3f;
                    y = x + c * (0.5f * (s - x) + 0.9f * ex);
                    break;
                }
            }
            // anti-continu (la saturation asymetrique en cree)
            const float o = y - dcX[ch] + dcK * dcY[ch];
            dcX[ch] = y; dcY[ch] = o;
            d[i] = o;
        }
    }
}

//==============================================================================
void OneCompressProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = juce::jmin (2, buffer.getNumChannels());
    if (n == 0 || nch == 0) return;

    const int style = (int) apvts.getRawParameterValue ("style")->load();
    const auto& st = getStyle (style);
    if (style != lastStyle)
    {
        if (st.scHpfHz > 0.0f)
        {
            auto c = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, st.scHpfHz, 0.707f);
            scHpfL.coefficients = c; scHpfR.coefficients = c;
        }
        scHpfL.reset(); scHpfR.reset();
        lastStyle = style;
        currentProgram = style;
        resetMakeup = true;
    }
    squeezeS.setTargetValue (apvts.getRawParameterValue ("squeeze")->load());
    colorS.setTargetValue (apvts.getRawParameterValue ("color")->load());
    mixS.setTargetValue (apvts.getRawParameterValue ("mix")->load());

    // copie seche (alignee sur la latence du sur-echantillonnage)
    dryBuf.setSize (2, n, false, false, true);
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* src = buffer.getReadPointer (juce::jmin (ch, nch - 1));
        auto* dst = dryBuf.getWritePointer (ch);
        for (int i = 0; i < n; ++i) { dryDelay.pushSample (ch, src[i]); dst[i] = dryDelay.popSample (ch); }
    }

    float inPeak = 0.0f;
    for (int ch = 0; ch < nch; ++ch) inPeak = juce::jmax (inPeak, buffer.getMagnitude (ch, 0, n));

    // --- compression ---
    const float attMs = st.attackMs, relMs = st.releaseMs;
    const float aAtt = std::exp (-1.0f / (0.001f * attMs * (float) sr));
    const float rmsK = std::exp (-1.0f / (0.010f * (float) sr));
    const float slowK = std::exp (-1.0f / (1.0f * (float) sr));
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (nch > 1 ? 1 : 0);
    float minGr = 0.0f, maxGr = 0.0f;
    const float sq0 = squeezeS.getCurrentValue();
    // compensation automatique de volume : suit la reduction moyenne (lente, sans pompage)
    if (resetMakeup) { avgGr = computeGain (st, -18.0f, sq0); resetMakeup = false; }
    const float avgK = std::exp (-1.0f / (1.5f * (float) sr));
    const float mkFactor = st.upward ? 0.6f : (style == 3 ? 0.75f : 0.9f);

    for (int i = 0; i < n; ++i)
    {
        const float sq = squeezeS.getNextValue();
        float dl = L[i], dr = R[i];
        if (st.scHpfHz > 0.0f) { dl = scHpfL.processSample (dl); dr = scHpfR.processSample (dr); }
        float level;
        if (st.rms)
        {
            const float p = 0.5f * (dl * dl + dr * dr);
            rmsState = rmsK * rmsState + (1.0f - rmsK) * p;
            level = std::sqrt (rmsState) * 1.414f;
        }
        else
            level = juce::jmax (std::abs (dl), std::abs (dr));
        const float levelDb = juce::Decibels::gainToDecibels (level, -100.0f);

        const float target = computeGain (st, levelDb, sq);
        float rel = relMs;
        if (st.programRelease) rel *= 1.0f + 8.0f * juce::jlimit (0.0f, 1.0f, -slowDb / 8.0f);   // opto : plus on compresse longtemps, plus ca relache lentement
        const float aRel = std::exp (-1.0f / (0.001f * rel * (float) sr));
        const float a = (target < envDb) ? aAtt : aRel;
        envDb = a * envDb + (1.0f - a) * target;
        slowDb = slowK * slowDb + (1.0f - slowK) * envDb;
        if (levelDb > -45.0f) avgGr = avgK * avgGr + (1.0f - avgK) * envDb;   // seulement quand il y a du son
        const float makeup = juce::jlimit (-12.0f, 24.0f, -avgGr * mkFactor);

        const float g = juce::Decibels::decibelsToGain (envDb + makeup);
        (void) makeup;
        L[i] *= g;
        if (nch > 1) R[i] *= g;
        minGr = juce::jmin (minGr, envDb);
        maxGr = juce::jmax (maxGr, envDb);
    }
    grDb = st.upward ? maxGr + juce::jmin (0.0f, minGr) : minGr;

    // --- couleur (sur-echantillonnee) ---
    {
        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nch, (size_t) n);
        auto up = os->processSamplesUp (block);
        applyColor (up, style, colorS.getNextValue());
        colorS.skip (n - 1);
        os->processSamplesDown (block);
    }

    // --- BLOOM : elargit la stereo ---
    if (st.upward && nch > 1)
    {
        const float w = 1.0f + 0.6f * colorS.getCurrentValue();
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]) * w;
            L[i] = m + s; R[i] = m - s;
        }
    }

    // --- compensation de volume : meme ressenti avant / apres (mesure lente, ~2 s) ---
    {
        double pin = 0, pout = 0;
        for (int ch = 0; ch < nch; ++ch)
            for (int i = 0; i < n; ++i) { const float a = dryBuf.getSample (ch, i), b = buffer.getSample (ch, i); pin += a * a; pout += b * b; }
        pin /= (n * nch); pout /= (n * nch);
        const float k = std::exp (-(float) n / (2.0f * (float) sr));
        if (pin > 1.0e-5)
        {
            powIn  = k * powIn  + (1.0f - k) * (float) pin;
            powOut = k * powOut + (1.0f - k) * (float) pout;
        }
        float target = powOut > 1.0e-9f ? std::sqrt (powIn / powOut) : 1.0f;
        if (style == 3) target *= 1.25f;                          // SMASH reste un peu plus fort : c'est son role
        target = juce::jlimit (0.125f, 8.0f, target);
        const float startG = matchGain;
        matchGain += 0.25f * (target - matchGain);
        for (int ch = 0; ch < nch; ++ch) buffer.applyGainRamp (ch, 0, n, startG, matchGain);
    }

    // --- MIX (compression parallele) + securite ---
    float outPeak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float m = mixS.getNextValue();
        for (int ch = 0; ch < nch; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            float y = dryBuf.getSample (ch, i) * (1.0f - m) + d[i] * m;
            if (! std::isfinite (y)) y = 0.0f;
            const float a = std::abs (y);
            if (a > 0.9f) y = std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), y);   // anti-saturation numerique
            d[i] = y;
            outPeak = juce::jmax (outPeak, a);
        }
    }

    inDb  = juce::jmax (juce::Decibels::gainToDecibels (inPeak, -100.0f), inDb.load() - 1.5f);
    outDb = juce::jmax (juce::Decibels::gainToDecibels (juce::jmin (outPeak, 1.0f), -100.0f), outDb.load() - 1.5f);
    for (int ch = nch; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
}

//==============================================================================
void OneCompressProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void OneCompressProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* OneCompressProcessor::createEditor() { return new OneCompressEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OneCompressProcessor(); }
