#pragma once

#include "Direction.hpp"

#include <string>

// User-editable settings loaded from config.ini at startup.
struct AppConfig {
    std::string avatarDirectory = "avatar/default";
    DirectionMode directionMode = DirectionMode::EightWay;
    double deadzone = 0.15;
    int updateMilliseconds = 100;
    double scale = 1.0;
    int windowX = 40;
    int windowY = 40;
    bool alwaysOnTop = true;
    bool draggable = true;

    // When true, the lightweight terminal setup screen is shown before the
    // avatar starts. The --setup command-line option can force it at any time.
    bool showSetupOnStart = true;

    // Microphone reaction is deliberately simple: normalized RMS volume above
    // micThreshold enters the talking state, then micReleaseMilliseconds keeps
    // the state alive through short pauses between words.
    bool microphoneEnabled = true;
    std::string microphoneDevice;
    double microphoneThreshold = 0.035;
    int microphoneReleaseMilliseconds = 250;

    // Talking bounce only changes the draw position; no physics or animation
    // engine is involved. A value of 0 disables visible movement.
    bool talkingBounce = true;
    int bouncePixels = 4;
    int bounceIntervalMilliseconds = 150;

    // Idle movement is positional only. Both effects are off by default so
    // existing configurations preserve their previous appearance.
    bool idleBob = false;
    int idleBobPixels = 3;
    int idleBobPeriodMilliseconds = 2400;
    bool idleSway = false;
    int idleSwayPixels = 2;
    int idleSwayPeriodMilliseconds = 3000;
};

// Reads a deliberately small key=value INI-style file. Unknown keys are
// ignored so future versions can add settings without breaking older builds.
AppConfig loadConfig(const std::string& path);

// Writes a canonical config.ini. The terminal setup menu uses this function so
// GUI libraries are unnecessary and the same file remains hand-editable.
void saveConfig(const AppConfig& config, const std::string& path);
