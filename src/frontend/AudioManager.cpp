#include "frontend/AudioManager.h"

#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace frontend {

static Sound waveToSound(short* samples, int count, unsigned sampleRate) {
    Wave w;
    w.frameCount = static_cast<unsigned>(count);
    w.sampleRate = sampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = samples;
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

Sound AudioManager::generateTone(float frequency, float durationSec,
                                 float sampleRate) {
    int count = static_cast<int>(sampleRate * durationSec);
    short* buf = static_cast<short*>(RL_MALLOC(count * sizeof(short)));
    if (!buf) return {};

    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float env = 1.0f;
        float fadeLen = 0.008f;
        if (t < fadeLen) env = t / fadeLen;
        else if (t > durationSec - fadeLen) env = (durationSec - t) / fadeLen;
        buf[i] = static_cast<short>(sinf(2.0f * M_PI * frequency * t) * env * 10000);
    }
    return waveToSound(buf, count, static_cast<unsigned>(sampleRate));
}

Sound AudioManager::generateChirp(float startFreq, float endFreq,
                                  float durationSec, float sampleRate) {
    int count = static_cast<int>(sampleRate * durationSec);
    short* buf = static_cast<short*>(RL_MALLOC(count * sizeof(short)));
    if (!buf) return {};

    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float freq = startFreq + (endFreq - startFreq) * (t / durationSec);
        float env = 1.0f;
        float fadeLen = 0.005f;
        if (t < fadeLen) env = t / fadeLen;
        else if (t > durationSec - fadeLen) env = (durationSec - t) / fadeLen;
        buf[i] = static_cast<short>(sinf(2.0f * M_PI * freq * t) * env * 8000);
    }
    return waveToSound(buf, count, static_cast<unsigned>(sampleRate));
}

Sound AudioManager::generateBGM() {
    const float sampleRate = 22050;
    const float dur = 8.0f;
    int count = static_cast<int>(sampleRate * dur);
    short* buf = static_cast<short*>(RL_MALLOC(count * sizeof(short)));
    if (!buf) return {};

    struct Note { float freq; float start; float len; };
    Note bass[] = {
        {130.8f, 0.0f, 2.0f},
        {220.0f, 2.0f, 2.0f},
        {174.6f, 4.0f, 2.0f},
        {196.0f, 6.0f, 2.0f},
    };
    Note mel[] = {
        {261.6f, 0.0f, 0.4f}, {329.6f, 0.4f, 0.4f}, {392.0f, 0.8f, 0.4f},
        {329.6f, 1.2f, 0.8f},
        {440.0f, 2.0f, 0.4f}, {329.6f, 2.4f, 0.4f}, {261.6f, 2.8f, 0.4f},
        {440.0f, 3.2f, 0.8f},
        {349.2f, 4.0f, 0.4f}, {261.6f, 4.4f, 0.4f}, {349.2f, 4.8f, 0.4f},
        {392.0f, 5.2f, 0.4f}, {349.2f, 5.6f, 0.4f},
        {392.0f, 6.0f, 0.4f}, {440.0f, 6.4f, 0.4f}, {392.0f, 6.8f, 0.4f},
        {349.2f, 7.2f, 0.8f},
    };

    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / sampleRate;
        float sample = 0.0f;
        for (const auto& n : bass) {
            if (t >= n.start && t < n.start + n.len) {
                float lt = t - n.start;
                float env = 1.0f;
                if (lt < 0.02f) env = lt / 0.02f;
                else if (lt > n.len - 0.1f) env = (n.len - lt) / 0.1f;
                sample += sinf(2.0f * M_PI * n.freq * t) * env * 0.25f;
            }
        }
        for (const auto& n : mel) {
            if (t >= n.start && t < n.start + n.len) {
                float lt = t - n.start;
                float env = 1.0f;
                if (lt < 0.01f) env = lt / 0.01f;
                else if (lt > n.len - 0.05f) env = (n.len - lt) / 0.05f;
                sample += sinf(2.0f * M_PI * n.freq * t) * env * 0.30f;
                sample += sinf(2.0f * M_PI * n.freq * 2.0f * t) * env * 0.08f;
            }
        }
        sample += sinf(2.0f * M_PI * 261.6f * t) * 0.06f;
        sample += sinf(2.0f * M_PI * 329.6f * t) * 0.04f;
        float globalEnv = 1.0f;
        if (t < 0.3f) globalEnv = t / 0.3f;
        else if (t > dur - 0.5f) globalEnv = (dur - t) / 0.5f;
        sample *= globalEnv;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        buf[i] = static_cast<short>(sample * 12000);
    }
    return waveToSound(buf, count, static_cast<unsigned>(sampleRate));
}

