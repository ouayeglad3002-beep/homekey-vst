#pragma once
#include <JuceHeader.h>

// Generateur de sons aleatoires mais coherents de HomeKey I.
// Chaque monde a sa "recette" (plages de reglages, cibles favorites des etoiles).
// MUTATION : 0 = petite variation du son actuel, 1 = son totalement nouveau.
namespace SoundDesigner
{
    // nouveau son complet (sources, effets, grain, atmosphere + constellation). Renvoie le nom genere.
    juce::String randomizeSound (juce::AudioProcessorValueTreeState&, float mutation, bool keepWorld);
    // nouvelle constellation seulement (positions, cibles, formes, vitesse, chaos)
    void randomizeStars (juce::AudioProcessorValueTreeState&, float mutation);
    juce::String makeName (int world);
}
