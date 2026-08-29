#pragma once

#include "Direction.hpp"
#include "PngImage.hpp"

#include <map>
#include <string>

// Loads the avatar's PNG files once and keeps them cached in memory.
// center.png is required. Directional images and *_talking.png variants are
// optional; sensible fallbacks keep partial avatar packs usable.
class AvatarSet {
public:
    explicit AvatarSet(const std::string& directory);

    // Returns the best image for the requested direction/talking combination.
    // Fallback order while talking is:
    //   exact talking image -> exact idle image -> center_talking -> center.
    const PngImage& imageFor(Direction direction, bool talking = false) const;

    int maxWidth() const { return maxWidth_; }
    int maxHeight() const { return maxHeight_; }

private:
    std::map<Direction, PngImage> idleImages_;
    std::map<Direction, PngImage> talkingImages_;
    int maxWidth_ = 0;
    int maxHeight_ = 0;
};
