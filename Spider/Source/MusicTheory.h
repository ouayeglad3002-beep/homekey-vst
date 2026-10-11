#pragma once
#include <array>
#include <cmath>
#include <cstdint>

// Theorie musicale de Spider : gammes, degres, profils tonaux.
namespace Theory
{
    inline const std::array<int, 7>& scale (bool minor)
    {
        static const std::array<int, 7> maj { 0, 2, 4, 5, 7, 9, 11 };
        static const std::array<int, 7> min { 0, 2, 3, 5, 7, 8, 10 };
        return minor ? min : maj;
    }

    // Profils de Krumhansl-Kessler (perception de la tonalite)
    inline const std::array<float, 12>& profile (bool minor)
    {
        static const std::array<float, 12> maj { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
        static const std::array<float, 12> min { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };
        return minor ? min : maj;
    }

    struct Key { int tonic = 0; bool minor = true; };

    inline const char* noteName (int pc)
    {
        static const char* n[12] = { "DO", "DO#", "RE", "MIB", "MI", "FA", "FA#", "SOL", "LAB", "LA", "SIB", "SI" };
        return n[((pc % 12) + 12) % 12];
    }

    // Degre absolu (octave*7 + index) le plus proche d'une hauteur MIDI, et l'ecart (notes hors gamme)
    struct Degree { int deg = 0; float offset = 0.0f; };

    inline float midiOfDegree (int deg, const Key& k)
    {
        const auto& sc = scale (k.minor);
        const int oct = (int) std::floor (deg / 7.0);
        const int idx = deg - oct * 7;
        return (float) (12 * oct + k.tonic + sc[(size_t) idx]);
    }

    inline Degree degreeOf (float midi, const Key& k)
    {
        // recherche autour de l'octave estimee
        const int approx = (int) std::floor ((midi - k.tonic) / 12.0f * 7.0f);
        Degree best; float bestD = 1.0e9f;
        for (int d = approx - 4; d <= approx + 4; ++d)
        {
            const float m = midiOfDegree (d, k);
            const float dist = std::abs (midi - m);
            if (dist < bestD) { bestD = dist; best.deg = d; best.offset = midi - m; }
        }
        return best;
    }

    inline float ratioFromSemis (float s) { return std::pow (2.0f, s / 12.0f); }

    // hachage deterministe -> [0,1)
    inline float hash01 (uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA6Bu ^ (c + 0x165667B1u) * 0xC2B2AE35u ^ (d + 0x27D4EB2Fu) * 0x27D4EB2Du;
        h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
        return (float) (h & 0xFFFFFF) / (float) 0x1000000;
    }
}
