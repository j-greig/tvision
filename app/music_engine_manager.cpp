/*---------------------------------------------------------*/
/*                                                         */
/*   music_engine_manager.cpp - Music Engine Manager      */
/*   Implementation                                        */
/*                                                         */
/*---------------------------------------------------------*/

#include "music_engine_manager.h"
#include "generative_music.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

MusicEngineManager::MusicEngineManager() :
    audioDevice(0),
    playing(false)
{
    // Initialize SDL audio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "[MUSIC MGR] SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    // Set up SDL2 audio spec (same as original TAudioReactorView)
    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_F32;  // Float samples
    want.channels = 2;
    want.samples = 2048;
    want.callback = audioCallback;
    want.userdata = this;

    audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audioDevice == 0) {
        fprintf(stderr, "[MUSIC MGR] Failed to open audio device: %s\n", SDL_GetError());
    } else {
        fprintf(stderr, "[MUSIC MGR] Opened audio device: %d Hz, %d channels\n",
                have.freq, have.channels);
        // Start paused
        SDL_PauseAudioDevice(audioDevice, 1);
    }
}

MusicEngineManager::~MusicEngineManager()
{
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }
    SDL_Quit();
}

MusicEngineManager& MusicEngineManager::getInstance()
{
    static MusicEngineManager instance;
    return instance;
}

void MusicEngineManager::registerEngine(const std::string& layer, GenerativeMusicEngine* engine)
{
    std::lock_guard<std::mutex> lock(engineMutex);
    engines[layer] = engine;
    fprintf(stderr, "[MUSIC MGR] Registered engine: %s (total: %zu)\n",
            layer.c_str(), engines.size());
}

void MusicEngineManager::deregisterEngine(const std::string& layer)
{
    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = engines.find(layer);
    if (it != engines.end()) {
        engines.erase(it);
        fprintf(stderr, "[MUSIC MGR] Deregistered engine: %s (remaining: %zu)\n",
                layer.c_str(), engines.size());
    }

    // Also remove from scale registry
    {
        std::lock_guard<std::mutex> scaleLock(scaleMutex);
        scaleRegistry.erase(layer);
    }
}

GenerativeMusicEngine* MusicEngineManager::getEngine(const std::string& layer)
{
    std::lock_guard<std::mutex> lock(engineMutex);
    auto it = engines.find(layer);
    return (it != engines.end()) ? it->second : nullptr;
}

void MusicEngineManager::setScale(const std::string& layer, const std::vector<float>& scale)
{
    std::lock_guard<std::mutex> lock(scaleMutex);
    scaleRegistry[layer] = scale;

    // Update engine if it exists
    GenerativeMusicEngine* engine = getEngine(layer);
    if (engine) {
        engine->setScale(scale);
    }
}

std::vector<float> MusicEngineManager::getScale(const std::string& layer) const
{
    std::lock_guard<std::mutex> lock(scaleMutex);
    auto it = scaleRegistry.find(layer);
    return (it != scaleRegistry.end()) ? it->second : std::vector<float>();
}

void MusicEngineManager::play()
{
    if (audioDevice != 0) {
        SDL_PauseAudioDevice(audioDevice, 0);  // Unpause
        playing = true;
        fprintf(stderr, "[MUSIC MGR] Playing (%zu engines)\n", engines.size());
    }
}

void MusicEngineManager::pause()
{
    if (audioDevice != 0) {
        SDL_PauseAudioDevice(audioDevice, 1);  // Pause
        playing = false;
        fprintf(stderr, "[MUSIC MGR] Paused\n");
    }
}

void MusicEngineManager::audioCallback(void* userdata, Uint8* stream, int len)
{
    MusicEngineManager* mgr = static_cast<MusicEngineManager*>(userdata);
    if (!mgr) {
        SDL_memset(stream, 0, len);
        return;
    }

    float* floatStream = (float*)stream;
    int frames = len / (sizeof(float) * 2);  // Stereo float samples

    mgr->mixEngines(floatStream, frames, 44100.0f);
}

void MusicEngineManager::mixEngines(float* buffer, int frames, float sampleRate)
{
    std::lock_guard<std::mutex> lock(engineMutex);

    if (engines.empty()) {
        // Silence
        std::memset(buffer, 0, frames * 2 * sizeof(float));
        return;
    }

    // Clear output
    std::memset(buffer, 0, frames * 2 * sizeof(float));

    // Mix all engines
    std::vector<float> tempBuffer(frames * 2);
    for (auto& pair : engines) {
        GenerativeMusicEngine* engine = pair.second;

        // Generate into temp buffer
        std::memset(tempBuffer.data(), 0, tempBuffer.size() * sizeof(float));
        engine->generate(tempBuffer.data(), frames, sampleRate);

        // Add to mix
        for (int i = 0; i < frames * 2; i++) {
            buffer[i] += tempBuffer[i];
        }
    }

    // Prevent clipping: divide by sqrt(N) for headroom
    float mixDivisor = std::sqrt(static_cast<float>(engines.size()));
    for (int i = 0; i < frames * 2; i++) {
        buffer[i] /= mixDivisor;

        // Hard clipping as safety net
        buffer[i] = std::max(-1.0f, std::min(1.0f, buffer[i]));
    }
}
