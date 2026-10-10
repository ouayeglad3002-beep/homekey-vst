#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>

// Banque d'echantillons partagee par toutes les instances de HomeKey I.
//  - Piano : Salamander Grand Piano V3 (Alexander Holm, CC-BY 3.0)
//  - Textures metal : Versilian Community Sample Library (CC0)
// Chargee une seule fois en memoire (int16), en arriere-plan.
class SampleBank
{
public:
    struct Zone
    {
        int root = 60;
        std::vector<juce::int16> data;   // stereo entrelace
        int length = 0;                  // en trames
        double sampleRate = 44100.0;

        inline void read (double pos, float& l, float& r) const noexcept
        {
            // interpolation Hermite 4 points (propre, sans aliasing audible)
            const int i = (int) pos;
            if (i < 1 || i >= length - 2) { l = r = 0.0f; return; }
            const float f = (float) (pos - i);
            const juce::int16* p = data.data() + (i - 1) * 2;
            constexpr float s = 1.0f / 32768.0f;
            for (int c = 0; c < 2; ++c)
            {
                const float y0 = p[c] * s, y1 = p[2 + c] * s, y2 = p[4 + c] * s, y3 = p[6 + c] * s;
                const float c1 = 0.5f * (y2 - y0);
                const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
                const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
                const float v = ((c3 * f + c2) * f + c1) * f + y1;
                if (c == 0) l = v; else r = v;
            }
        }
    };

    static constexpr int numLayers = 4;
    static constexpr int numTextures = 7;

    SampleBank();
    ~SampleBank();

    bool isReady() const noexcept { return ready.load(); }

    // zone de piano la plus proche (note MIDI, couche 0..3)
    const Zone* pianoZone (int midiNote, int layer) const noexcept;
    const Zone* texture (int index) const noexcept;

    static juce::StringArray textureNames()
    {
        return { "AUCUNE", "GONG", "CYMBALE ARCHET", "VIBES ARCHET", "CLOCHE TUBE", "ENCLUME", "FREIN ARCHET", "CLOCHE NEPAL" };
    }

private:
    void load();
    std::vector<Zone> piano;            // triees par note puis couche
    std::array<int, 128 * numLayers> pianoMap {};
    std::vector<Zone> textures;
    std::atomic<bool> ready { false };
    std::thread loader;
};
