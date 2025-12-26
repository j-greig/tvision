/*---------------------------------------------------------*/
/*                                                         */
/*   music_engine_manager.h - Music Engine Manager        */
/*   Singleton managing SDL audio device and mixing       */
/*                                                         */
/*---------------------------------------------------------*/

#ifndef MUSIC_ENGINE_MANAGER_H
#define MUSIC_ENGINE_MANAGER_H

#include <SDL2/SDL.h>
#include <map>
#include <string>
#include <vector>
#include <mutex>

// Forward declare
class GenerativeMusicEngine;

class MusicEngineManager {
public:
    // Singleton access
    static MusicEngineManager& getInstance();

    // Engine lifecycle
    void registerEngine(const std::string& layer, GenerativeMusicEngine* engine);
    void deregisterEngine(const std::string& layer);
    GenerativeMusicEngine* getEngine(const std::string& layer);

    // Scale synchronization
    void setScale(const std::string& layer, const std::vector<float>& scale);
    std::vector<float> getScale(const std::string& layer) const;

    // Audio control
    void play();
    void pause();
    bool isPlaying() const { return playing; }

private:
    MusicEngineManager();
    ~MusicEngineManager();

    // SDL audio
    SDL_AudioDeviceID audioDevice;
    bool playing;

    // Engine registry
    std::map<std::string, GenerativeMusicEngine*> engines;
    std::map<std::string, std::vector<float>> scaleRegistry;

    // Thread safety
    mutable std::mutex engineMutex;
    mutable std::mutex scaleMutex;

    // Audio callback (runs on audio thread)
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void mixEngines(float* buffer, int frames, float sampleRate);

    // Singleton pattern
    MusicEngineManager(const MusicEngineManager&) = delete;
    MusicEngineManager& operator=(const MusicEngineManager&) = delete;
};

#endif // MUSIC_ENGINE_MANAGER_H
