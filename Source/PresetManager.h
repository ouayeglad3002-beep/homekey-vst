#pragma once
#include <JuceHeader.h>

// Gere les presets d'usine HomeKeys + les presets utilisateur (.hkpreset)
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    static juce::File getUserFolder();
    static constexpr const char* extension = ".hkpreset";

    void refresh();                               // relit le dossier utilisateur
    static void installFactoryPack();             // installe le pack integre (une seule fois par version)
    static int  importFrom (const juce::File& source); // dossier, .zip ou .hkpreset -> nb de presets ajoutes
    int  getNumPresets() const;
    int  getNumFactory() const;
    juce::String getPresetName (int index) const;
    bool isFactory (int index) const { return index < getNumFactory(); }
    juce::String getCategory (int index) const;   // nom du dossier du preset ("" = mes presets)

    void loadPreset (int index);
    bool savePreset (const juce::String& name);   // sauvegarde l'etat actuel
    bool deletePreset (int index);                // seulement les presets utilisateur
    void next();
    void previous();

    int  getCurrentIndex() const { return currentIndex; }
    juce::String getCurrentName() const { return currentName; }
    void setCurrentName (const juce::String& n);

private:
    struct Factory
    {
        const char* name;
        int type;
        float volume, tone, velocity, release, layer, chorus, reverb, size, width;
    };

    juce::AudioProcessorValueTreeState& apvts;
    juce::Array<juce::File> userFiles;
    int currentIndex = 0;
    juce::String currentName { "Init" };

    void setParam (const juce::String& id, float value);
    static const std::vector<Factory>& factory();
};
