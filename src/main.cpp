#include "AppConfig.hpp"
#include "AvatarSet.hpp"
#include "ConsoleMenu.hpp"
#include "Direction.hpp"
#include "MicrophoneMonitor.hpp"
#include "X11AvatarWindow.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
enum class SessionResult {
    Quit,
    Setup,
    Reload
};

struct CommandLineOptions {
    std::filesystem::path configPath;
    bool forceSetup = false;
};

std::filesystem::path executableDirectory(const char* argv0) {
    std::error_code error;
    const std::filesystem::path procExe = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) {
        return procExe.parent_path();
    }
    return std::filesystem::absolute(argv0).parent_path();
}

std::filesystem::path resolvedAvatarDirectory(
    const std::filesystem::path& base,
    const AppConfig& config) {

    std::filesystem::path avatarPath(config.avatarDirectory);
    return avatarPath.is_relative() ? base / avatarPath : avatarPath;
}

CommandLineOptions parseCommandLine(
    int argc,
    char** argv,
    const std::filesystem::path& base) {

    CommandLineOptions options{base / "config.ini", false};

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--setup") {
            options.forceSetup = true;
        } else if (argument == "--help" || argument == "-h") {
            std::cout << "V_mini_me\n\n"
                      << "Usage: V_mini_me [config.ini] [--setup]\n\n"
                      << "  --setup   force the terminal setup menu before starting\n";
            std::exit(0);
        } else if (!argument.empty() && argument[0] == '-') {
            throw std::runtime_error("Unknown command-line option: " + argument);
        } else {
            options.configPath = argument;
        }
    }

    return options;
}

int sinusoidalOffset(
    std::chrono::steady_clock::time_point now,
    std::chrono::steady_clock::time_point started,
    bool enabled,
    int pixels,
    int periodMilliseconds) {

    if (!enabled || pixels <= 0 || periodMilliseconds <= 0) {
        return 0;
    }

    constexpr double tau = 6.28318530717958647692;
    const double elapsedMs = std::chrono::duration<double, std::milli>(now - started).count();
    const double phase = tau * elapsedMs / static_cast<double>(periodMilliseconds);
    return static_cast<int>(std::lround(std::sin(phase) * pixels));
}

