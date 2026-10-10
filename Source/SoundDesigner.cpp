#include "SoundDesigner.h"
#include "PluginProcessor.h"

namespace
{
    using R = std::pair<float, float>;
    struct Recipe
    {
        std::map<juce::String, R> ranges;
        std::vector<int> octaves, textures, delays;
        std::vector<int> targets;      // cibles favorites des etoiles
        std::vector<int> shapes;       // formes favorites
        R rate, chaos;
    };

    using namespace Constellation;

    const Recipe& recipe (int world)
    {
        static const std::vector<Recipe> recipes =
        {
            // 0 DREAMING : reve, shimmer, nappes, douceur infinie
            { { { "tone", { -0.4f, 0.3f } }, { "release", { 1.0f, 3.5f } }, { "layer", { 0.3f, 0.8f } }, { "chorus", { 0.3f, 0.8f } },
                { "reverb", { 0.45f, 0.8f } }, { "size", { 0.8f, 1.0f } }, { "width", { 0.8f, 1.0f } }, { "drive", { 0.0f, 0.2f } },
                { "wow", { 0.0f, 0.25f } }, { "crush", { 0.0f, 0.08f } }, { "vinyl", { 0.0f, 0.1f } }, { "piano", { 0.6f, 1.0f } },
                { "harm", { 0.1f, 0.5f } }, { "metal", { 0.0f, 0.3f } }, { "ring", { 0.0f, 0.08f } }, { "texlvl", { 0.2f, 0.6f } },
                { "fcut", { 4000.0f, 20000.0f } }, { "fres", { 0.0f, 0.3f } }, { "attack", { 0.002f, 0.4f } }, { "hammer", { 0.2f, 1.0f } },
                { "sub", { 0.0f, 0.2f } }, { "unison", { 1.0f, 3.0f } }, { "dmix", { 0.0f, 0.35f } }, { "dfb", { 0.3f, 0.6f } },
                { "shimmer", { 0.3f, 0.8f } }, { "air", { 0.0f, 0.4f } }, { "hum", { 0.0f, 0.0f } }, { "decay", { 0.8f, 1.5f } } },
              { 0, 0, 1 }, { 0, 3, 3, 7, 2 }, { 2, 3, 4 },
              { Shimmer, Constellation::Reverb, Pad, Tone, Chorus, Space, Pitch, Air }, { 0, 0, 4, 1 }, { 0.05f, 0.4f }, { 0.2f, 0.6f } },

            // 1 NEBULA : espace alien, metal, anneau, textures etranges
            { { { "tone", { -0.2f, 0.5f } }, { "release", { 0.8f, 3.0f } }, { "layer", { 0.2f, 0.6f } }, { "chorus", { 0.2f, 0.7f } },
                { "reverb", { 0.4f, 0.8f } }, { "size", { 0.75f, 1.0f } }, { "width", { 0.85f, 1.0f } }, { "drive", { 0.0f, 0.4f } },
                { "wow", { 0.0f, 0.4f } }, { "crush", { 0.0f, 0.4f } }, { "vinyl", { 0.0f, 0.0f } }, { "piano", { 0.3f, 0.9f } },
                { "harm", { 0.3f, 0.8f } }, { "metal", { 0.3f, 0.9f } }, { "ring", { 0.1f, 0.6f } }, { "texlvl", { 0.3f, 0.7f } },
                { "fcut", { 1500.0f, 20000.0f } }, { "fres", { 0.1f, 0.6f } }, { "attack", { 0.002f, 0.3f } }, { "hammer", { 0.5f, 2.0f } },
                { "sub", { 0.0f, 0.3f } }, { "unison", { 1.0f, 4.0f } }, { "dmix", { 0.1f, 0.5f } }, { "dfb", { 0.3f, 0.7f } },
                { "shimmer", { 0.1f, 0.6f } }, { "air", { 0.2f, 0.7f } }, { "hum", { 0.0f, 0.1f } }, { "decay", { 0.6f, 1.6f } } },
              { 0, 1, -1 }, { 2, 3, 6, 7, 0 }, { 0, 1, 2, 3, 4, 5 },
              { Ring, Metal, Filter, Pitch, Shimmer, Air, Delay, Texture, Wow }, { 0, 1, 2, 3, 4 }, { 0.2f, 1.5f }, { 0.4f, 0.9f } },

            // 2 CINEMA : lourd, epique, grandes salles, cordes
            { { { "tone", { -0.3f, 0.2f } }, { "release", { 1.2f, 3.5f } }, { "layer", { 0.5f, 1.0f } }, { "chorus", { 0.05f, 0.3f } },
                { "reverb", { 0.5f, 0.8f } }, { "size", { 0.85f, 1.0f } }, { "width", { 0.85f, 1.0f } }, { "drive", { 0.0f, 0.2f } },
                { "wow", { 0.0f, 0.1f } }, { "crush", { 0.0f, 0.05f } }, { "vinyl", { 0.0f, 0.0f } }, { "piano", { 0.8f, 1.0f } },
                { "harm", { 0.1f, 0.4f } }, { "metal", { 0.0f, 0.25f } }, { "ring", { 0.0f, 0.05f } }, { "texlvl", { 0.3f, 0.6f } },
                { "fcut", { 6000.0f, 20000.0f } }, { "fres", { 0.0f, 0.2f } }, { "attack", { 0.002f, 0.8f } }, { "hammer", { 0.5f, 1.2f } },
                { "sub", { 0.2f, 0.7f } }, { "unison", { 1.0f, 2.0f } }, { "dmix", { 0.0f, 0.2f } }, { "dfb", { 0.3f, 0.5f } },
                { "shimmer", { 0.0f, 0.35f } }, { "air", { 0.0f, 0.3f } }, { "hum", { 0.0f, 0.0f } }, { "decay", { 0.9f, 1.6f } } },
              { 0, -1, 0 }, { 1, 4, 0, 0 }, { 3, 4, 5 },
              { Pad, Constellation::Reverb, Filter, Volume, Tone, Texture, Space, Shimmer }, { 0, 1, 4 }, { 0.05f, 0.3f }, { 0.1f, 0.4f } },

            // 3 ABYSS : dark fantasy, basses lourdes et metalliques
            { { { "tone", { -0.8f, -0.1f } }, { "release", { 0.8f, 3.0f } }, { "layer", { 0.3f, 0.9f } }, { "chorus", { 0.0f, 0.3f } },
                { "reverb", { 0.35f, 0.7f } }, { "size", { 0.7f, 1.0f } }, { "width", { 0.6f, 1.0f } }, { "drive", { 0.25f, 0.75f } },
                { "wow", { 0.0f, 0.3f } }, { "crush", { 0.0f, 0.35f } }, { "vinyl", { 0.0f, 0.15f } }, { "piano", { 0.5f, 1.0f } },
                { "harm", { 0.3f, 0.8f } }, { "metal", { 0.4f, 1.0f } }, { "ring", { 0.1f, 0.6f } }, { "texlvl", { 0.3f, 0.8f } },
                { "fcut", { 600.0f, 8000.0f } }, { "fres", { 0.1f, 0.6f } }, { "attack", { 0.002f, 0.1f } }, { "hammer", { 0.6f, 2.0f } },
                { "sub", { 0.4f, 1.0f } }, { "unison", { 1.0f, 3.0f } }, { "dmix", { 0.0f, 0.3f } }, { "dfb", { 0.3f, 0.6f } },
                { "shimmer", { 0.0f, 0.25f } }, { "air", { 0.0f, 0.3f } }, { "hum", { 0.0f, 0.15f } }, { "decay", { 0.7f, 1.8f } } },
              { -1, -1, -2, 0 }, { 1, 5, 6, 4 }, { 1, 3, 5 },
              { Drive, Filter, Ring, Metal, Reso, Crush, Volume, Texture }, { 1, 2, 3, 4 }, { 0.1f, 0.8f }, { 0.3f, 0.8f } },

            // 4 BACKROOMS : liminal, neon qui gresille, bande usee, piece etouffee
            { { { "tone", { -0.7f, 0.0f } }, { "release", { 0.4f, 2.0f } }, { "layer", { 0.1f, 0.5f } }, { "chorus", { 0.1f, 0.5f } },
                { "reverb", { 0.25f, 0.55f } }, { "size", { 0.2f, 0.55f } }, { "width", { 0.4f, 0.8f } }, { "drive", { 0.1f, 0.45f } },
                { "wow", { 0.3f, 0.8f } }, { "crush", { 0.1f, 0.5f } }, { "vinyl", { 0.2f, 0.6f } }, { "piano", { 0.6f, 1.0f } },
                { "harm", { 0.0f, 0.3f } }, { "metal", { 0.0f, 0.2f } }, { "ring", { 0.0f, 0.15f } }, { "texlvl", { 0.1f, 0.4f } },
                { "fcut", { 1200.0f, 4500.0f } }, { "fres", { 0.0f, 0.4f } }, { "attack", { 0.002f, 0.2f } }, { "hammer", { 0.3f, 1.0f } },
                { "sub", { 0.0f, 0.2f } }, { "unison", { 1.5f, 4.0f } }, { "dmix", { 0.0f, 0.25f } }, { "dfb", { 0.2f, 0.5f } },
                { "shimmer", { 0.0f, 0.1f } }, { "air", { 0.0f, 0.2f } }, { "hum", { 0.3f, 0.8f } }, { "decay", { 0.5f, 1.2f } },
                { "fine", { -30.0f, 30.0f } } },
              { 0, 0, -1 }, { 0, 2, 6 }, { 0, 1, 2 },
              { Wow, Crush, Filter, Volume, Pitch, Tone, Constellation::Reverb, Air }, { 1, 2, 2, 3 }, { 0.3f, 2.0f }, { 0.5f, 1.0f } },
        };
        return recipes[(size_t) juce::jlimit (0, 4, world)];
    }

