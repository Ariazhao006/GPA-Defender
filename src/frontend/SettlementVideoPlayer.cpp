#include "frontend/SettlementVideoPlayer.h"

#include "frontend/Renderer.h"
#include "frontend/SettlementVideoAudioDecoder.h"
#include "frontend/SettlementVideoDecoder.h"

#include <algorithm>

namespace frontend {

SettlementVideoPlayer::~SettlementVideoPlayer() {
    unload();
}

bool SettlementVideoPlayer::load(const std::string& path) {
    unload();

    decoder = new SettlementVideoDecoder();
    sourcePath = path;
    if (!decoder->open(path, width, height, frameDuration)) {
        unload();
        return false;
    }

    pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
    Image image = GenImageColor(width, height, BLACK);
    texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id == 0) {
        unload();
        return false;
    }

    loaded = true;
    finished = false;
    hasFrame = false;
    frameTimer = 0.0f;
    if (!readNextFrame()) {
        unload();
        return false;
    }
    startAudio(path);
    return true;
}

bool SettlementVideoPlayer::restart() {
    if (!loaded || decoder == nullptr || !decoder->isOpen()) return false;
    if (!decoder->restart()) return false;

    finished = false;
    hasFrame = false;
    frameTimer = 0.0f;
    if (!readNextFrame()) return false;
    stopAudio();
    startAudio(sourcePath);
    return true;
}

void SettlementVideoPlayer::unload() {
    stopAudio();
    if (texture.id != 0) {
        UnloadTexture(texture);
        texture = Texture2D{};
    }
    pixels.clear();
    delete decoder;
    decoder = nullptr;
    loaded = false;
    finished = false;
    hasFrame = false;
    width = 0;
    height = 0;
    sourcePath.clear();
}

void SettlementVideoPlayer::update(float dt) {
    if (!loaded || finished) return;

    updateAudio();
    frameTimer += dt;
    while (frameTimer >= frameDuration && !finished) {
        frameTimer -= frameDuration;
        readNextFrame();
    }
}

void SettlementVideoPlayer::draw() const {
    ClearBackground(BLACK);
    if (!loaded || !hasFrame || texture.id == 0) return;

    const float scale = std::min(SCREEN_WIDTH / static_cast<float>(width),
                                 SCREEN_HEIGHT / static_cast<float>(height));
    const float drawW = width * scale;
    const float drawH = height * scale;
    Rectangle src{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)};
    Rectangle dst{SCREEN_WIDTH / 2.0f - drawW / 2.0f,
                  SCREEN_HEIGHT / 2.0f - drawH / 2.0f,
                  drawW, drawH};
    DrawTexturePro(texture, src, dst, {0.0f, 0.0f}, 0.0f, WHITE);
}

bool SettlementVideoPlayer::readNextFrame() {
    if (!loaded || decoder == nullptr || !decoder->isOpen()) return false;

    if (!decoder->readFrame(pixels)) {
        finished = true;
        return false;
    }

    UpdateTexture(texture, pixels.data());
    hasFrame = true;
    return true;
}

void SettlementVideoPlayer::startAudio(const std::string& path) {
    stopAudio();

    audioDecoder = new SettlementVideoAudioDecoder();
    if (!audioDecoder->open(path, audioSampleRate, audioChannels)) {
        stopAudio();
        return;
    }

    SetAudioStreamBufferSizeDefault(4096);
    audioStream = LoadAudioStream(static_cast<unsigned int>(audioSampleRate), 16,
                                  static_cast<unsigned int>(audioChannels));
    if (!IsAudioStreamReady(audioStream)) {
        stopAudio();
        return;
    }

    audioLoaded = true;
    audioFinished = false;
    SetAudioStreamVolume(audioStream, 0.9f);

    queueAudioBuffer();
    queueAudioBuffer();
    PlayAudioStream(audioStream);
}

void SettlementVideoPlayer::stopAudio() {
    if (audioLoaded) {
        StopAudioStream(audioStream);
        UnloadAudioStream(audioStream);
        audioStream = AudioStream{};
    }
    delete audioDecoder;
    audioDecoder = nullptr;
    audioSamples.clear();
    audioLoaded = false;
    audioFinished = false;
    audioSampleRate = 44100;
    audioChannels = 2;
}

void SettlementVideoPlayer::updateAudio() {
    if (!audioLoaded || audioFinished || audioDecoder == nullptr) return;

    while (IsAudioStreamProcessed(audioStream)) {
        if (!queueAudioBuffer()) break;
    }
}

bool SettlementVideoPlayer::queueAudioBuffer() {
    if (!audioLoaded || audioFinished || audioDecoder == nullptr) return false;

    const int framesRead = audioDecoder->readFrames(audioSamples, 4096);
    if (framesRead <= 0) {
        audioFinished = true;
        return false;
    }

    UpdateAudioStream(audioStream, audioSamples.data(), framesRead);
    return true;
}

}  // namespace frontend
