/*---------------------------------------------------------*/
/*                                                         */
/*   audio_reactor.h - Audio-Reactive Spectrum Window     */
/*   Real-time FFT analysis and frequency visualization   */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef AUDIO_REACTOR_H
#define AUDIO_REACTOR_H

#define Uses_TView
#define Uses_TRect
#define Uses_TDrawBuffer
#define Uses_TColorAttr
#include <tvision/tv.h>
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <cmath>

// Forward declare generative music engine
class GenerativeMusicEngine;

/*---------------------------------------------------------*/
/* Audio Event Broadcasting                                */
/*---------------------------------------------------------*/
const ushort cmAudioBeat = 300;      // Beat detected
const ushort cmAudioFreqUpdate = 301; // Frequency bands updated

struct AudioFrequencyEvent {
    float bands[8];  // 8 frequency bands (0.0 - 1.0)
    float bass;      // Average of low frequencies
    float mid;       // Average of mid frequencies
    float treble;    // Average of high frequencies
    float volume;    // Overall volume/amplitude
};

/*---------------------------------------------------------*/
/* TAudioReactorView - Audio spectrum visualizer          */
/*---------------------------------------------------------*/
class TAudioReactorView : public TView
{
public:
    TAudioReactorView(const TRect& bounds);
    virtual ~TAudioReactorView();

    virtual void draw() override;
    virtual void handleEvent(TEvent& event) override;
    virtual void setState(ushort aState, Boolean enable) override;

    // Audio control methods
    bool loadAudio(const std::string& filePath);
    void play();
    void pause();
    void stop();
    bool isPlaying() const { return playing; }

    // Get current frequency analysis
    void getFrequencyBands(float bands[8]) const;

protected:
    // Audio state
    bool playing;
    bool loaded;
    std::string currentFile;

    // Generative music engine
    GenerativeMusicEngine* genMusic;

    // SDL2 audio device
    SDL_AudioDeviceID audioDevice;
    static TAudioReactorView* activeInstance;  // For audio callback

    // Timer for animation updates
    TTimerId timerId;
    unsigned updatePeriodMs;

    // Frequency band data (8 bands: 0-1 normalized)
    float freqBands[8];

    // FFT analysis helpers
    void updateFrequencyAnalysis();
    void broadcastAudioEvent();

    // Timer control
    void startTimer();
    void stopTimer();

    // Visualization helpers
    void drawSpectrum();
    void drawWaveform();
    void drawInfo();

    // Color mapping based on frequency
    TColorRGB getBandColor(int bandIndex, float intensity) const;

    // Simple mock FFT for POC (replaced with real analysis)
    void mockFFTAnalysis();

    // SDL2 audio callback (static)
    static void audioCallback(void* userdata, Uint8* stream, int len);

    // Process audio samples into frequency bands
    void processAudioSamples(const float* samples, int sampleCount);

    // Simple FFT helper (8-band approximation)
    void simpleFFT(const float* samples, int sampleCount);

private:
    int phase;  // Animation phase
    std::vector<float> audioBuffer;  // Ring buffer for FFT
};

#endif // AUDIO_REACTOR_H
