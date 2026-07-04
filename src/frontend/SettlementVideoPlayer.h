#pragma once

#include "raylib.h"

#include <string>
#include <vector>

namespace frontend {

class SettlementVideoDecoder;

class SettlementVideoPlayer {
public:
    SettlementVideoPlayer() = default;
    ~SettlementVideoPlayer();

    SettlementVideoPlayer(const SettlementVideoPlayer&) = delete;
    SettlementVideoPlayer& operator=(const SettlementVideoPlayer&) = delete;

    bool load(const std::string& path);
    bool restart();
    void unload();
    void update(float dt);
    void draw() const;

    bool isLoaded() const { return loaded; }
    bool isFinished() const { return finished; }

private:
    bool readNextFrame();

    bool loaded = false;
    bool finished = false;
    bool hasFrame = false;
    float frameTimer = 0.0f;
    float frameDuration = 1.0f / 30.0f;
    int width = 0;
    int height = 0;
    std::string sourcePath;
    Texture2D texture{};
    std::vector<unsigned char> pixels;
    SettlementVideoDecoder* decoder = nullptr;
};

}  // namespace frontend
