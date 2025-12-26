/*---------------------------------------------------------*/
/*                                                         */
/*   generative_music.cpp - Generative Ambient Synth      */
/*   Implementation                                        */
/*                                                         */
/*---------------------------------------------------------*/

#include "generative_music.h"
#include <algorithm>
#include <cstdio>

// Pentatonic scales (frequencies in Hz)
const std::vector<float> GenerativeMusicEngine::PENTATONIC_C = {
    130.81f, 146.83f, 164.81f, 196.00f, 220.00f,  // C3-A3
    261.63f, 293.66f, 329.63f, 392.00f, 440.00f   // C4-A4
};

const std::vector<float> GenerativeMusicEngine::PENTATONIC_D = {
    146.83f, 164.81f, 185.00f, 220.00f, 246.94f,  // D3-B3
    293.66f, 329.63f, 369.99f, 440.00f, 493.88f   // D4-B4
};

const std::vector<float> GenerativeMusicEngine::DORIAN_A = {
    110.00f, 123.47f, 130.81f, 146.83f, 164.81f, 185.00f, 196.00f,  // A2-G3
    220.00f, 246.94f, 261.63f, 293.66f, 329.63f, 369.99f, 392.00f   // A3-G4
};

// High-octave scales for melody layer (+1 octave)
const std::vector<float> GenerativeMusicEngine::PENTATONIC_C_HIGH = {
    261.63f, 293.66f, 329.63f, 392.00f, 440.00f,  // C4-A4
    523.25f, 587.33f, 659.25f, 783.99f, 880.00f   // C5-A5
};

const std::vector<float> GenerativeMusicEngine::PENTATONIC_D_HIGH = {
    293.66f, 329.63f, 369.99f, 440.00f, 493.88f,  // D4-B4
    587.33f, 659.25f, 739.99f, 880.00f, 987.77f   // D5-B5
};

const std::vector<float> GenerativeMusicEngine::DORIAN_A_HIGH = {
    220.00f, 246.94f, 261.63f, 293.66f, 329.63f, 369.99f, 392.00f,  // A3-G4
    440.00f, 493.88f, 523.25f, 587.33f, 659.25f, 739.99f, 783.99f   // A4-G5
};

/*---------------------------------------------------------*/
/* Voice Implementation                                    */
/*---------------------------------------------------------*/

void Voice::evolve()
{
    // Slowly move towards target frequency and amplitude
    frequency += (targetFreq - frequency) * evolveRate;
    amplitude += (targetAmp - amplitude) * evolveRate;

    // Decrease change timer
    changeTimer--;
}

float Voice::generate(float sampleRate)
{
    // Generate sine wave
    float sample = std::sin(phase) * amplitude;

    // Advance phase
    phase += 2.0f * M_PI * frequency / sampleRate;

    // Wrap phase to prevent overflow
    if (phase >= 2.0f * M_PI) {
        phase -= 2.0f * M_PI;
    }

    return sample;
}

/*---------------------------------------------------------*/
/* GenerativeMusicEngine Implementation                    */
/*---------------------------------------------------------*/

GenerativeMusicEngine::GenerativeMusicEngine() :
    reverbPos(0),
    reverbAmount(0.3f),
    scaleSize(0)
{
    // Seed random number generator
    std::random_device rd;
    rng.seed(rd());

    // Start with pentatonic C scale
    setScale(PENTATONIC_C);

    // Create 6 voices for rich ambient texture
    setNumVoices(6);

    // Initialize reverb buffer (0.5 second delay at 44.1kHz)
    reverbBuffer.resize(44100 / 2, 0.0f);

    // Initialize frequency bands
    for (int i = 0; i < 8; i++) {
        freqBands[i] = 0.0f;
    }

    fprintf(stderr, "[GEN MUSIC] Initialized with 6 voices, pentatonic scale\n");
}

GenerativeMusicEngine::~GenerativeMusicEngine()
{
}

