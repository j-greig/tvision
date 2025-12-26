/*---------------------------------------------------------*/
/*                                                         */
/*   audio_reactor.cpp - Audio-Reactive Spectrum View     */
/*   Implementation                                        */
/*                                                         */
/*---------------------------------------------------------*/

#include "audio_reactor.h"
#include "generative_music.h"
#include "music_engine_manager.h"
#include <cstring>
#include <algorithm>

#define Uses_TEvent
#define Uses_TProgram
#include <tvision/tv.h>

/*---------------------------------------------------------*/
/* TAudioReactorView Implementation                       */
/*---------------------------------------------------------*/

TAudioReactorView::TAudioReactorView(const TRect& bounds, const std::string& layer) :
    TView(bounds),
    playing(false),
    loaded(true),  // Always loaded (generative)
    engineLayer(layer),
    timerId(0),
    updatePeriodMs(50),  // 20 FPS for spectrum updates
    phase(0),
    genMusic(nullptr)
{
    growMode = gfGrowHiX | gfGrowHiY;
    eventMask |= evBroadcast;

    // Create generative music engine
    genMusic = new GenerativeMusicEngine();

    // Set scale based on layer
    if (layer == "bass") {
        genMusic->setScale(GenerativeMusicEngine::PENTATONIC_C);
        currentFile = "Generative Ambient (Bass)";
    } else if (layer == "melody") {
        genMusic->setScale(GenerativeMusicEngine::PENTATONIC_C_HIGH);
        currentFile = "Generative Melody (Treble)";
    } else {
        genMusic->setScale(GenerativeMusicEngine::PENTATONIC_C);
        currentFile = "Generative Ambient";
    }

    // Register engine with manager (manager owns SDL device)
    MusicEngineManager::getInstance().registerEngine(layer, genMusic);
    fprintf(stderr, "[AUDIO] Created %s layer view\n", layer.c_str());

    // Initialize frequency bands to VISIBLE VALUES
    for (int i = 0; i < 8; i++) {
        freqBands[i] = 0.5f;  // 50% height to start
    }

    // Timer will be started when view becomes exposed (setState override)
}

TAudioReactorView::~TAudioReactorView()
{
    stopTimer();

    // Deregister engine from manager BEFORE deleting
    MusicEngineManager::getInstance().deregisterEngine(engineLayer);

    if (genMusic) {
        delete genMusic;
        genMusic = nullptr;
    }
}

void TAudioReactorView::startTimer()
{
    if (timerId == 0) {
        timerId = setTimer(updatePeriodMs, updatePeriodMs);
    }
}

void TAudioReactorView::stopTimer()
{
    if (timerId != 0) {
        killTimer(timerId);
        timerId = 0;
    }
}

void TAudioReactorView::setState(ushort aState, Boolean enable)
{
    TView::setState(aState, enable);

    if ((aState & sfExposed) != 0) {
        if (enable) {
            // View is being exposed - start animation
            phase = 0;
            startTimer();
            drawView();
        } else {
            // View is being hidden - stop animation
            stopTimer();
        }
    }
}

void TAudioReactorView::handleEvent(TEvent& event)
{
    TView::handleEvent(event);

    if (event.what == evBroadcast && event.message.command == cmTimerExpired) {
        if (playing || true) {  // Always update for demo purposes
            updateFrequencyAnalysis();
            drawView();
            broadcastAudioEvent();
        }
    }
}

bool TAudioReactorView::loadAudio(const std::string& filePath)
{
    // Generative mode - no file loading needed
    currentFile = "Generative: " + filePath;
    loaded = true;
    return true;
}

void TAudioReactorView::play()
{
    MusicEngineManager::getInstance().play();
    playing = true;
    fprintf(stderr, "[AUDIO] Playing %s layer\n", engineLayer.c_str());
}

void TAudioReactorView::pause()
{
    auto& mgr = MusicEngineManager::getInstance();
    if (mgr.isPlaying()) {
        mgr.pause();
        playing = false;
    } else {
        mgr.play();
        playing = true;
    }
}

void TAudioReactorView::stop()
{
    MusicEngineManager::getInstance().pause();
    playing = false;
    phase = 0;
    std::memset(freqBands, 0, sizeof(freqBands));
    drawView();
}

void TAudioReactorView::getFrequencyBands(float bands[8]) const
{
    std::memcpy(bands, freqBands, sizeof(freqBands));
}

void TAudioReactorView::updateFrequencyAnalysis()
{
    // Frequency data is updated in real-time by audio callback
    // from generative music engine - nothing to do here
    if (genMusic) {
        genMusic->getFrequencyBands(freqBands);
    }
}

void TAudioReactorView::mockFFTAnalysis()
{
    // Generate realistic-looking frequency data using sine waves
    phase++;

    for (int i = 0; i < 8; i++) {
        // Each band oscillates at different frequencies
        float freq = 0.02f + (i * 0.01f);  // Slower oscillation
        float wave = std::sin(phase * freq + i);

        // Convert sine wave (-1 to 1) to amplitude (0.2 to 1.0)
        float amplitude = 0.6f + 0.4f * wave;

        // Add some bass emphasis (lower bands stronger)
        if (i < 2) {
            amplitude = std::min(1.0f, amplitude * 1.2f);
        }

        // Ensure in valid range
        freqBands[i] = std::max(0.2f, std::min(1.0f, amplitude));
    }
}

void TAudioReactorView::broadcastAudioEvent()
{
    // Calculate aggregate frequency values
    float bass = (freqBands[0] + freqBands[1]) / 2.0f;
    float mid = (freqBands[2] + freqBands[3] + freqBands[4]) / 3.0f;
    float treble = (freqBands[5] + freqBands[6] + freqBands[7]) / 3.0f;

    float totalVolume = 0;
    for (int i = 0; i < 8; i++) {
        totalVolume += freqBands[i];
    }
    float volume = totalVolume / 8.0f;

    // TODO: Broadcast audio event to other windows
    // This will be implemented when we add the event system
}