    void setReal (juce::AudioProcessorValueTreeState& s, const juce::String& id, float v)
    {
        if (auto* p = s.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    }
    float getReal (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        return s.getRawParameterValue (id)->load();
    }
    template <typename T> T pick (juce::Random& r, const std::vector<T>& v) { return v[(size_t) r.nextInt ((int) v.size())]; }
    float in (juce::Random& r, R range) { return range.first + r.nextFloat() * (range.second - range.first); }
}

juce::String SoundDesigner::makeName (int world)
{
    auto& r = juce::Random::getSystemRandom();
    static const juce::StringArray words[5] = {
        { "Reve", "Lucid", "Halo", "Opaline", "Somnia", "Aurora", "Velours", "Nuage" },
        { "Xeno", "Nebula", "Quasar", "Zeta", "Orbit", "Signal", "Pulsar", "Void" },
        { "Titan", "Eclipse", "Exodus", "Monolith", "Requiem", "Odyssey", "Empire", "Genesis" },
        { "Abyss", "Obsidian", "Wraith", "Iron", "Necro", "Throne", "Dragon", "Rune" },
        { "Level", "Hallway", "Neon", "Liminal", "Room", "Exit", "Fluo", "Carpet" } };
    static const juce::StringArray a { "Vel", "Xa", "Nor", "Ael", "Kae", "Thal", "Ory", "Zy", "Mor", "Sel", "Ix", "Lu", "Dra", "Vey" };
    static const juce::StringArray b { "ith", "ora", "ys", "en", "ar", "eon", "is", "ax", "uun", "iel", "os", "ae" };
    const int w = juce::jlimit (0, 4, world);
    const auto word = words[w][r.nextInt (words[w].size())];
    if (w == 4) return word + " " + juce::String (r.nextInt (900) + 7);
    return word + " " + a[r.nextInt (a.size())] + b[r.nextInt (b.size())];
}

void SoundDesigner::randomizeStars (juce::AudioProcessorValueTreeState& s, float mut)
{
    auto& r = juce::Random::getSystemRandom();
    const int world = (int) getReal (s, "type");
    const auto& rc = recipe (world);
    auto mix = [&] (const juce::String& id, float target) { const float o = getReal (s, id); setReal (s, id, o + (target - o) * mut); };

    mix ("cdepth", 0.35f + r.nextFloat() * 0.45f);
    mix ("crate", in (r, rc.rate));
    mix ("cchaos", in (r, rc.chaos));
    mix ("corbit", (r.nextFloat() * 2.0f - 1.0f) * 0.5f);

    // forme harmonieuse : angles repartis, rayons varies
    const float start = r.nextFloat() * juce::MathConstants<float>::twoPi;
    std::vector<int> used;
    for (int i = 0; i < HomeKeysProcessor::numStars; ++i)
    {
        const auto id = "s" + juce::String (i);
        const float ang = start + juce::MathConstants<float>::twoPi * ((float) i + 0.7f * r.nextFloat()) / (float) HomeKeysProcessor::numStars;
        const float rad = 0.08f + 0.38f * r.nextFloat();
        mix (id + "x", juce::jlimit (0.02f, 0.98f, 0.5f + rad * std::cos (ang)));
        mix (id + "y", juce::jlimit (0.02f, 0.98f, 0.5f + rad * std::sin (ang)));

        if (r.nextFloat() < juce::jmax (0.15f, mut))
        {
            int t = -1;
            for (int tries = 0; tries < 12; ++tries)
            {
                const int c = r.nextFloat() < 0.8f ? pick (r, rc.targets) : r.nextInt (NumTargets);
                if (std::find (used.begin(), used.end(), c) == used.end() || tries == 11) { t = c; break; }
            }
            used.push_back (t);
            setReal (s, id + "t", (float) t);
            setReal (s, id + "w", (float) pick (r, rc.shapes));
        }
    }
}

juce::String SoundDesigner::randomizeSound (juce::AudioProcessorValueTreeState& s, float mut, bool keepWorld)
{
    auto& r = juce::Random::getSystemRandom();
    int world = (int) getReal (s, "type");
    if (! keepWorld && r.nextFloat() < 0.15f + mut * 0.7f)
        world = r.nextInt (5);
    setReal (s, "type", (float) world);
    const auto& rc = recipe (world);

    for (auto& [id, range] : rc.ranges)
    {
        const float o = getReal (s, id);
        float t = in (r, range);
        if (id == "fcut") t = range.first * std::pow (range.second / range.first, r.nextFloat());   // echelle log
        setReal (s, id, o + (t - o) * mut);
    }
    // valeurs a choix : changent selon la mutation
    if (r.nextFloat() < mut) setReal (s, "octave",  (float) pick (r, rc.octaves));
    if (r.nextFloat() < mut) setReal (s, "tex",     (float) pick (r, rc.textures));
    if (r.nextFloat() < mut) setReal (s, "dtime",   (float) pick (r, rc.delays));
    if (r.nextFloat() < mut) setReal (s, "ftype",   (float) (world == 1 && r.nextFloat() < 0.3f ? 1 : 0));
    setReal (s, "velocity", 0.65f + r.nextFloat() * 0.25f);
    setReal (s, "cvar", r.nextFloat() * 0.45f * mut + getReal (s, "cvar") * (1.0f - mut));
    setReal (s, "chop", 0.0f);

    randomizeStars (s, mut);

    // volume coherent : les sons lourds et satures sont compenses
    const float hot = getReal (s, "drive") * 3.0f + getReal (s, "sub") * 2.0f + getReal (s, "ring") * 1.5f;
    setReal (s, "volume", -hot);
    return makeName (world);
}
