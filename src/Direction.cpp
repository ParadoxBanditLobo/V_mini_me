#include "Direction.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
}

DirectionMode parseDirectionMode(const std::string& value) {
    const std::string mode = lowerCopy(value);
    if (mode == "horizontal" || mode == "3") {
        return DirectionMode::Horizontal;
    }
    if (mode == "fourway" || mode == "4way" || mode == "4") {
        return DirectionMode::FourWay;
    }
    if (mode == "quadrants" || mode == "quadrant" || mode == "4quadrant") {
        return DirectionMode::Quadrants;
    }
    if (mode == "eightway" || mode == "8way" || mode == "8") {
        return DirectionMode::EightWay;
    }
    throw std::runtime_error("Unknown direction_mode: " + value);
}

std::string directionModeToName(DirectionMode mode) {
    switch (mode) {
        case DirectionMode::Horizontal: return "horizontal";
        case DirectionMode::FourWay:    return "fourway";
        case DirectionMode::Quadrants:  return "quadrants";
        case DirectionMode::EightWay:   return "eightway";
    }
    return "eightway";
}

std::string directionToName(Direction direction) {
    switch (direction) {
        case Direction::Center:    return "center";
        case Direction::Left:      return "left";
        case Direction::Right:     return "right";
        case Direction::Up:        return "up";
        case Direction::Down:      return "down";
        case Direction::UpLeft:    return "up_left";
        case Direction::UpRight:   return "up_right";
        case Direction::DownLeft:  return "down_left";
        case Direction::DownRight: return "down_right";
    }
    return "center";
}

Direction determineDirection(
    int mouseX,
    int mouseY,
    int screenWidth,
    int screenHeight,
    double deadzoneFraction,
    DirectionMode mode) {

    const double centerX = screenWidth / 2.0;
    const double centerY = screenHeight / 2.0;
    const double dx = mouseX - centerX;
    const double dy = mouseY - centerY;
    const double clampedDeadzone = std::clamp(deadzoneFraction, 0.0, 0.95);

    if (mode == DirectionMode::Horizontal) {
        const double horizontalDeadzone = (screenWidth / 2.0) * clampedDeadzone;
        if (std::abs(dx) <= horizontalDeadzone) {
            return Direction::Center;
        }
        return dx < 0.0 ? Direction::Left : Direction::Right;
    }

    const double radius = std::hypot(dx, dy);
    const double deadzoneRadius = (std::min(screenWidth, screenHeight) / 2.0) * clampedDeadzone;
    if (radius <= deadzoneRadius) {
        return Direction::Center;
    }

    if (mode == DirectionMode::FourWay) {
        if (std::abs(dx) >= std::abs(dy)) {
            return dx < 0.0 ? Direction::Left : Direction::Right;
        }
        return dy < 0.0 ? Direction::Up : Direction::Down;
    }

    if (mode == DirectionMode::Quadrants) {
        if (dx < 0.0 && dy < 0.0) return Direction::UpLeft;
        if (dx >= 0.0 && dy < 0.0) return Direction::UpRight;
        if (dx < 0.0 && dy >= 0.0) return Direction::DownLeft;
        return Direction::DownRight;
    }

    // Screen Y grows downward, so negate dy to make mathematical angles point up.
    constexpr double kPi = 3.14159265358979323846;
    double angle = std::atan2(-dy, dx) * 180.0 / kPi;
    if (angle < 0.0) {
        angle += 360.0;
    }

    if (angle < 22.5 || angle >= 337.5) return Direction::Right;
    if (angle < 67.5)  return Direction::UpRight;
    if (angle < 112.5) return Direction::Up;
    if (angle < 157.5) return Direction::UpLeft;
    if (angle < 202.5) return Direction::Left;
    if (angle < 247.5) return Direction::DownLeft;
    if (angle < 292.5) return Direction::Down;
    return Direction::DownRight;
}
