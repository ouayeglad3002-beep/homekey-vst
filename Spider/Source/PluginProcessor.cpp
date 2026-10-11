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
    engine.setPreset ((int) apvts.getRawParameterValue ("preset")->load());
    engine.prepare (sampleRate, juce::jmax (samplesPerBlock, 4096));
    lastPreset = (int) apvts.getRawParameterValue ("preset")->load();
    setLatencySamples (engine.latency());
}

void SpiderProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0) return;
    const int p = (int) apvts.getRawParameterValue ("preset")->load();
    if (p != lastPreset) { engine.setPreset (p); lastPreset = p; }

    double bpm = 120.0, ppq = internalPpq; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (playing) if (auto pq = pos->getPpqPosition()) ppq = *pq;
        }
    const double spb = getSampleRate() * 60.0 / juce::jlimit (30.0, 300.0, bpm);
    engine.process (buffer, sampleCounter, ppq, spb, playing);
    sampleCounter += n;
    internalPpq = ppq + n / spb;
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
}

void SpiderProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void SpiderProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* SpiderProcessor::createEditor() { return new SpiderEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpiderProcessor(); }
