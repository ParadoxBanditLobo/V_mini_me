#include "AvatarSet.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace {
constexpr Direction kDirections[] = {
    Direction::Center, Direction::Left, Direction::Right, Direction::Up, Direction::Down,
    Direction::UpLeft, Direction::UpRight, Direction::DownLeft, Direction::DownRight
};
}

AvatarSet::AvatarSet(const std::string& directory) {
    namespace fs = std::filesystem;

    const fs::path base(directory);
    const fs::path centerPath = base / "center.png";
    if (!fs::exists(centerPath)) {
        throw std::runtime_error("Avatar is missing required file: " + centerPath.string());
    }

    // Load every existing idle and talking image once at startup. State changes
    // then require no disk access, which keeps interaction instant and cheap.
    for (Direction direction : kDirections) {
        const std::string name = directionToName(direction);

        const fs::path idlePath = base / (name + ".png");
        if (fs::exists(idlePath)) {
            PngImage image = loadPng(idlePath.string());
            maxWidth_ = std::max(maxWidth_, image.width);
            maxHeight_ = std::max(maxHeight_, image.height);
            idleImages_.emplace(direction, std::move(image));
        }

        const fs::path talkingPath = base / (name + "_talking.png");
        if (fs::exists(talkingPath)) {
            PngImage image = loadPng(talkingPath.string());
            maxWidth_ = std::max(maxWidth_, image.width);
            maxHeight_ = std::max(maxHeight_, image.height);
            talkingImages_.emplace(direction, std::move(image));
        }
    }
}

const PngImage& AvatarSet::imageFor(Direction direction, bool talking) const {
    if (talking) {
        const auto exactTalking = talkingImages_.find(direction);
        if (exactTalking != talkingImages_.end()) {
            return exactTalking->second;
        }
    }

    // Preserve the current direction when a matching talking image is absent.
    const auto exactIdle = idleImages_.find(direction);
    if (exactIdle != idleImages_.end()) {
        return exactIdle->second;
    }

    // A pack containing only center_talking.png can still react to speech when
    // the requested directional idle image is also absent.
    if (talking) {
        const auto centerTalking = talkingImages_.find(Direction::Center);
        if (centerTalking != talkingImages_.end()) {
            return centerTalking->second;
        }
    }

    return idleImages_.at(Direction::Center);
}