bool AudioManager::tryLoadCustomBGM() {
    // Try several paths / formats; first match wins.
    const char* candidates[] = {
        "assets/bgm.mp3",
        "assets/bgm.wav",
        "assets/bgm.ogg",
        "assets/bgm.flac",
        "bgm.mp3",
        "bgm.wav",
    };
    for (const char* path : candidates) {
        if (FileExists(path)) {
            bgmStream = LoadMusicStream(path);
            if (IsMusicReady(bgmStream)) {
                SetMusicVolume(bgmStream, 0.35f);
                return true;
            }
        }
    }
    return false;
}

void AudioManager::init() {
    InitAudioDevice();

    clickSfx    = generateTone(880.0f, 0.06f);
    towerSfx    = generateChirp(400.0f, 900.0f, 0.18f);
    deathSfx    = generateChirp(350.0f, 80.0f, 0.25f);
    waveSfx     = generateChirp(200.0f, 600.0f, 0.35f);
    gameOverSfx = generateChirp(400.0f, 60.0f, 0.8f);
    victorySfx  = generateChirp(300.0f, 900.0f, 0.7f);

    SetSoundVolume(clickSfx, 0.4f);
    SetSoundVolume(towerSfx, 0.5f);
    SetSoundVolume(deathSfx, 0.5f);
    SetSoundVolume(waveSfx, 0.6f);
    SetSoundVolume(gameOverSfx, 0.7f);
    SetSoundVolume(victorySfx, 0.7f);

    // BGM: try custom file first, fall back to procedural
    usingCustomBGM = tryLoadCustomBGM();
    if (!usingCustomBGM) {
        bgmSound = generateBGM();
        SetSoundVolume(bgmSound, 0.35f);
    }

    initialized = true;
}

void AudioManager::shutdown() {
    if (!initialized) return;
    stopBGM();
    UnloadSound(clickSfx);
    UnloadSound(towerSfx);
    UnloadSound(deathSfx);
    UnloadSound(waveSfx);
    UnloadSound(gameOverSfx);
    UnloadSound(victorySfx);
    if (usingCustomBGM) {
        UnloadMusicStream(bgmStream);
    } else {
        UnloadSound(bgmSound);
    }
    CloseAudioDevice();
    initialized = false;
}

void AudioManager::update() {
    if (!bgmPlaying || !initialized) return;
    if (usingCustomBGM) {
        UpdateMusicStream(bgmStream);
        if (!IsMusicStreamPlaying(bgmStream)) {
            PlayMusicStream(bgmStream);
        }
    } else {
        if (!IsSoundPlaying(bgmSound)) {
            PlaySound(bgmSound);
        }
    }
}

void AudioManager::playClick()     { if (initialized) PlaySound(clickSfx); }
void AudioManager::playTowerPlace(){ if (initialized) PlaySound(towerSfx); }
void AudioManager::playEnemyDeath(){ if (initialized) PlaySound(deathSfx); }
void AudioManager::playWaveStart() { if (initialized) PlaySound(waveSfx); }
void AudioManager::playGameOver()  { if (initialized) PlaySound(gameOverSfx); }
void AudioManager::playVictory()   { if (initialized) PlaySound(victorySfx); }

void AudioManager::startBGM() {
    if (!initialized) return;
    bgmPlaying = true;
    if (usingCustomBGM) {
        PlayMusicStream(bgmStream);
    } else {
        PlaySound(bgmSound);
    }
}

void AudioManager::stopBGM() {
    bgmPlaying = false;
    if (!initialized) return;
    if (usingCustomBGM) {
        StopMusicStream(bgmStream);
    } else {
        StopSound(bgmSound);
    }
}

}  // namespace frontend
