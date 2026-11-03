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

        const fs::path shoutingPath = base / (name + "_shouting.png");
        if (fs::exists(shoutingPath)) {
            PngImage image = loadPng(shoutingPath.string());
            maxWidth_ = std::max(maxWidth_, image.width);
            maxHeight_ = std::max(maxHeight_, image.height);
            shoutingImages_.emplace(direction, std::move(image));
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

const PngImage& AvatarSet::imageFor(Direction direction, int speechState) const {
    if (speechState >= 2) {
        const auto image = shoutingImages_.find(direction);
        if (image != shoutingImages_.end()) return image->second;
    }
    if (speechState >= 1) {
        const auto image = talkingImages_.find(direction);
        if (image != talkingImages_.end()) return image->second;
    }
    const auto idle = idleImages_.find(direction);
    if (idle != idleImages_.end()) return idle->second;
    if (speechState >= 2) {
        const auto center = shoutingImages_.find(Direction::Center);
        if (center != shoutingImages_.end()) return center->second;
    }
    if (speechState >= 1) {
        const auto center = talkingImages_.find(Direction::Center);
        if (center != talkingImages_.end()) return center->second;
    }
    return idleImages_.at(Direction::Center);
}
