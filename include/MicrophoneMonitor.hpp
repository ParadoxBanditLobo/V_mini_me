#pragma once

#include <memory>
#include <string>

// Lightweight microphone level monitor.
//
// The implementation dynamically opens libpulse-simple at runtime instead of
// linking against it. This keeps microphone support optional: if PulseAudio or
// PipeWire's PulseAudio compatibility layer is unavailable, the rest of the
// application continues to work normally.
class MicrophoneMonitor {
public:
    // An empty device name selects the desktop's default recording source.
    explicit MicrophoneMonitor(const std::string& device = "");
    ~MicrophoneMonitor();

    MicrophoneMonitor(const MicrophoneMonitor&) = delete;
    MicrophoneMonitor& operator=(const MicrophoneMonitor&) = delete;

    // True when a recording stream was opened successfully and is still usable.
    bool available() const noexcept;

    // Most recent normalized RMS level, approximately 0.0 (silence) to 1.0.
    float level() const noexcept;

    // Human-readable reason microphone capture could not be started.
    const std::string& errorMessage() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
