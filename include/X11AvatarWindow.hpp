#pragma once

#include "PngImage.hpp"

#include <X11/Xlib.h>

// Small X11 window responsible only for native window behavior and drawing.
// A 32-bit ARGB visual is used when available, allowing transparent PNG areas
// to be composited by the desktop compositor.
class X11AvatarWindow {
public:
    X11AvatarWindow(int width, int height, int x, int y, bool alwaysOnTop, bool draggable);
    ~X11AvatarWindow();

    X11AvatarWindow(const X11AvatarWindow&) = delete;
    X11AvatarWindow& operator=(const X11AvatarWindow&) = delete;

    bool processEvents();
    // Draws an image centered in the transparent window. A positive vertical
    // offset moves the avatar upward and is used for the talking bounce.
    void draw(const PngImage& image, double scale, int verticalOffset = 0);
    bool queryPointer(int& x, int& y) const;

    // Right-click requests the text setup menu instead of terminating the app.
    // The main loop consumes this flag and rebuilds the avatar after settings change.
    bool takeSetupRequest();

    int screenWidth() const;
    int screenHeight() const;
    bool hasAlphaVisual() const { return hasAlphaVisual_; }

private:
    void chooseVisual();
    void removeDecorations();
    void setAlwaysOnTop();
    unsigned long packPixel(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) const;

    Display* display_ = nullptr;
    int screen_ = 0;
    Window root_ = 0;
    Window window_ = 0;
    Visual* visual_ = nullptr;
    int depth_ = 0;
    Colormap colormap_ = 0;
    GC gc_ = 0;
    Atom wmDelete_ = 0;

    int width_ = 0;
    int height_ = 0;
    bool hasAlphaVisual_ = false;
    bool draggable_ = true;
    bool dragging_ = false;
    int dragOffsetX_ = 0;
    int dragOffsetY_ = 0;
    bool setupRequested_ = false;
};
