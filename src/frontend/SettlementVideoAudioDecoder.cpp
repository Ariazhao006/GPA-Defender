#include "frontend/SettlementVideoAudioDecoder.h"

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
#include <cstring>
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

}  // namespace

struct SettlementVideoAudioDecoder::Impl {
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
};

SettlementVideoAudioDecoder::~SettlementVideoAudioDecoder() {
    close();
}

bool SettlementVideoAudioDecoder::open(const std::string& path, int& outSampleRate, int& outChannels) {
    close();

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;
    mediaFoundationStarted = true;

    impl = new Impl();

    Microsoft::WRL::ComPtr<IMFAttributes> attributes;
    hr = MFCreateAttributes(&attributes, 2);
    if (FAILED(hr)) {
        close();
        return false;
    }
    attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
    attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);

    hr = MFCreateSourceReaderFromURL(widenPath(path).c_str(), attributes.Get(), &impl->reader);
    if (FAILED(hr)) {
        std::cerr << "[VideoAudio] MFCreateSourceReaderFromURL failed: 0x" << std::hex << hr
                  << std::dec << " path=" << path << "\n";
        close();
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
    hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr)) {
        close();
        return false;
    }
    mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    mediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);

    hr = impl->reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaType.Get());
    if (FAILED(hr)) {
        std::cerr << "[VideoAudio] No playable audio stream: 0x" << std::hex << hr << std::dec << "\n";
        close();
        return false;
    }

    Microsoft::WRL::ComPtr<IMFMediaType> currentType;
    hr = impl->reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &currentType);
    if (FAILED(hr)) {
        close();
        return false;
    }

    UINT32 rate = 0;
    UINT32 channelCount = 0;
    UINT32 bits = 0;
    currentType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate);
    currentType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channelCount);
    currentType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits);
    if (rate == 0 || channelCount == 0 || bits != 16) {
        std::cerr << "[VideoAudio] Unsupported PCM format: rate=" << rate
                  << " channels=" << channelCount << " bits=" << bits << "\n";
        close();
        return false;
    }

    sampleRate = static_cast<int>(rate);
    channels = static_cast<int>(channelCount);
    outSampleRate = sampleRate;
    outChannels = channels;
    reachedEnd = false;
    pendingBytes.clear();
    pendingOffset = 0;
    return true;
}

bool SettlementVideoAudioDecoder::restart() {
    if (!isOpen()) return false;

    PROPVARIANT pos;
    PropVariantInit(&pos);
    pos.vt = VT_I8;
    pos.hVal.QuadPart = 0;
    HRESULT hr = impl->reader->SetCurrentPosition(GUID_NULL, pos);
    PropVariantClear(&pos);
    pendingBytes.clear();
    pendingOffset = 0;
    reachedEnd = false;
    return SUCCEEDED(hr);
}

int SettlementVideoAudioDecoder::readFrames(std::vector<short>& outSamples, int maxFrames) {
    outSamples.clear();
    if (!isOpen() || maxFrames <= 0 || channels <= 0) return 0;

    const std::size_t wantedBytes =
        static_cast<std::size_t>(maxFrames) * static_cast<std::size_t>(channels) * sizeof(short);
    std::vector<unsigned char> outBytes;
    outBytes.reserve(wantedBytes);

    while (outBytes.size() < wantedBytes) {
        if (pendingOffset < pendingBytes.size()) {
            const std::size_t available = pendingBytes.size() - pendingOffset;
            const std::size_t take = std::min(available, wantedBytes - outBytes.size());
            outBytes.insert(outBytes.end(),
                            pendingBytes.begin() + static_cast<std::ptrdiff_t>(pendingOffset),
                            pendingBytes.begin() + static_cast<std::ptrdiff_t>(pendingOffset + take));
            pendingOffset += take;
            if (pendingOffset >= pendingBytes.size()) {
                pendingBytes.clear();
                pendingOffset = 0;
            }
            continue;
        }

        if (reachedEnd) break;

        DWORD streamIndex = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        Microsoft::WRL::ComPtr<IMFSample> sample;
        HRESULT hr = impl->reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                              0, &streamIndex, &flags, &timestamp, &sample);
        if (FAILED(hr)) {
            std::cerr << "[VideoAudio] ReadSample failed: 0x" << std::hex << hr << std::dec << "\n";
            reachedEnd = true;
            break;
        }
        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
            reachedEnd = true;
            break;
        }
        if (sample == nullptr) continue;

        Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
        hr = sample->ConvertToContiguousBuffer(&buffer);
        if (FAILED(hr)) continue;

        BYTE* data = nullptr;
        DWORD maxLength = 0;
        DWORD currentLength = 0;
        hr = buffer->Lock(&data, &maxLength, &currentLength);
        if (FAILED(hr) || data == nullptr || currentLength == 0) {
            if (SUCCEEDED(hr)) buffer->Unlock();
            continue;
        }

        pendingBytes.assign(data, data + currentLength);
        pendingOffset = 0;
        buffer->Unlock();
    }

    const std::size_t alignedBytes = outBytes.size() - (outBytes.size() % sizeof(short));
    outSamples.resize(alignedBytes / sizeof(short));
    if (alignedBytes > 0) {
        std::memcpy(outSamples.data(), outBytes.data(), alignedBytes);
    }

    return static_cast<int>(outSamples.size() / static_cast<std::size_t>(channels));
}

void SettlementVideoAudioDecoder::close() {
    delete impl;
    impl = nullptr;
    pendingBytes.clear();
    pendingOffset = 0;
    reachedEnd = false;
    sampleRate = 44100;
    channels = 2;
    if (mediaFoundationStarted) {
        MFShutdown();
        mediaFoundationStarted = false;
    }
}

bool SettlementVideoAudioDecoder::isOpen() const {
    return impl != nullptr && impl->reader != nullptr;
}

}  // namespace frontend

#else

namespace frontend {

struct SettlementVideoAudioDecoder::Impl {};

SettlementVideoAudioDecoder::~SettlementVideoAudioDecoder() = default;
bool SettlementVideoAudioDecoder::open(const std::string&, int&, int&) { return false; }
bool SettlementVideoAudioDecoder::restart() { return false; }
int SettlementVideoAudioDecoder::readFrames(std::vector<short>&, int) { return 0; }
void SettlementVideoAudioDecoder::close() {}
bool SettlementVideoAudioDecoder::isOpen() const { return false; }

}  // namespace frontend

#endif
