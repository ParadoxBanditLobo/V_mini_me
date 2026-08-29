#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Decoded 8-bit RGBA image kept in memory for instant state changes.
struct PngImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;

    bool empty() const { return width <= 0 || height <= 0 || rgba.empty(); }
};

// Loads a PNG and normalizes it to 8-bit RGBA regardless of source format.
PngImage loadPng(const std::string& path);
