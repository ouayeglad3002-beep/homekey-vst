#include "PresetManager.h"
#include "BinaryData.h"

// Presets d'usine : un point de depart propre par monde
const std::vector<PresetManager::Factory>& PresetManager::factory()
{
    static const std::vector<Factory> f =
    {
        //  nom                   type vol   tone  vel   rel   layer chorus reverb size width
        { "INIT - Dreaming",      0,  0.0f,  0.0f, 0.75f, 1.2f, 0.4f, 0.35f, 0.50f, 0.85f, 0.90f },
        { "INIT - Nebula",        1,  0.0f,  0.1f, 0.75f, 1.0f, 0.3f, 0.30f, 0.45f, 0.85f, 0.90f },
        { "INIT - Cinema",        2,  0.0f,  0.0f, 0.80f, 1.2f, 0.4f, 0.15f, 0.45f, 0.90f, 0.85f },
        { "INIT - Abyss",         3,  0.0f, -0.2f, 0.85f, 1.0f, 0.3f, 0.10f, 0.40f, 0.85f, 0.75f },
        { "INIT - Backrooms",     4,  0.0f, -0.2f, 0.70f, 0.8f, 0.2f, 0.25f, 0.35f, 0.35f, 0.60f },
    };
    return f;
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s)
{
    installFactoryPack();
    refresh();
}

void PresetManager::installFactoryPack()
{
    const auto root = getUserFolder();
    const auto marker = root.getChildFile (".homekeyi_pack_v1");
    if (marker.existsAsFile())
        return;

    // reprend les presets perso de l'ancienne version HomeKeys (une seule fois)
    const auto old = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("HomeKeys").getChildFile ("Presets");
    if (old.isDirectory())
        for (auto& f : old.findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension))
            if (! root.getChildFile (f.getFileName()).exists())
                f.copyFileTo (root.getChildFile (f.getFileName()));

    // remplace l'ancienne version du pack (les presets perso ne sont pas touches)
    root.getChildFile ("HomeKey Preset").deleteRecursively();
    juce::ZipFile zip (new juce::MemoryInputStream (BinaryData::HomeKeyPresets_zip, BinaryData::HomeKeyPresets_zipSize, false), true);
    if (zip.uncompressTo (root, true).wasOk())
        marker.replaceWithText ("HomeKey I pack v1");
}

int PresetManager::importFrom (const juce::File& src)
{
    const auto root = getUserFolder();
    const auto pattern = juce::String ("*") + extension;

    if (src.isDirectory())
    {
        auto dest = root.getChildFile (src.getFileName());
        if (dest == src) return 0;
        int n = 0;
        for (auto& f : src.findChildFiles (juce::File::findFiles, true, pattern))
        {
            auto target = dest.getChildFile (f.getRelativePathFrom (src));
            target.getParentDirectory().createDirectory();
            if (f.copyFileTo (target)) ++n;
        }
        return n;
    }
    if (src.hasFileExtension ("zip"))
    {
        juce::ZipFile zip (src);
        int n = 0;
        for (int i = 0; i < zip.getNumEntries(); ++i)
            if (auto* e = zip.getEntry (i))
                if (e->filename.endsWithIgnoreCase (extension)) ++n;
        auto dest = root.getChildFile (src.getFileNameWithoutExtension());
        dest.createDirectory();
        return zip.uncompressTo (dest, true).wasOk() ? n : 0;
    }
    if (src.hasFileExtension (juce::String (extension).substring (1)))
        return src.copyFileTo (root.getChildFile (src.getFileName())) ? 1 : 0;
    return 0;
}

juce::File PresetManager::getUserFolder()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("HomeKey I").getChildFile ("Presets");
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

void PresetManager::refresh()
{
    // lit aussi les sous-dossiers (packs de presets ranges par theme)
    const auto root = getUserFolder();
    userFiles = root.findChildFiles (juce::File::findFiles, true, juce::String ("*") + extension);
    std::sort (userFiles.begin(), userFiles.end(), [root] (const juce::File& a, const juce::File& b)
    {
        // presets perso (racine) en premier, puis chaque dossier dans l'ordre alphabetique
        const auto ca = a.getParentDirectory() == root ? juce::String() : a.getParentDirectory().getRelativePathFrom (root);
        const auto cb = b.getParentDirectory() == root ? juce::String() : b.getParentDirectory().getRelativePathFrom (root);
        if (ca != cb) return ca.compareIgnoreCase (cb) < 0;
        return a.getFileNameWithoutExtension().compareIgnoreCase (b.getFileNameWithoutExtension()) < 0;
    });
}

int PresetManager::getNumFactory() const { return (int) factory().size(); }
int PresetManager::getNumPresets() const { return getNumFactory() + userFiles.size(); }

juce::String PresetManager::getPresetName (int index) const
{
    if (index < 0 || index >= getNumPresets()) return {};
    if (isFactory (index)) return factory()[(size_t) index].name;
    return userFiles[index - getNumFactory()].getFileNameWithoutExtension();
}

juce::String PresetManager::getCategory (int index) const
{
    if (isFactory (index) || index >= getNumPresets()) return {};
    const auto parent = userFiles[index - getNumFactory()].getParentDirectory();
    if (parent == getUserFolder()) return {};
    // "HomeKey Preset/03 - Trap" -> "TRAP"
    auto name = parent.getFileName();
    if (name.containsChar ('-') && name.substring (0, 2).containsOnly ("0123456789"))
        name = name.fromFirstOccurrenceOf ("-", false, false).trim();
    return name.toUpperCase();
}

void PresetManager::setParam (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void PresetManager::loadPreset (int index)
{
    if (index < 0 || index >= getNumPresets()) return;

    // on repart des valeurs par defaut : aucun grain d'un ancien preset ne reste
    for (auto* param : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

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
