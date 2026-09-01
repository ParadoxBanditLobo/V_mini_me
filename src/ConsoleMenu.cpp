#include "ConsoleMenu.hpp"

#include "Direction.hpp"

#include <poll.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::string trim(std::string value) {
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string yesNo(bool value) {
    return value ? "ON" : "OFF";
}

void printTitle() {
    std::cout <<
        "\n"
        " __      __                 _       _                 \n"
        " \\ \\    / /                (_)     (_)                \n"
        "  \\ \\  / / _ __ ___  _ __  _ _ __  _   _ __ ___   ___\n"
        "   \\ \\/ / | '_ ` _ \\| '_ \\| | '_ \\| | | '_ ` _ \\ / _ \\\n"
        "    \\  /  | | | | | | | | | | | | | | | | | | | | |  __/\n"
        "     \\/   |_| |_| |_|_| |_|_|_| |_|_| |_| |_| |_| |_|\\___|\n"
        "                       V_mini_me\n";
}

std::filesystem::path resolveAvatarPath(
    const std::filesystem::path& executableDirectory,
    const std::string& configuredPath) {

    std::filesystem::path path(configuredPath);
    return path.is_relative() ? executableDirectory / path : path;
}

bool validAvatarDirectory(
    const std::filesystem::path& executableDirectory,
    const std::string& configuredPath) {

    std::error_code error;
    const auto path = resolveAvatarPath(executableDirectory, configuredPath);
    return std::filesystem::is_directory(path, error) &&
           std::filesystem::is_regular_file(path / "center.png", error);
}

