#include "SampleBank.h"
#include "BinaryData.h"

SampleBank::SampleBank()
{
    pianoMap.fill (-1);
    loader = std::thread ([this] { load(); });
}

SampleBank::~SampleBank()
{
    if (loader.joinable())
        loader.join();
}

void SampleBank::load()
{
    juce::ZipFile zip (new juce::MemoryInputStream (BinaryData::Samples_zip, BinaryData::Samples_zipSize, false), true);
    juce::OggVorbisAudioFormat ogg;

    std::vector<Zone> pz, tz (numTextures);
    struct Key { int note, layer; };
    std::vector<Key> keys;

    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        const auto* entry = zip.getEntry (i);
        const auto name = entry->filename;
        std::unique_ptr<juce::InputStream> in (zip.createStreamForEntry (i));
        if (in == nullptr) continue;
        std::unique_ptr<juce::AudioFormatReader> reader (ogg.createReaderFor (in.release(), true));
        if (reader == nullptr) continue;

        Zone z;
        z.sampleRate = reader->sampleRate;
        z.length = (int) reader->lengthInSamples;
        juce::AudioBuffer<float> tmp (2, z.length);
        reader->read (&tmp, 0, z.length, 0, true, true);
        z.data.resize ((size_t) z.length * 2);
        for (int s = 0; s < z.length; ++s)
            for (int c = 0; c < 2; ++c)
                z.data[(size_t) s * 2 + (size_t) c] = (juce::int16) juce::jlimit (-32767, 32767,
                    juce::roundToInt (tmp.getSample (juce::jmin (c, tmp.getNumChannels() - 1), s) * 32767.0f));

        const auto parts = juce::StringArray::fromTokens (name.upToFirstOccurrenceOf (".", false, false), "_", "");
        if (name.startsWith ("p_") && parts.size() == 3)
        {
            z.root = parts[1].getIntValue();
            keys.push_back ({ z.root, parts[2].getIntValue() });
            pz.push_back (std::move (z));
        }
        else if (name.startsWith ("t_") && parts.size() == 3)
        {
            const int idx = parts[1].getIntValue();
            z.root = parts[2].getIntValue();
            if (idx >= 0 && idx < numTextures)
                tz[(size_t) idx] = std::move (z);
        }
    }

    // pour chaque note et couche : zone la plus proche
    for (int layer = 0; layer < numLayers; ++layer)
        for (int note = 0; note < 128; ++note)
        {
            int best = -1, bestDist = 1000;
            for (size_t k = 0; k < keys.size(); ++k)
                if (keys[k].layer == layer)
                {
                    const int d = std::abs (keys[k].note - note);
                    if (d < bestDist) { bestDist = d; best = (int) k; }
                }
            pianoMap[(size_t) (note * numLayers + layer)] = best;
        }

    piano = std::move (pz);
    textures = std::move (tz);
    ready = true;
}

const SampleBank::Zone* SampleBank::pianoZone (int note, int layer) const noexcept
{
    if (! ready) return nullptr;
    const int idx = pianoMap[(size_t) (juce::jlimit (0, 127, note) * numLayers + juce::jlimit (0, numLayers - 1, layer))];
    return idx >= 0 ? &piano[(size_t) idx] : nullptr;
}

const SampleBank::Zone* SampleBank::texture (int index) const noexcept
{
    if (! ready || index < 0 || index >= (int) textures.size() || textures[(size_t) index].length == 0) return nullptr;
    return &textures[(size_t) index];
}
