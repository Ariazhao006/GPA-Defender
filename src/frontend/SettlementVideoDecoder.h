#pragma once

#include <string>
#include <vector>

namespace frontend {

class SettlementVideoDecoder {
public:
    SettlementVideoDecoder() = default;
    ~SettlementVideoDecoder();

    SettlementVideoDecoder(const SettlementVideoDecoder&) = delete;
    SettlementVideoDecoder& operator=(const SettlementVideoDecoder&) = delete;

    bool open(const std::string& path, int& outWidth, int& outHeight, float& outFrameDuration);
    bool restart();
    bool readFrame(std::vector<unsigned char>& rgbaPixels);
    void close();
    bool isOpen() const;

private:
    struct Impl;
    Impl* impl = nullptr;
    bool mediaFoundationStarted = false;
    int width = 0;
    int height = 0;
};

}  // namespace frontend