SessionResult runAvatarSession(
    const AppConfig& config,
    const std::filesystem::path& base) {

    AvatarSet avatar(resolvedAvatarDirectory(base, config).string());

    const int talkingMargin = config.talkingBounce ? config.bouncePixels : 0;
    const int bobMargin = config.idleBob ? config.idleBobPixels : 0;
    const int swayMargin = config.idleSway ? config.idleSwayPixels : 0;
    const int verticalMargin = talkingMargin + bobMargin;

    const int windowWidth = std::max(
        1,
        static_cast<int>(std::ceil(avatar.maxWidth() * config.scale)) + swayMargin * 2);
    const int windowHeight = std::max(
        1,
        static_cast<int>(std::ceil(avatar.maxHeight() * config.scale)) + verticalMargin * 2);

    X11AvatarWindow window(
        windowWidth,
        windowHeight,
        config.windowX,
        config.windowY,
        config.alwaysOnTop,
        config.draggable);

    if (!window.hasAlphaVisual()) {
        std::cerr << "Warning: no 32-bit X11 visual found; transparent PNG areas may appear opaque.\n";
    }

    std::unique_ptr<MicrophoneMonitor> microphone;
    if (config.microphoneEnabled) {
        microphone = std::make_unique<MicrophoneMonitor>(config.microphoneDevice);
        if (!microphone->available()) {
            std::cerr << "Warning: microphone reaction disabled: "
                      << microphone->errorMessage() << '\n';
        }
    }

    Direction currentDirection = Direction::Center;
    bool currentTalking = false;
    int currentVerticalOffset = 0;
    int currentHorizontalOffset = 0;

    window.draw(
        avatar.imageFor(currentDirection, currentTalking),
        config.scale,
        currentVerticalOffset,
        currentHorizontalOffset);
    printRuntimeHelp();

    const auto directionInterval = std::chrono::milliseconds(config.updateMilliseconds);
    const auto microphoneRelease = std::chrono::milliseconds(config.microphoneReleaseMilliseconds);
    const auto bounceInterval = std::chrono::milliseconds(config.bounceIntervalMilliseconds);

    const auto sessionStarted = std::chrono::steady_clock::now();
    auto nextDirectionUpdate = sessionStarted;
    auto lastVoiceActivity = std::chrono::steady_clock::time_point::min();
    auto talkingStarted = sessionStarted;

    while (window.processEvents()) {
        if (window.takeSetupRequest()) {
            std::cout << "\nReturning to setup...\n";
            return SessionResult::Setup;
        }

        const auto now = std::chrono::steady_clock::now();
        Direction nextDirection = currentDirection;
        bool nextTalking = currentTalking;

        if (now >= nextDirectionUpdate) {
            int mouseX = 0;
            int mouseY = 0;
            if (window.queryPointer(mouseX, mouseY)) {
                nextDirection = determineDirection(
                    mouseX,
                    mouseY,
                    window.screenWidth(),
                    window.screenHeight(),
                    config.deadzone,
                    config.directionMode);
            }
            nextDirectionUpdate = now + directionInterval;
        }

        if (microphone && microphone->available()) {
            if (microphone->level() >= config.microphoneThreshold) {
                lastVoiceActivity = now;
                if (!nextTalking) {
                    nextTalking = true;
                    talkingStarted = now;
                }
            } else if (nextTalking && now - lastVoiceActivity >= microphoneRelease) {
                nextTalking = false;
            }
        } else {
            nextTalking = false;
        }

        int talkingOffset = 0;
        if (nextTalking && config.talkingBounce && config.bouncePixels > 0) {
            const auto elapsed = now - talkingStarted;
            const auto phase = elapsed / bounceInterval;
            talkingOffset = (phase % 2 == 0) ? config.bouncePixels : 0;
        }

        const int bobOffset = sinusoidalOffset(
            now,
            sessionStarted,
            config.idleBob,
            config.idleBobPixels,
            config.idleBobPeriodMilliseconds);
        const int swayOffset = sinusoidalOffset(
            now,
            sessionStarted,
            config.idleSway,
            config.idleSwayPixels,
            config.idleSwayPeriodMilliseconds);

        const int nextVerticalOffset = talkingOffset + bobOffset;
        const int nextHorizontalOffset = swayOffset;

        if (nextDirection != currentDirection ||
            nextTalking != currentTalking ||
            nextVerticalOffset != currentVerticalOffset ||
            nextHorizontalOffset != currentHorizontalOffset) {
            currentDirection = nextDirection;
            currentTalking = nextTalking;
            currentVerticalOffset = nextVerticalOffset;
            currentHorizontalOffset = nextHorizontalOffset;
            window.draw(
                avatar.imageFor(currentDirection, currentTalking),
                config.scale,
                currentVerticalOffset,
                currentHorizontalOffset);
        }

        switch (waitForRuntimeCommand(5)) {
            case RuntimeCommand::Setup:
                return SessionResult::Setup;
            case RuntimeCommand::Reload:
                return SessionResult::Reload;
            case RuntimeCommand::Quit:
                return SessionResult::Quit;
            case RuntimeCommand::NoCommand:
                break;
        }
    }

    return SessionResult::Quit;
}
}

int main(int argc, char** argv) {
    try {
        const std::filesystem::path base = executableDirectory(argv[0]);
        const CommandLineOptions options = parseCommandLine(argc, argv, base);

        AppConfig config = loadConfig(options.configPath.string());
        bool showSetup = options.forceSetup || config.showSetupOnStart;

        while (true) {
            if (showSetup) {
                if (runSetupMenu(config, base, options.configPath) == SetupMenuResult::Quit) {
                    return 0;
                }
            }

            const SessionResult result = runAvatarSession(config, base);
            if (result == SessionResult::Quit) {
                return 0;
            }

            config = loadConfig(options.configPath.string());
            showSetup = (result == SessionResult::Setup);
        }
    } catch (const std::exception& error) {
        std::cerr << "V_mini_me: " << error.what() << '\n';
        return 1;
    }
}