void GenerativeMusicEngine::setNumVoices(int n)
{
    voices.resize(n);

    // Initialize each voice with random parameters
    std::uniform_real_distribution<float> ampDist(0.1f, 0.4f);
    std::uniform_real_distribution<float> panDist(-0.7f, 0.7f);
    std::uniform_int_distribution<int> timeDist(44100 * 5, 44100 * 20);  // 5-20 seconds

    for (auto& voice : voices) {
        voice.frequency = randomScaleFrequency();
        voice.targetFreq = voice.frequency;
        voice.amplitude = ampDist(rng);
        voice.targetAmp = voice.amplitude;
        voice.evolveRate = 0.0001f + (rng() % 100) / 100000.0f;  // Very slow evolution
        voice.pan = panDist(rng);
        voice.changeTimer = timeDist(rng);
        voice.phase = 0.0f;
    }
}

void GenerativeMusicEngine::setScale(const std::vector<float>& frequencies)
{
    scaleSize = std::min((int)frequencies.size(), 12);
    for (int i = 0; i < scaleSize; i++) {
        currentScale[i] = frequencies[i];
    }
}

void GenerativeMusicEngine::setEvolveSpeed(float speed)
{
    // Adjust evolve rate for all voices
    for (auto& voice : voices) {
        voice.evolveRate = 0.0001f * (1.0f + speed * 10.0f);
    }
}

void GenerativeMusicEngine::setReverb(float amount)
{
    reverbAmount = std::max(0.0f, std::min(1.0f, amount));
}

float GenerativeMusicEngine::randomScaleFrequency()
{
    if (scaleSize == 0) return 440.0f;

    std::uniform_int_distribution<int> dist(0, scaleSize - 1);
    return currentScale[dist(rng)];
}

void GenerativeMusicEngine::generate(float* buffer, int frames, float sampleRate)
{
    std::uniform_real_distribution<float> ampDist(0.05f, 0.35f);

    for (int frame = 0; frame < frames; frame++) {
        float leftSample = 0.0f;
        float rightSample = 0.0f;

        // Mix all voices
        for (auto& voice : voices) {
            // Check if it's time to change target parameters
            if (voice.changeTimer <= 0) {
                voice.targetFreq = randomScaleFrequency();
                voice.targetAmp = ampDist(rng);
                voice.changeTimer = 44100 * (10 + rng() % 15);  // 10-25 seconds
            }

            // Evolve and generate
            voice.evolve();
            float sample = voice.generate(sampleRate);

            // Apply stereo panning
            float left = sample * (1.0f - std::max(0.0f, voice.pan));
            float right = sample * (1.0f + std::min(0.0f, voice.pan));

            leftSample += left;
            rightSample += right;
        }

        // Normalize
        leftSample /= voices.size();
        rightSample /= voices.size();

        // Simple reverb (delay + feedback)
        float reverbSample = reverbBuffer[reverbPos];
        leftSample += reverbSample * reverbAmount;
        rightSample += reverbSample * reverbAmount;

        reverbBuffer[reverbPos] = (leftSample + rightSample) * 0.5f * 0.5f;  // Feedback
        reverbPos = (reverbPos + 1) % reverbBuffer.size();

        // Write stereo interleaved
        buffer[frame * 2] = leftSample;
        buffer[frame * 2 + 1] = rightSample;
    }

    // Update frequency bands for visualization
    // Analyze voice frequencies and amplitudes
    for (int i = 0; i < 8; i++) {
        freqBands[i] = 0.0f;
    }

    for (const auto& voice : voices) {
        // Map frequency to band (log scale)
        int band = 0;
        if (voice.frequency < 150.0f) band = 0;        // Sub bass
        else if (voice.frequency < 250.0f) band = 1;   // Bass
        else if (voice.frequency < 400.0f) band = 2;   // Low mid
        else if (voice.frequency < 600.0f) band = 3;   // Mid
        else if (voice.frequency < 1000.0f) band = 4;  // Upper mid
        else if (voice.frequency < 2000.0f) band = 5;  // Presence
        else if (voice.frequency < 4000.0f) band = 6;  // Brilliance
        else band = 7;                                  // Air

        freqBands[band] += voice.amplitude;
    }

    // Normalize bands
    for (int i = 0; i < 8; i++) {
        freqBands[i] = std::min(1.0f, freqBands[i]);
    }
}

void GenerativeMusicEngine::getFrequencyBands(float bands[8]) const
{
    for (int i = 0; i < 8; i++) {
        bands[i] = freqBands[i];
    }
}
