#pragma once

#include <string>

// Every visual pose the application can request from an avatar set.
enum class Direction {
    Center,
    Left,
    Right,
    Up,
    Down,
    UpLeft,
    UpRight,
    DownLeft,
    DownRight
};

// Determines how cursor position is converted into a Direction.
enum class DirectionMode {
    Horizontal, // center / left / right
    FourWay,    // center + dominant left/right/up/down axis
    Quadrants,  // center + four diagonal quadrants
    EightWay    // center + all eight compass directions
};

DirectionMode parseDirectionMode(const std::string& value);
std::string directionModeToName(DirectionMode mode);
std::string directionToName(Direction direction);

// Converts a global cursor position into one of the discrete avatar directions.
// deadzoneFraction is measured from screen center as a fraction of half the
// shortest screen dimension. Example: 0.15 means a 15% center dead zone.
Direction determineDirection(
    int mouseX,
    int mouseY,
    int screenWidth,
    int screenHeight,
    double deadzoneFraction,
    DirectionMode mode);
