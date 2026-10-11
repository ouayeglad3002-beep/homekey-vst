#pragma once
#include <JuceHeader.h>
#include "MusicTheory.h"

// L'oreille de Spider : suit la hauteur (YIN), decoupe les notes, reconnait la gamme.
class Analyzer
{
public:
    struct NoteEvent
    {
        int64_t startSample = 0;   // index absolu (domaine d'entree)
        float midi = 60.0f;
        float f0 = 261.6f;
        float level = 0.0f;        // amplitude
        bool voiced = true;        // false = accord / bruit (mode sur)
    };

    void prepare (double sampleRate);
    // analyse un bloc mono ; index absolu du 1er echantillon ; renvoie les nouvelles notes
    void process (const float* mono, int n, int64_t firstSample, std::vector<NoteEvent>& out);

    Theory::Key key() const { return currentKey; }
    float keyConfidence() const { return confidence; }
    int   notesHeard() const { return heard; }
    float density() const { return noteRate; }        // notes / seconde
    float registerMidi() const { return avgMidi; }
    bool  isPlaying() const { return activity > 0.02f; }
    float currentMidi() const { return lastMidi; }

private:
    double sr = 44100.0, dsr = 11025.0;
    int factor = 4;
    // decimation
    juce::dsp::IIR::Filter<float> aa1, aa2;
    int decCount = 0;
    // YIN
    static constexpr int W = 320, maxBuf = 1024;
    std::array<float, maxBuf> ring {};
    int ringPos = 0, hopCount = 0, hop = 64;
    int tauMin = 7, tauMax = 184;
    std::vector<float> diff;
    // suivi de note
    std::array<float, 3> medBuf {};
    int medN = 0;
    float curNote = -1.0f, pendingNote = -1.0f, pendingSum = 0.0f;
    int pendingCount = 0, unvoicedCount = 0;
    float lastRmsDb = -100.0f, lastMidi = 0.0f;
    int64_t lastNoteSample = -1000000;
    // tonalite
    std::array<float, 12> chroma {};
    std::array<float, 12> fftChroma {};
    juce::dsp::FFT fft { 12 };
    std::vector<float> fftIn, fftWork, window;
    int fftFill = 0;
    Theory::Key currentKey;
    float confidence = 0.0f, candidateTime = 0.0f;
    Theory::Key candidate;
    int keyTimer = 0;
    // profil
    float noteRate = 0.0f, avgMidi = 64.0f, activity = 0.0f;
    int heard = 0;

    float yin (float& clarity);
    void updateKey();
};
