#pragma once

#include "AppConfig.hpp"

#include <filesystem>
#include <string>

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
    Expressions,
    Quit
};

// Runs the small text-only configuration menu. Settings are saved to
// configPath before Start/Quit is returned. No GUI toolkit is involved.
SetupMenuResult runSetupMenu(
    AppConfig& config,
    const std::filesystem::path& executableDirectory,
    const std::filesystem::path& configPath);

// Opens the nested Quick Expressions menu. Returns true and writes the selected
// folder when a configured slot is chosen; false means back/no selection.
bool chooseQuickExpression(const AppConfig& config, std::string& selectedDirectory);

// Prints the compact command list that remains available while the avatar runs.
void printRuntimeHelp();

// Waits up to timeoutMilliseconds for one complete terminal command.
RuntimeCommand waitForRuntimeCommand(int timeoutMilliseconds);
