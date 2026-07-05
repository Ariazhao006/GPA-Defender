#pragma once

#include <string>
#include <vector>

namespace frontend {

class SettlementVideoAudioDecoder {
public:
    SettlementVideoAudioDecoder() = default;
    ~SettlementVideoAudioDecoder();

    SettlementVideoAudioDecoder(const SettlementVideoAudioDecoder&) = delete;
    SettlementVideoAudioDecoder& operator=(const SettlementVideoAudioDecoder&) = delete;

    bool open(const std::string& path, int& outSampleRate, int& outChannels);
    bool restart();
    int readFrames(std::vector<short>& outSamples, int maxFrames);
    void close();
    bool isOpen() const;

private:
    struct Impl;
    Impl* impl = nullptr;
    bool mediaFoundationStarted = false;
    int sampleRate = 44100;
    int channels = 2;
    std::vector<unsigned char> pendingBytes;
    std::size_t pendingOffset = 0;
    bool reachedEnd = false;
};

}  // namespace frontend
