#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    juce::StringArray names;
    for (int i = 0; i < Web::numPresets; ++i) names.add (Web::preset (i).name);
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "preset", 1 }, "Preset", names, 0));
    return { p.begin(), p.end() };
}

SpiderProcessor::SpiderProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Spider", layout())
{
}

bool SpiderProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono());
}

void SpiderProcessor::selectPreset (int index)
{
    if (auto* p = apvts.getParameter ("preset"))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, Web::numPresets - 1, index)));
}

void SpiderProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // la toile apprise n'est PAS effacee ici : seuls les tampons audio repartent a zero
    engine.setPreset ((int) apvts.getRawParameterValue ("preset")->load());
    engine.prepare (sampleRate, juce::jlimit (512, 8192, samplesPerBlock));
    lastPreset = (int) apvts.getRawParameterValue ("preset")->load();
    setLatencySamples (engine.latency());
    bypassDelay.setSize (2, juce::jmax (1, engine.latency()));
    bypassDelay.clear(); bypassPos = 0;
    wasPlaying = false;
}

void SpiderProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0) return;
    const int p = (int) apvts.getRawParameterValue ("preset")->load();
    const bool snap = snapPreset.exchange (false);
    if (p != lastPreset) { if (snap) engine.setPresetImmediate (p); else engine.setPreset (p); lastPreset = p; }

    double bpm = 120.0, ppq = internalPpq; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (playing) if (auto pq = pos->getPpqPosition()) ppq = *pq;
        }
    const double spb = getSampleRate() * 60.0 / juce::jlimit (30.0, 300.0, bpm);

    // saut, boucle, demarrage, ou retour de bypass : on vide ce qui etait en vol, la toile reste
    if ((playing && (! wasPlaying || std::abs (ppq - expectedPpq) > 0.02)) || wasBypassed)
        engine.onTransportJump();
    wasPlaying = playing; wasBypassed = false;
    expectedPpq = ppq + n / spb;

    // blocs de taille quelconque : decoupes en morceaux prepares a l'avance (aucune allocation)
    const int maxB = engine.maxBlock();
    for (int off = 0; off < n; off += maxB)
    {
        const int len = juce::jmin (maxB, n - off);
        juce::AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), juce::jmin (2, buffer.getNumChannels()), off, len);
        engine.process (sub, sampleCounter, ppq + off / spb, spb, playing);
        sampleCounter += len;
    }
    internalPpq = ppq + n / spb;
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
}

void SpiderProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int n = buffer.getNumSamples(), D = bypassDelay.getNumSamples();
    if (getLatencySamples() <= 0) return;
    const int nch = juce::jmin (2, buffer.getNumChannels());
    for (int i = 0; i < n; ++i)
    {
        for (int ch = 0; ch < nch; ++ch)
        {
            const float x = buffer.getSample (ch, i);
            buffer.setSample (ch, i, bypassDelay.getSample (ch, bypassPos));
            bypassDelay.setSample (ch, bypassPos, x);
        }
        bypassPos = (bypassPos + 1) % D;
    }
    wasBypassed = true;
}

void SpiderProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    // la toile apprise voyage avec le projet
    auto l = engine.learned();
    l.gain = engine.currentGain.load();
    juce::ValueTree web ("WEB");
    web.setProperty ("valid", l.valid, nullptr);
    web.setProperty ("tonic", l.tonic, nullptr);
    web.setProperty ("minor", l.minor, nullptr);
    web.setProperty ("register", l.reg, nullptr);
    web.setProperty ("density", l.dens, nullptr);
    web.setProperty ("gain", l.gain, nullptr);
    state.removeChild (state.getChildWithName ("WEB"), nullptr);
    state.appendChild (web, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void SpiderProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            Engine::Learned l;
            auto web = tree.getChildWithName ("WEB");
            if (web.isValid())
            {
                l.valid = (bool) web.getProperty ("valid", false);
                l.tonic = (int) web.getProperty ("tonic", 0);
                l.minor = (bool) web.getProperty ("minor", true);
                l.reg = (float) (double) web.getProperty ("register", 64.0);
                l.dens = (float) (double) web.getProperty ("density", 3.0);
                l.gain = (float) (double) web.getProperty ("gain", 1.0);
            }
            engine.restoreLearned (l);
            snapPreset = true;      // le preset du projet s'applique sans fondu
            tree.removeChild (web, nullptr);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* SpiderProcessor::createEditor() { return new SpiderEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpiderProcessor(); }
