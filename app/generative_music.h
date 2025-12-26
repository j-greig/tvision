/*---------------------------------------------------------*/
/*                                                         */
/*   generative_music.h - Brian Eno Style Generative Synth*/
/*   Evolving ambient soundscapes                          */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef GENERATIVE_MUSIC_H
#define GENERATIVE_MUSIC_H

#include <cmath>
#include <vector>
#include <random>

/*---------------------------------------------------------*/
/* Voice - Single evolving tone generator                 */
/*---------------------------------------------------------*/
struct Voice {
    float frequency;      // Current frequency in Hz
    float phase;          // Current phase (0-2π)
    float amplitude;      // Current amplitude (0-1)
    float targetFreq;     // Target frequency for evolution
    float targetAmp;      // Target amplitude
    float evolveRate;     // How fast to evolve (0-1)
    float pan;            // Stereo pan (-1 left, +1 right)
    int changeTimer;      // Frames until next random change

    Voice() : frequency(440.0f), phase(0.0f), amplitude(0.3f),
              targetFreq(440.0f), targetAmp(0.3f), evolveRate(0.001f),
              pan(0.0f), changeTimer(44100 * 10) {}

    void evolve();
    float generate(float sampleRate);
};

/*---------------------------------------------------------*/
/* GenerativeMusicEngine - Eno-style ambient generator    */
/*---------------------------------------------------------*/
class GenerativeMusicEngine {
public:
    GenerativeMusicEngine();
    ~GenerativeMusicEngine();

    // Generate audio samples (stereo interleaved)
    void generate(float* buffer, int frames, float sampleRate);

    // Control parameters
    void setNumVoices(int n);
    void setScale(const std::vector<float>& frequencies);
    void setEvolveSpeed(float speed);  // 0.0 = static, 1.0 = fast changes
    void setReverb(float amount);       // Reverb depth (0-1)

    // Get current frequency bands for visualization
    void getFrequencyBands(float bands[8]) const;

    // Pentatonic scale in various keys (bass layer: 110-494 Hz)
    static const std::vector<float> PENTATONIC_C;
    static const std::vector<float> PENTATONIC_D;
    static const std::vector<float> DORIAN_A;

    // High-octave scales for melody layer (+1 octave: 220-988 Hz)
    static const std::vector<float> PENTATONIC_C_HIGH;
    static const std::vector<float> PENTATONIC_D_HIGH;
    static const std::vector<float> DORIAN_A_HIGH;

private:
    std::vector<Voice> voices;
    std::mt19937 rng;

    // Simple reverb buffer
    std::vector<float> reverbBuffer;
    int reverbPos;
    float reverbAmount;

    // Current frequency analysis for visualization
    mutable float freqBands[8];

    // Choose random frequency from current scale
    float randomScaleFrequency();
    float currentScale[12];
    int scaleSize;
};

#endif // GENERATIVE_MUSIC_H