std::vector<std::string> discoverAvatarDirectories(const std::filesystem::path& executableDirectory) {
    std::vector<std::string> result;
    const auto root = executableDirectory / "avatar";
    std::error_code error;

    if (!std::filesystem::is_directory(root, error)) {
        return result;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root, error)) {
        if (error) break;
        if (!entry.is_directory(error)) continue;
        if (!std::filesystem::is_regular_file(entry.path() / "center.png", error)) continue;

        const auto relative = std::filesystem::relative(entry.path(), executableDirectory, error);
        if (!error) {
            result.push_back(relative.generic_string());
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

bool chooseAvatarPath(
    const std::filesystem::path& executableDirectory,
    std::string& target,
    bool allowClear) {

    const auto avatars = discoverAvatarDirectories(executableDirectory);

    std::cout << "\nAvatar folders containing center.png:\n";
    for (std::size_t i = 0; i < avatars.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << avatars[i] << '\n';
    }
    std::cout << "  C. Enter a custom path\n";
    if (allowClear) std::cout << "  X. Clear this slot\n";
    std::cout << "  B. Back\n> ";

    std::string input;
    if (!std::getline(std::cin, input)) return false;
    input = trim(input);
    const std::string lowered = lowerCopy(input);
    if (input.empty() || lowered == "b") return false;
    if (allowClear && lowered == "x") {
        target.clear();
        return true;
    }

    if (lowered == "c") {
        std::cout << "Path (relative paths are relative to the executable):\n> ";
        if (!std::getline(std::cin, input)) return false;
        input = trim(input);
        if (input.empty()) return false;
        if (!validAvatarDirectory(executableDirectory, input)) {
            std::cout << "That folder does not contain a readable center.png. No change made.\n";
            return false;
        }
        target = input;
        return true;
    }

    try {
        const std::size_t choice = static_cast<std::size_t>(std::stoul(input));
        if (choice >= 1 && choice <= avatars.size()) {
            target = avatars[choice - 1];
            return true;
        }
    } catch (...) {
    }

    std::cout << "Unknown selection. No change made.\n";
    return false;
}

void chooseAvatarDirectory(AppConfig& config, const std::filesystem::path& executableDirectory) {
    chooseAvatarPath(executableDirectory, config.avatarDirectory, false);
}

void configureQuickExpressions(AppConfig& config, const std::filesystem::path& executableDirectory) {
    while (true) {
        std::cout << "\nQUICK EXPRESSIONS\n\n";
        for (std::size_t i = 0; i < config.expressionDirectories.size(); ++i) {
            std::cout << "  " << (i + 1) << ". ";
            if (config.expressionDirectories[i].empty()) std::cout << "[empty]";
            else std::cout << config.expressionDirectories[i];
            std::cout << '\n';
        }
        std::cout << "\nSelect a slot to assign/change, or B to go back.\n> ";

        std::string input;
        if (!std::getline(std::cin, input)) return;
        input = lowerCopy(trim(input));
        if (input.empty() || input == "b" || input == "back") return;

        try {
            const std::size_t slot = static_cast<std::size_t>(std::stoul(input));
            if (slot >= 1 && slot <= config.expressionDirectories.size()) {
                chooseAvatarPath(executableDirectory, config.expressionDirectories[slot - 1], true);
            } else {
                std::cout << "Choose a slot from 1 to 8.\n";
            }
        } catch (...) {
            std::cout << "Unknown selection.\n";
        }
    }
}

void chooseScale(AppConfig& config) {
    std::cout << "\nCurrent scale: " << std::fixed << std::setprecision(2) << config.scale << "\n"
              << "Enter a scale from 0.10 to 8.00 (1.00 = original size):\n> ";

    std::string input;
    if (!std::getline(std::cin, input)) return;
    input = trim(input);
    if (input.empty()) return;

    try {
        const double value = std::stod(input);
        if (value < 0.10 || value > 8.00) {
            std::cout << "Scale must be between 0.10 and 8.00.\n";
            return;
        }
        config.scale = value;
    } catch (...) {
        std::cout << "Invalid scale. No change made.\n";
    }
}

void chooseDirectionMode(AppConfig& config) {
    std::cout << "\nDirection mode:\n"
              << "  1. horizontal  (left / center / right)\n"
              << "  2. fourway    (cardinal directions + center)\n"
              << "  3. quadrants  (four diagonals + center)\n"
              << "  4. eightway   (all eight directions + center)\n"
              << "> ";

    std::string input;
    if (!std::getline(std::cin, input)) return;
    input = trim(input);

    if (input == "1") config.directionMode = DirectionMode::Horizontal;
    else if (input == "2") config.directionMode = DirectionMode::FourWay;
    else if (input == "3") config.directionMode = DirectionMode::Quadrants;
    else if (input == "4") config.directionMode = DirectionMode::EightWay;
    else std::cout << "Unknown selection. No change made.\n";
}

void printSetupScreen(const AppConfig& config, const std::filesystem::path& configPath) {
    printTitle();
    std::cout << "\n========================================\n"
              << "                 SETUP\n"
              << "========================================\n"
              << "  1. Avatar folder      " << config.avatarDirectory << '\n'
              << "  2. Scale              " << std::fixed << std::setprecision(2) << config.scale << '\n'
              << "  3. Direction mode     " << directionModeToName(config.directionMode) << '\n'
              << "  4. Microphone         " << yesNo(config.microphoneEnabled) << '\n'
              << "  5. Talking bounce     " << yesNo(config.talkingBounce) << '\n'
              << "  6. Idle bob           " << yesNo(config.idleBob) << '\n'
              << "  7. Idle sway          " << yesNo(config.idleSway) << '\n'
              << "  8. Quick expressions\n"
              << "  9. Setup on startup   " << yesNo(config.showSetupOnStart) << '\n'
              << "\n"
              << "  S. Save and start\n"
              << "  Q. Save and quit\n"
              << "\n"
              << "Advanced values remain editable in:\n"
              << "  " << configPath.string() << "\n"
              << "> ";
}
}

SetupMenuResult runSetupMenu(
    AppConfig& config,
    const std::filesystem::path& executableDirectory,
    const std::filesystem::path& configPath) {

    while (true) {
        printSetupScreen(config, configPath);

        std::string input;
        if (!std::getline(std::cin, input)) {
            return SetupMenuResult::Start;
        }

        input = lowerCopy(trim(input));
        if (input == "1") {
            chooseAvatarDirectory(config, executableDirectory);
        } else if (input == "2") {
            chooseScale(config);
        } else if (input == "3") {
            chooseDirectionMode(config);
        } else if (input == "4") {
            config.microphoneEnabled = !config.microphoneEnabled;
        } else if (input == "5") {
            config.talkingBounce = !config.talkingBounce;
        } else if (input == "6") {
            config.idleBob = !config.idleBob;
        } else if (input == "7") {
            config.idleSway = !config.idleSway;
        } else if (input == "8") {
            configureQuickExpressions(config, executableDirectory);
        } else if (input == "9") {
            config.showSetupOnStart = !config.showSetupOnStart;
        } else if (input == "s" || input == "start") {
            saveConfig(config, configPath.string());
            return SetupMenuResult::Start;
        } else if (input == "q" || input == "quit" || input == "exit") {
            saveConfig(config, configPath.string());
            return SetupMenuResult::Quit;
        } else if (!input.empty()) {
            std::cout << "Unknown selection.\n";
        }
    }
}

bool chooseQuickExpression(const AppConfig& config, std::string& selectedDirectory) {
    bool any = false;
    std::cout << "\nQUICK EXPRESSIONS\n\n";
    for (std::size_t i = 0; i < config.expressionDirectories.size(); ++i) {
        if (config.expressionDirectories[i].empty()) continue;
        any = true;
        std::cout << "  " << (i + 1) << ". " << config.expressionDirectories[i] << '\n';
    }

    if (!any) {
        std::cout << "  No expression slots are configured.\n"
                  << "  Return to setup and choose Quick expressions to add some.\n\n";
        return false;
    }

    std::cout << "  B. Back\n\nExpression: ";
    std::string input;
    if (!std::getline(std::cin, input)) return false;
    input = lowerCopy(trim(input));
    if (input.empty() || input == "b" || input == "back") return false;

    try {
        const std::size_t slot = static_cast<std::size_t>(std::stoul(input));
        if (slot >= 1 && slot <= config.expressionDirectories.size() &&
            !config.expressionDirectories[slot - 1].empty()) {
            selectedDirectory = config.expressionDirectories[slot - 1];
            std::cout << "Switched to: " << selectedDirectory << "\n> " << std::flush;
            return true;
        }
    } catch (...) {
    }

    std::cout << "That expression slot is not configured.\n> " << std::flush;
    return false;
}

void printRuntimeHelp() {
    std::cout << "\nV_mini_me is running.\n"
              << "  Left-drag avatar : move it\n"
              << "  Right-click      : return to setup\n"
              << "\nTerminal commands:\n"
              << "  E / expressions  quick expression menu\n"
              << "  S / setup        return to setup menu\n"
              << "  R / reload       reload config.ini and avatar files\n"
              << "  H / help         show this list\n"
              << "  Q / quit         exit\n"
              << "> " << std::flush;
}

RuntimeCommand waitForRuntimeCommand(int timeoutMilliseconds) {
    static bool stdinAvailable = true;

    if (!stdinAvailable) {
        ::poll(nullptr, 0, timeoutMilliseconds);
        return RuntimeCommand::NoCommand;
    }

    bool lineMayBeBuffered = std::cin.rdbuf()->in_avail() > 0;
    if (!lineMayBeBuffered) {
        pollfd descriptor{};
        descriptor.fd = STDIN_FILENO;
        descriptor.events = POLLIN;

        const int result = ::poll(&descriptor, 1, timeoutMilliseconds);
        if (result <= 0) {
            return RuntimeCommand::NoCommand;
        }

        if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 &&
            (descriptor.revents & POLLIN) == 0) {
            stdinAvailable = false;
            return RuntimeCommand::NoCommand;
        }

        if ((descriptor.revents & POLLIN) == 0) {
            return RuntimeCommand::NoCommand;
        }
    }

    std::string input;
    if (!std::getline(std::cin, input)) {
        stdinAvailable = false;
        return RuntimeCommand::NoCommand;
    }

    input = lowerCopy(trim(input));
    if (input == "e" || input == "expressions" || input == "expression") {
        return RuntimeCommand::Expressions;
    }
    if (input == "s" || input == "setup" || input == "menu") {
        return RuntimeCommand::Setup;
    }
    if (input == "r" || input == "reload") {
        return RuntimeCommand::Reload;
    }
    if (input == "q" || input == "quit" || input == "exit") {
        return RuntimeCommand::Quit;
    }
    if (input == "h" || input == "help" || input == "?") {
        printRuntimeHelp();
        return RuntimeCommand::NoCommand;
    }

    if (!input.empty()) {
        std::cout << "Unknown command. Type H for help.\n> " << std::flush;
    } else {
        std::cout << "> " << std::flush;
    }
    return RuntimeCommand::NoCommand;
}
