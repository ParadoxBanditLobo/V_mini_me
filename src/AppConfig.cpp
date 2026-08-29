#include "AppConfig.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace {
std::string trim(std::string value) {
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

bool parseBool(const std::string& value) {
    std::string lowered = value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
}
}

AppConfig loadConfig(const std::string& path) {
    AppConfig config;
    std::ifstream file(path);

    // Missing config is not fatal; built-in defaults make the binary usable.
    if (!file) {
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') {
            continue;
        }

        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, equals));
        const std::string value = trim(line.substr(equals + 1));

        try {
            if (key == "avatar_dir") config.avatarDirectory = value;
            else if (key == "direction_mode") config.directionMode = parseDirectionMode(value);
            else if (key == "deadzone") config.deadzone = std::stod(value);
            else if (key == "update_ms") config.updateMilliseconds = std::max(10, std::stoi(value));
            else if (key == "scale") config.scale = std::clamp(std::stod(value), 0.1, 8.0);
            else if (key == "window_x") config.windowX = std::stoi(value);
            else if (key == "window_y") config.windowY = std::stoi(value);
            else if (key == "always_on_top") config.alwaysOnTop = parseBool(value);
            else if (key == "draggable") config.draggable = parseBool(value);
            else if (key == "show_setup_on_start") config.showSetupOnStart = parseBool(value);
            else if (key == "mic_enabled") config.microphoneEnabled = parseBool(value);
            else if (key == "mic_device") config.microphoneDevice = value;
            else if (key == "mic_threshold") config.microphoneThreshold = std::stod(value);
            else if (key == "mic_release_ms") config.microphoneReleaseMilliseconds = std::clamp(std::stoi(value), 0, 5000);
            else if (key == "talk_bounce") config.talkingBounce = parseBool(value);
            else if (key == "bounce_pixels") config.bouncePixels = std::clamp(std::stoi(value), 0, 200);
            else if (key == "bounce_interval_ms") config.bounceIntervalMilliseconds = std::clamp(std::stoi(value), 50, 5000);
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid value for config key '" + key + "': " + value);
        }
    }

    config.deadzone = std::clamp(config.deadzone, 0.0, 0.95);
    config.microphoneThreshold = std::clamp(config.microphoneThreshold, 0.0, 1.0);
    return config;
}

void saveConfig(const AppConfig& config, const std::string& path) {
    const std::filesystem::path configPath(path);
    if (!configPath.parent_path().empty()) {
        std::error_code error;
        std::filesystem::create_directories(configPath.parent_path(), error);
    }

    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Could not write configuration file: " + path);
    }

    file << "# V_mini_me configuration\n"
         << "# Paths are relative to the executable unless absolute.\n\n"
         << "avatar_dir=" << config.avatarDirectory << "\n\n"
         << "# horizontal / fourway / quadrants / eightway\n"
         << "direction_mode=" << directionModeToName(config.directionMode) << "\n"
         << "deadzone=" << std::fixed << std::setprecision(4) << config.deadzone << "\n"
         << "update_ms=" << config.updateMilliseconds << "\n"
         << "scale=" << std::setprecision(4) << config.scale << "\n\n"
         << "window_x=" << config.windowX << "\n"
         << "window_y=" << config.windowY << "\n"
         << "always_on_top=" << (config.alwaysOnTop ? "true" : "false") << "\n"
         << "draggable=" << (config.draggable ? "true" : "false") << "\n"
         << "show_setup_on_start=" << (config.showSetupOnStart ? "true" : "false") << "\n\n"
         << "# --- Microphone reaction ---\n"
         << "mic_enabled=" << (config.microphoneEnabled ? "true" : "false") << "\n"
         << "mic_device=" << config.microphoneDevice << "\n"
         << "mic_threshold=" << std::setprecision(4) << config.microphoneThreshold << "\n"
         << "mic_release_ms=" << config.microphoneReleaseMilliseconds << "\n\n"
         << "talk_bounce=" << (config.talkingBounce ? "true" : "false") << "\n"
         << "bounce_pixels=" << config.bouncePixels << "\n"
         << "bounce_interval_ms=" << config.bounceIntervalMilliseconds << "\n";
}
