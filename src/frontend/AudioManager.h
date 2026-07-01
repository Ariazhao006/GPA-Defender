#pragma once

#include "raylib.h"

namespace frontend {

class AudioManager {
public:
    void init();
    void shutdown();
    void update();

    void playClick();
    void playTowerPlace();
    void playEnemyDeath();
    void playWaveStart();
    void playGameOver();
    void playVictory();

    void startBGM();
    void stopBGM();

private:
    Sound clickSfx;
    Sound towerSfx;
    Sound deathSfx;
    Sound waveSfx;
    Sound gameOverSfx;
    Sound victorySfx;

    // Two BGM paths: custom file (streamed) or procedural (preloaded)
    Music bgmStream;
    Sound bgmSound;
    bool usingCustomBGM = false;
    bool bgmPlaying = false;
    bool initialized = false;

    Sound generateTone(float frequency, float durationSec,
                       float sampleRate = 22050);
    Sound generateChirp(float startFreq, float endFreq, float durationSec,
                        float sampleRate = 22050);
    Sound generateBGM();
    bool tryLoadCustomBGM();
};

}  // namespace frontend
