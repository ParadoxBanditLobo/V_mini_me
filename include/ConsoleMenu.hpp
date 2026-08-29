#pragma once

#include "AppConfig.hpp"

#include <filesystem>

// Result returned by the interactive setup screen shown in the terminal.
enum class SetupMenuResult {
    Start,
    Quit
};

// Commands accepted while the avatar window is running.
enum class RuntimeCommand {
    NoCommand,
    Setup,
    Reload,
    Quit
};

// Runs the small text-only configuration menu. Settings are saved to
// configPath before Start/Quit is returned. No GUI toolkit is involved.
SetupMenuResult runSetupMenu(
    AppConfig& config,
    const std::filesystem::path& executableDirectory,
    const std::filesystem::path& configPath);

// Prints the compact command list that remains available while the avatar runs.
void printRuntimeHelp();

// Waits up to timeoutMilliseconds for one complete terminal command.
// On Linux this uses poll(2), replacing the application's old sleep call rather
// than adding another busy loop or console thread.
RuntimeCommand waitForRuntimeCommand(int timeoutMilliseconds);
