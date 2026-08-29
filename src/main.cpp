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

// Resolves paths relative to the executable, not the shell's current directory.
// This makes the release folder portable: users can run the binary from anywhere.
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

// Runs one avatar session. Returning Setup/Reload deliberately destroys the
// current X11 window and microphone monitor, then main() rebuilds them cleanly
// from the updated configuration. This is simpler and less error-prone than
// mutating every subsystem in place.
SessionResult runAvatarSession(
    const AppConfig& config,
    const std::filesystem::path& base) {

    AvatarSet avatar(resolvedAvatarDirectory(base, config).string());

    // Reserve equal transparent padding above and below the avatar so the
    // talking bounce can move upward without clipping the image.
    const int bounceMargin = config.talkingBounce ? config.bouncePixels : 0;
    const int windowWidth = std::max(1, static_cast<int>(std::ceil(avatar.maxWidth() * config.scale)));
    const int windowHeight = std::max(
        1,
        static_cast<int>(std::ceil(avatar.maxHeight() * config.scale)) + bounceMargin * 2);

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

    // Microphone support is optional. If the system lacks a PulseAudio-
    // compatible recording service, direction tracking still works normally.
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
    int currentBounce = 0;

    window.draw(avatar.imageFor(currentDirection, currentTalking), config.scale, currentBounce);
    printRuntimeHelp();

    const auto directionInterval = std::chrono::milliseconds(config.updateMilliseconds);
    const auto microphoneRelease = std::chrono::milliseconds(config.microphoneReleaseMilliseconds);
    const auto bounceInterval = std::chrono::milliseconds(config.bounceIntervalMilliseconds);

    auto nextDirectionUpdate = std::chrono::steady_clock::now();
    auto lastVoiceActivity = std::chrono::steady_clock::time_point::min();
    auto talkingStarted = std::chrono::steady_clock::now();

    while (window.processEvents()) {
        if (window.takeSetupRequest()) {
            std::cout << "\nReturning to setup...\n";
            return SessionResult::Setup;
        }

        const auto now = std::chrono::steady_clock::now();
        Direction nextDirection = currentDirection;
        bool nextTalking = currentTalking;
        int nextBounce = currentBounce;

        // Mouse direction can be intentionally polled slowly for a relaxed
        // visual style. This setting is independent of microphone response.
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

        // Speech detection is deliberately only a volume threshold. The
        // release timer prevents tiny gaps between words from flickering
        // rapidly between idle and talking images.
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

        // Bounce alternates between the normal position and a small upward
        // offset while talking. The image itself is never transformed.
        if (nextTalking && config.talkingBounce && config.bouncePixels > 0) {
            const auto elapsed = now - talkingStarted;
            const auto phase = elapsed / bounceInterval;
            nextBounce = (phase % 2 == 0) ? config.bouncePixels : 0;
        } else {
            nextBounce = 0;
        }

        // Redraw only when a visible state changes. During silence and while
        // the mouse remains in one region, the window generates no draw work.
        if (nextDirection != currentDirection ||
            nextTalking != currentTalking ||
            nextBounce != currentBounce) {
            currentDirection = nextDirection;
            currentTalking = nextTalking;
            currentBounce = nextBounce;
            window.draw(
                avatar.imageFor(currentDirection, currentTalking),
                config.scale,
                currentBounce);
        }

        // Waiting on the terminal replaces the old fixed sleep. poll(2) blocks
        // for up to 5 ms, so this remains effectively idle when nothing happens.
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

            // Reload from disk so manual config.ini edits made while the avatar
            // is running take effect with the R command as well.
            config = loadConfig(options.configPath.string());
            showSetup = (result == SessionResult::Setup);
        }
    } catch (const std::exception& error) {
        std::cerr << "V_mini_me: " << error.what() << '\n';
        return 1;
    }
}