TColorRGB TAudioReactorView::getBandColor(int bandIndex, float intensity) const
{
    // Monochrome: white with varying intensity
    uint8_t value = (uint8_t)(0xFF * intensity);
    return TColorRGB(value, value, value);
}

void TAudioReactorView::draw()
{
    drawSpectrum();
    drawInfo();
}

void TAudioReactorView::drawSpectrum()
{
    const int W = size.x;
    const int H = size.y - 2;  // Leave room for info line

    if (W <= 0 || H <= 0) return;

    // Calculate bar width for 8 bands
    int barWidth = std::max(1, W / 8);
    int spacing = 1;

    for (int y = 0; y < H; y++) {
        TDrawBuffer b;  // Create fresh buffer for each line

        for (int x = 0; x < W; x++) {
            // Determine which band this column belongs to
            int bandIndex = x / (barWidth + spacing);
            if (bandIndex >= 8) bandIndex = 7;

            // Check if we're in the spacing between bars
            bool inSpacing = (x % (barWidth + spacing)) == barWidth;

            if (inSpacing) {
                // Black spacing between bars
                TColorRGB black(0, 0, 0);
                TColorAttr attr(black, black);
                b.moveChar(x, ' ', attr, 1);
            } else {
                // Calculate bar height for this band
                int barHeight = (int)(freqBands[bandIndex] * H);

                // Draw from bottom up
                int yFromBottom = H - 1 - y;

                if (yFromBottom < barHeight) {
                    // Inside the bar - use shade characters for vertical gradient
                    // Full white at bottom, fading at top
                    float positionInBar = (float)yFromBottom / barHeight;

                    // Select shade character based on position (bottom to top)
                    char shadeChar;
                    float intensity;

                    if (positionInBar < 0.25f) {
                        // Bottom 25% - full block, brightest
                        shadeChar = '\xDB';  // █ Full block
                        intensity = 1.0f;
                    } else if (positionInBar < 0.50f) {
                        // 25-50% - dark shade
                        shadeChar = '\xB2';  // ▓ Dark shade
                        intensity = 0.9f;
                    } else if (positionInBar < 0.75f) {
                        // 50-75% - medium shade
                        shadeChar = '\xB1';  // ▒ Medium shade
                        intensity = 0.8f;
                    } else {
                        // Top 25% - light shade
                        shadeChar = '\xB0';  // ░ Light shade
                        intensity = 0.7f;
                    }

                    TColorRGB color = getBandColor(bandIndex, intensity);
                    TColorAttr attr(color, color);
                    b.moveChar(x, shadeChar, attr, 1);
                } else {
                    // Above the bar - draw background
                    TColorRGB bg(0x10, 0x10, 0x10);
                    TColorAttr attr(bg, bg);
                    b.moveChar(x, ' ', attr, 1);
                }
            }
        }
        writeLine(0, y, W, 1, b);
    }
}

void TAudioReactorView::drawInfo()
{
    TDrawBuffer b;
    const int W = size.x;
    const int H = size.y;

    if (H < 2) return;

    // Draw status line at bottom
    TColorRGB fg(0xFF, 0xFF, 0xFF);
    TColorRGB bg(0x20, 0x20, 0x20);
    TColorAttr attr(fg, bg);

    std::string status;
    if (playing) {
        status = "\x10 PLAYING";  // Play symbol
    } else if (loaded) {
        status = "\x13\x13 PAUSED";  // Pause symbol
    } else {
        status = "AUDIO REACTOR [POC - Mock FFT]";
    }

    if (!currentFile.empty()) {
        status += " | " + currentFile;
    }

    // Pad to width
    while ((int)status.length() < W) {
        status += ' ';
    }
    if ((int)status.length() > W) {
        status = status.substr(0, W);
    }

    b.moveStr(0, status.c_str(), attr);
    writeLine(0, H - 1, W, 1, b);
}

// Audio callback now handled by MusicEngineManager

void TAudioReactorView::processAudioSamples(const float* samples, int sampleCount)
{
    // Simple band-pass filtering approach for 8 frequency bands
    // This is a very simplified FFT approximation

    // Clear frequency bands
    for (int i = 0; i < 8; i++) {
        freqBands[i] = 0.0f;
    }

    if (sampleCount < 64) return;  // Need minimum samples

    // Divide audio into 8 rough frequency ranges
    // Band 0-1: Bass (20-250 Hz)
    // Band 2-4: Mid (250-2000 Hz)
    // Band 5-7: Treble (2000-20000 Hz)

    // Simple energy calculation per band
    int samplesPerBand = sampleCount / 8;

    for (int band = 0; band < 8; band++) {
        float energy = 0.0f;
        int startIdx = band * samplesPerBand;
        int endIdx = startIdx + samplesPerBand;

        for (int i = startIdx; i < endIdx && i < sampleCount; i++) {
            energy += samples[i] * samples[i];  // Square for energy
        }

        // Normalize and apply logarithmic scaling
        energy = std::sqrt(energy / samplesPerBand);
        energy = std::min(1.0f, energy * 3.0f);  // Amplify

        // Smooth with previous value
        freqBands[band] = freqBands[band] * 0.7f + energy * 0.3f;
    }
}

void TAudioReactorView::simpleFFT(const float* samples, int sampleCount)
{
    // TODO: Implement proper FFT if needed
    // For now, processAudioSamples() does a simple approximation
    processAudioSamples(samples, sampleCount);
}
