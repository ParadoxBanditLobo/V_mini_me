#include "MicrophoneMonitor.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <dlfcn.h>
#include <stdexcept>
#include <thread>

namespace {
// Minimal declarations for the stable libpulse-simple C ABI. Declaring only
// the tiny subset we use lets this project compile without PulseAudio headers.
// The library itself is discovered dynamically at runtime.
struct pa_simple;

enum pa_stream_direction_t {
    PA_STREAM_NODIRECTION = 0,
    PA_STREAM_PLAYBACK = 1,
    PA_STREAM_RECORD = 2,
    PA_STREAM_UPLOAD = 3
};

enum pa_sample_format_t {
    PA_SAMPLE_U8 = 0,
    PA_SAMPLE_ALAW = 1,
    PA_SAMPLE_ULAW = 2,
    PA_SAMPLE_S16LE = 3
};

struct pa_sample_spec {
    pa_sample_format_t format;
    std::uint32_t rate;
    std::uint8_t channels;
};

template <typename T>
T loadSymbol(void* library, const char* name) {
    dlerror();
    void* symbol = dlsym(library, name);
    if (const char* error = dlerror()) {
        throw std::runtime_error(std::string("Missing PulseAudio symbol '") + name + "': " + error);
    }
    return reinterpret_cast<T>(symbol);
}
}

struct MicrophoneMonitor::Impl {
    using SimpleNewFn = pa_simple* (*)(
        const char*, const char*, pa_stream_direction_t, const char*, const char*,
        const pa_sample_spec*, const void*, const void*, int*);
    using SimpleReadFn = int (*)(pa_simple*, void*, std::size_t, int*);
    using SimpleFreeFn = void (*)(pa_simple*);
    using StrErrorFn = const char* (*)(int);

    void* library = nullptr;
    pa_simple* stream = nullptr;
    SimpleReadFn simpleRead = nullptr;
    SimpleFreeFn simpleFree = nullptr;
    StrErrorFn strError = nullptr;

    std::thread worker;
    std::atomic<bool> stop{false};
    std::atomic<bool> isAvailable{false};
    std::atomic<float> currentLevel{0.0F};
    std::string error;

    explicit Impl(const std::string& device) {
        try {
            library = dlopen("libpulse-simple.so.0", RTLD_NOW | RTLD_LOCAL);
            if (!library) {
                const char* message = dlerror();
                error = message ? message : "libpulse-simple.so.0 is unavailable";
                return;
            }

            const auto simpleNew = loadSymbol<SimpleNewFn>(library, "pa_simple_new");
            simpleRead = loadSymbol<SimpleReadFn>(library, "pa_simple_read");
            simpleFree = loadSymbol<SimpleFreeFn>(library, "pa_simple_free");
            strError = loadSymbol<StrErrorFn>(library, "pa_strerror");

            // 16 kHz mono is ample for voice activity detection and keeps the
            // amount of audio data processed by this utility very small.
            const pa_sample_spec sampleSpec{PA_SAMPLE_S16LE, 16000U, 1U};
            int pulseError = 0;
            stream = simpleNew(
                nullptr,
                "V_mini_me",
                PA_STREAM_RECORD,
                device.empty() ? nullptr : device.c_str(),
                "Microphone level",
                &sampleSpec,
                nullptr,
                nullptr,
                &pulseError);

            if (!stream) {
                error = strError ? strError(pulseError) : "Could not open the recording source";
                return;
            }

            isAvailable.store(true, std::memory_order_release);
            worker = std::thread([this] { captureLoop(); });
        } catch (const std::exception& exception) {
            error = exception.what();
        }
    }

    ~Impl() {
        stop.store(true, std::memory_order_release);
        if (worker.joinable()) {
            worker.join();
        }
        if (stream && simpleFree) {
            simpleFree(stream);
        }
        if (library) {
            dlclose(library);
        }
    }

    void captureLoop() {
        // 320 samples at 16 kHz is 20 ms of audio. This is responsive enough
        // for talking detection without creating a high-rate processing loop.
        std::array<std::int16_t, 320> samples{};

        while (!stop.load(std::memory_order_acquire)) {
            int pulseError = 0;
            if (simpleRead(stream, samples.data(), samples.size() * sizeof(samples[0]), &pulseError) < 0) {
                // A stream that fails after startup is simply marked unavailable.
                // The main application will return to the idle state gracefully.
                currentLevel.store(0.0F, std::memory_order_release);
                isAvailable.store(false, std::memory_order_release);
                return;
            }

            double sumSquares = 0.0;
            for (std::int16_t sample : samples) {
                const double normalized = static_cast<double>(sample) / 32768.0;
                sumSquares += normalized * normalized;
            }

            const float rms = static_cast<float>(std::sqrt(sumSquares / samples.size()));
            currentLevel.store(rms, std::memory_order_release);
        }
    }
};

MicrophoneMonitor::MicrophoneMonitor(const std::string& device)
    : impl_(std::make_unique<Impl>(device)) {}

MicrophoneMonitor::~MicrophoneMonitor() = default;

bool MicrophoneMonitor::available() const noexcept {
    return impl_->isAvailable.load(std::memory_order_acquire);
}

float MicrophoneMonitor::level() const noexcept {
    return impl_->currentLevel.load(std::memory_order_acquire);
}

const std::string& MicrophoneMonitor::errorMessage() const noexcept {
    return impl_->error;
}
