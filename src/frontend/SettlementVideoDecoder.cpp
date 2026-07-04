#include "frontend/SettlementVideoDecoder.h"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace frontend {
namespace {

std::wstring widenPath(const std::string& path) {
    return std::filesystem::u8path(path).wstring();
}

void copyBgraToRgba(const unsigned char* src,
                    long stride,
                    int width,
                    int height,
                    std::vector<unsigned char>& dst) {
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4u;
    const std::size_t byteCount = rowBytes * static_cast<std::size_t>(height);
    if (dst.size() != byteCount) dst.assign(byteCount, 0);

    const bool bottomUp = stride < 0;
    const std::size_t absStride = static_cast<std::size_t>(bottomUp ? -stride : stride);
    for (int y = 0; y < height; ++y) {
        const int srcY = bottomUp ? (height - 1 - y) : y;
        const unsigned char* srcRow = src + static_cast<std::size_t>(srcY) * absStride;
        unsigned char* dstRow = dst.data() + static_cast<std::size_t>(y) * rowBytes;
        for (int x = 0; x < width; ++x) {
            const int i = x * 4;
            dstRow[i + 0] = srcRow[i + 2];
            dstRow[i + 1] = srcRow[i + 1];
            dstRow[i + 2] = srcRow[i + 0];
            dstRow[i + 3] = 255;
        }
    }
}

}  // namespace

struct SettlementVideoDecoder::Impl {
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
};

SettlementVideoDecoder::~SettlementVideoDecoder() {
    close();
}

bool SettlementVideoDecoder::open(const std::string& path,
                                  int& outWidth,
                                  int& outHeight,
                                  float& outFrameDuration) {
    close();

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;
    mediaFoundationStarted = true;

    impl = new Impl();
    Microsoft::WRL::ComPtr<IMFAttributes> attributes;
    hr = MFCreateAttributes(&attributes, 2);
    if (FAILED(hr)) {
        std::cerr << "[Video] MFCreateAttributes failed: 0x" << std::hex << hr << std::dec << "\n";
        close();
        return false;
    }
    attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

    hr = MFCreateSourceReaderFromURL(widenPath(path).c_str(), attributes.Get(), &impl->reader);
    if (FAILED(hr)) {
        std::cerr << "[Video] MFCreateSourceReaderFromURL failed: 0x" << std::hex << hr
                  << std::dec << " path=" << path << "\n";
        close();
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
    hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr)) {
        std::cerr << "[Video] MFCreateMediaType failed: 0x" << std::hex << hr << std::dec << "\n";
        close();
        return false;
    }
    mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    hr = impl->reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, mediaType.Get());
    if (FAILED(hr)) {
        std::cerr << "[Video] Set RGB32 media type failed: 0x" << std::hex << hr << std::dec << "\n";
        close();
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> currentType;
    hr = impl->reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType);
    if (FAILED(hr)) {
        std::cerr << "[Video] GetCurrentMediaType failed: 0x" << std::hex << hr << std::dec << "\n";
        close();
        return false;
    }

    UINT32 videoW = 0;
    UINT32 videoH = 0;
    hr = MFGetAttributeSize(currentType.Get(), MF_MT_FRAME_SIZE, &videoW, &videoH);
    if (FAILED(hr) || videoW == 0 || videoH == 0) {
        std::cerr << "[Video] Invalid frame size: 0x" << std::hex << hr << std::dec << "\n";
        close();
        return false;
    }
    width = static_cast<int>(videoW);
    height = static_cast<int>(videoH);
    outWidth = width;
    outHeight = height;

    UINT32 fpsNumerator = 0;
    UINT32 fpsDenominator = 0;
    if (SUCCEEDED(MFGetAttributeRatio(currentType.Get(), MF_MT_FRAME_RATE,
                                      &fpsNumerator, &fpsDenominator))
        && fpsNumerator > 0 && fpsDenominator > 0) {
        outFrameDuration = static_cast<float>(fpsDenominator) / static_cast<float>(fpsNumerator);
    } else {
        outFrameDuration = 1.0f / 30.0f;
    }

    return true;
}

bool SettlementVideoDecoder::restart() {
    if (!isOpen()) return false;

    PROPVARIANT pos;
    PropVariantInit(&pos);
    pos.vt = VT_I8;
    pos.hVal.QuadPart = 0;
    HRESULT hr = impl->reader->SetCurrentPosition(GUID_NULL, pos);
    PropVariantClear(&pos);
    return SUCCEEDED(hr);
}

bool SettlementVideoDecoder::readFrame(std::vector<unsigned char>& rgbaPixels) {
    if (!isOpen()) return false;

    while (true) {
        DWORD streamIndex = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        Microsoft::WRL::ComPtr<IMFSample> sample;
        HRESULT hr = impl->reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                              0, &streamIndex, &flags, &timestamp, &sample);
        if (FAILED(hr)) {
            std::cerr << "[Video] ReadSample failed: 0x" << std::hex << hr << std::dec << "\n";
            return false;
        }
        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) return false;
        if (sample == nullptr) continue;

        Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
        hr = sample->ConvertToContiguousBuffer(&buffer);
        if (FAILED(hr)) continue;

        Microsoft::WRL::ComPtr<IMF2DBuffer> buffer2D;
        if (SUCCEEDED(buffer.As(&buffer2D))) {
            BYTE* scanline0 = nullptr;
            LONG stride = 0;
            hr = buffer2D->Lock2D(&scanline0, &stride);
            if (FAILED(hr) || scanline0 == nullptr) continue;
            copyBgraToRgba(scanline0, stride, width, height, rgbaPixels);
            buffer2D->Unlock2D();
            return true;
        }

        BYTE* data = nullptr;
        DWORD maxLength = 0;
        DWORD currentLength = 0;
        hr = buffer->Lock(&data, &maxLength, &currentLength);
        if (FAILED(hr) || data == nullptr) continue;

        const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
        if (currentLength >= expected) copyBgraToRgba(data, width * 4, width, height, rgbaPixels);
        buffer->Unlock();
        if (currentLength >= expected) return true;
    }
}

void SettlementVideoDecoder::close() {
    delete impl;
    impl = nullptr;
    width = 0;
    height = 0;
    if (mediaFoundationStarted) {
        MFShutdown();
        mediaFoundationStarted = false;
    }
}

bool SettlementVideoDecoder::isOpen() const {
    return impl != nullptr && impl->reader != nullptr;
}

}  // namespace frontend

#else

namespace frontend {

struct SettlementVideoDecoder::Impl {};

SettlementVideoDecoder::~SettlementVideoDecoder() = default;
bool SettlementVideoDecoder::open(const std::string&, int&, int&, float&) { return false; }
bool SettlementVideoDecoder::restart() { return false; }
bool SettlementVideoDecoder::readFrame(std::vector<unsigned char>&) { return false; }
void SettlementVideoDecoder::close() {}
bool SettlementVideoDecoder::isOpen() const { return false; }

}  // namespace frontend

#endif
