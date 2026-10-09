#include "PresetManager.h"

//                                             type  vol   tone  vel   rel   layer chorus reverb size width
const std::vector<PresetManager::Factory>& PresetManager::factory()
{
    static const std::vector<Factory> f =
    {
        // ---- DOUX ----
        { "Doux - Felt Lullaby",      0,  0.0f, -0.20f, 0.70f, 0.45f, 0.0f, 0.10f, 0.28f, 0.45f, 0.65f },
        { "Doux - Bedroom Keys",      0,  0.0f,  0.05f, 0.80f, 0.35f, 0.0f, 0.00f, 0.18f, 0.30f, 0.60f },
        { "Doux - Whisper Felt",      0,  1.0f, -0.45f, 0.55f, 0.70f, 0.0f, 0.20f, 0.40f, 0.60f, 0.75f },
        // ---- GRAVE ----
        { "Grave - Abyss Grand",      1,  0.0f, -0.10f, 0.80f, 0.60f, 0.40f, 0.05f, 0.30f, 0.65f, 0.70f },
        { "Grave - Low Cathedral",    1, -1.0f,  0.00f, 0.75f, 1.20f, 0.60f, 0.10f, 0.50f, 0.90f, 0.85f },
        { "Grave - Dark Matter",      1,  0.0f, -0.40f, 0.85f, 0.80f, 0.80f, 0.15f, 0.35f, 0.75f, 0.70f },
        // ---- ORCHESTRE ----
        { "Orchestre - Concert Hall", 2, -1.0f,  0.15f, 0.85f, 0.55f, 0.30f, 0.05f, 0.35f, 0.80f, 0.80f },
        { "Orchestre - Strings & Grand", 2, -2.0f, 0.05f, 0.80f, 0.90f, 0.80f, 0.15f, 0.40f, 0.85f, 0.90f },
        { "Orchestre - Bright Stage", 2, -1.0f,  0.45f, 0.90f, 0.40f, 0.10f, 0.00f, 0.25f, 0.55f, 0.70f },
        // ---- CINEMATIQUE ----
        { "Cinematique - Epic Trailer", 3, -1.5f, 0.10f, 0.85f, 1.40f, 0.70f, 0.20f, 0.55f, 0.90f, 0.90f },
        { "Cinematique - Nebula Score", 3, -1.0f, -0.20f, 0.70f, 2.00f, 0.55f, 0.35f, 0.65f, 0.95f, 1.00f },
        { "Cinematique - Last Scene",   3,  0.0f, -0.05f, 0.80f, 1.00f, 0.35f, 0.15f, 0.45f, 0.80f, 0.80f },
        // ---- DREAMING ----
        { "Dreaming - Dream Signal",    4, -1.0f, -0.10f, 0.70f, 2.00f, 0.60f, 0.55f, 0.65f, 0.95f, 1.00f },
        { "Dreaming - Astral Lullaby",  4,  0.0f, -0.35f, 0.60f, 2.80f, 0.75f, 0.70f, 0.75f, 0.98f, 1.00f },
        { "Dreaming - Lucid Drift",     4, -0.5f,  0.10f, 0.75f, 1.60f, 0.45f, 0.45f, 0.55f, 0.85f, 0.90f },
    };
    return f;
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s)
{
    refresh();
}

juce::File PresetManager::getUserFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("HomeKeys").getChildFile ("Presets");
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

void PresetManager::refresh()
{
    userFiles = getUserFolder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension);
    std::sort (userFiles.begin(), userFiles.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareIgnoreCase (b.getFileNameWithoutExtension()) < 0; });
}

int PresetManager::getNumFactory() const { return (int) factory().size(); }
int PresetManager::getNumPresets() const { return getNumFactory() + userFiles.size(); }

juce::String PresetManager::getPresetName (int index) const
{
    if (index < 0 || index >= getNumPresets()) return {};
    if (isFactory (index)) return factory()[(size_t) index].name;
    return "* " + userFiles[index - getNumFactory()].getFileNameWithoutExtension();
}

void PresetManager::setParam (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= getNumPresets()) return;

    if (isFactory (index))
    {
        const auto& f = factory()[(size_t) index];
        setParam ("type",     (float) f.type);
        setParam ("volume",   f.volume);
        setParam ("tone",     f.tone);
        setParam ("velocity", f.velocity);
        setParam ("release",  f.release);
        setParam ("layer",    f.layer);
        setParam ("chorus",   f.chorus);
        setParam ("reverb",   f.reverb);
        setParam ("size",     f.size);
        setParam ("width",    f.width);
    }
    else
    {
        const auto file = userFiles[index - getNumFactory()];
        if (auto xml = juce::parseXML (file))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            // applique chaque parametre (compatible avec les futures versions)
            for (auto* param : apvts.processor.getParameters())
                if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                {
                    auto child = tree.getChildWithProperty ("id", ranged->getParameterID());
                    if (child.isValid())
                        ranged->setValueNotifyingHost (ranged->convertTo0to1 ((float) child.getProperty ("value")));
                }
        }
    }

    currentIndex = index;
    currentName = getPresetName (index);
}

bool PresetManager::savePreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;

    auto file = getUserFolder().getChildFile (name + extension);
    auto state = apvts.copyState();
    state.setProperty ("presetName", name, nullptr);

    if (auto xml = state.createXml())
    {
        if (! xml->writeTo (file)) return false;
        refresh();
        for (int i = getNumFactory(); i < getNumPresets(); ++i)
            if (userFiles[i - getNumFactory()] == file)
            {
                currentIndex = i;
                currentName = getPresetName (i);
            }
        return true;
    }
    return false;
}

bool PresetManager::deletePreset (int index)
{
    if (isFactory (index) || index >= getNumPresets()) return false;
    const bool ok = userFiles[index - getNumFactory()].deleteFile();
    refresh();
    loadPreset (0);
    return ok;
}

void PresetManager::next()     { loadPreset ((currentIndex + 1) % getNumPresets()); }
void PresetManager::previous() { loadPreset ((currentIndex - 1 + getNumPresets()) % getNumPresets()); }

void PresetManager::setCurrentName (const juce::String& n)
{
    currentName = n;
    for (int i = 0; i < getNumPresets(); ++i)
        if (getPresetName (i) == n) { currentIndex = i; return; }
}
