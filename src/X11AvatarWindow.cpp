#include "X11AvatarWindow.hpp"

#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {
struct MotifWmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long inputMode;
    unsigned long status;
};

unsigned int maskShift(unsigned long mask) {
    unsigned int shift = 0;
    if (mask == 0) return 0;
    while ((mask & 1UL) == 0) {
        mask >>= 1;
        ++shift;
    }
    return shift;
}

unsigned int maskBits(unsigned long mask) {
    unsigned int bits = 0;
    while (mask) {
        bits += static_cast<unsigned int>(mask & 1UL);
        mask >>= 1;
    }
    return bits;
}

unsigned long scaleToMask(std::uint8_t value, unsigned long mask) {
    if (mask == 0) return 0;
    const unsigned int shift = maskShift(mask);
    const unsigned int bits = maskBits(mask);
    const unsigned long maxValue = (1UL << bits) - 1UL;
    return ((static_cast<unsigned long>(value) * maxValue + 127UL) / 255UL) << shift;
}
}

X11AvatarWindow::X11AvatarWindow(
    int width,
    int height,
    int x,
    int y,
    bool alwaysOnTop,
    bool draggable)
    : width_(width), height_(height), draggable_(draggable) {

    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        throw std::runtime_error("Could not open X11 display. Is DISPLAY set?");
    }

    screen_ = DefaultScreen(display_);
    root_ = RootWindow(display_, screen_);
    chooseVisual();

    XSetWindowAttributes attrs{};
    attrs.colormap = colormap_;
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;
    attrs.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
                       ButtonReleaseMask | PointerMotionMask;

    window_ = XCreateWindow(
        display_, root_, x, y,
        static_cast<unsigned int>(width_), static_cast<unsigned int>(height_),
        0, depth_, InputOutput, visual_,
        CWColormap | CWBackPixel | CWBorderPixel | CWEventMask, &attrs);

    if (!window_) {
        throw std::runtime_error("XCreateWindow failed");
    }

    XStoreName(display_, window_, "V_mini_me");
    removeDecorations();
    if (alwaysOnTop) {
        setAlwaysOnTop();
    }

    wmDelete_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display_, window_, &wmDelete_, 1);

    gc_ = XCreateGC(display_, window_, 0, nullptr);
    XMapWindow(display_, window_);
    XFlush(display_);
}

X11AvatarWindow::~X11AvatarWindow() {
    if (!display_) return;
    if (gc_) XFreeGC(display_, gc_);
    if (window_) XDestroyWindow(display_, window_);
    if (colormap_) XFreeColormap(display_, colormap_);
    XCloseDisplay(display_);
}

void X11AvatarWindow::chooseVisual() {
    XVisualInfo templateInfo{};
    templateInfo.screen = screen_;
    templateInfo.depth = 32;
    templateInfo.c_class = TrueColor;

    int count = 0;
    XVisualInfo* infos = XGetVisualInfo(
        display_, VisualScreenMask | VisualDepthMask | VisualClassMask,
        &templateInfo, &count);

    if (infos && count > 0) {
        visual_ = infos[0].visual;
        depth_ = infos[0].depth;
        hasAlphaVisual_ = true;
        XFree(infos);
    } else {
        if (infos) XFree(infos);
        visual_ = DefaultVisual(display_, screen_);
        depth_ = DefaultDepth(display_, screen_);
        hasAlphaVisual_ = false;
    }

    colormap_ = XCreateColormap(display_, root_, visual_, AllocNone);
}

void X11AvatarWindow::removeDecorations() {
    Atom property = XInternAtom(display_, "_MOTIF_WM_HINTS", False);
    MotifWmHints hints{};
    hints.flags = 1UL << 1;
    hints.decorations = 0;
    XChangeProperty(display_, window_, property, property, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(&hints), 5);
}

void X11AvatarWindow::setAlwaysOnTop() {
    Atom state = XInternAtom(display_, "_NET_WM_STATE", False);
    Atom above = XInternAtom(display_, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(display_, window_, state, XA_ATOM, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(&above), 1);
}

unsigned long X11AvatarWindow::packPixel(
    std::uint8_t r,
    std::uint8_t g,
    std::uint8_t b,
    std::uint8_t a) const {

    const std::uint8_t pr = static_cast<std::uint8_t>((static_cast<unsigned int>(r) * a + 127) / 255);
    const std::uint8_t pg = static_cast<std::uint8_t>((static_cast<unsigned int>(g) * a + 127) / 255);
    const std::uint8_t pb = static_cast<std::uint8_t>((static_cast<unsigned int>(b) * a + 127) / 255);

    unsigned long pixel = scaleToMask(pr, visual_->red_mask) |
                          scaleToMask(pg, visual_->green_mask) |
                          scaleToMask(pb, visual_->blue_mask);

    if (hasAlphaVisual_) {
        const unsigned long rgbMask = visual_->red_mask | visual_->green_mask | visual_->blue_mask;
        const unsigned long alphaMask = (~rgbMask) & 0xFFFFFFFFUL;
        pixel |= scaleToMask(a, alphaMask);
    }
    return pixel;
}

void X11AvatarWindow::draw(
    const PngImage& image,
    double scale,
    int verticalOffset,
    int horizontalOffset) {

    if (image.empty()) return;

    XImage* ximage = XCreateImage(
        display_, visual_, static_cast<unsigned int>(depth_), ZPixmap, 0, nullptr,
        static_cast<unsigned int>(width_), static_cast<unsigned int>(height_), 32, 0);
    if (!ximage) {
        throw std::runtime_error("XCreateImage failed");
    }

    const std::size_t bytes = static_cast<std::size_t>(ximage->bytes_per_line) * height_;
    ximage->data = static_cast<char*>(std::calloc(bytes, 1));
    if (!ximage->data) {
        XDestroyImage(ximage);
        throw std::bad_alloc();
    }

    const int outputWidth = std::max(1, static_cast<int>(std::lround(image.width * scale)));
    const int outputHeight = std::max(1, static_cast<int>(std::lround(image.height * scale)));
    const int startX = (width_ - outputWidth) / 2 + horizontalOffset;
    const int startY = (height_ - outputHeight) / 2 - verticalOffset;

    for (int y = 0; y < outputHeight; ++y) {
        const int dstY = startY + y;
        if (dstY < 0 || dstY >= height_) continue;
        const int srcY = std::clamp(static_cast<int>(y / scale), 0, image.height - 1);

        for (int x = 0; x < outputWidth; ++x) {
            const int dstX = startX + x;
            if (dstX < 0 || dstX >= width_) continue;
            const int srcX = std::clamp(static_cast<int>(x / scale), 0, image.width - 1);

            const std::size_t src = (static_cast<std::size_t>(srcY) * image.width + srcX) * 4;
            XPutPixel(ximage, dstX, dstY, packPixel(
                image.rgba[src], image.rgba[src + 1], image.rgba[src + 2], image.rgba[src + 3]));
        }
    }

    XPutImage(display_, window_, gc_, ximage, 0, 0, 0, 0,
              static_cast<unsigned int>(width_), static_cast<unsigned int>(height_));
    XFlush(display_);
    XDestroyImage(ximage);
}

bool X11AvatarWindow::processEvents() {
    while (XPending(display_) > 0) {
        XEvent event{};
        XNextEvent(display_, &event);

        if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == wmDelete_) {
            return false;
        }

        if (event.type == ButtonPress && event.xbutton.button == Button3) {
            setupRequested_ = true;
        }

        if (draggable_ && event.type == ButtonPress && event.xbutton.button == Button1) {
            dragging_ = true;
            dragOffsetX_ = event.xbutton.x;
            dragOffsetY_ = event.xbutton.y;
        }
        if (draggable_ && event.type == ButtonRelease && event.xbutton.button == Button1) {
            dragging_ = false;
        }
        if (draggable_ && dragging_ && event.type == MotionNotify) {
            XMoveWindow(display_, window_, event.xmotion.x_root - dragOffsetX_, event.xmotion.y_root - dragOffsetY_);
        }
    }
    return true;
}

bool X11AvatarWindow::queryPointer(int& x, int& y) const {
    Window rootReturn = 0;
    Window childReturn = 0;
    int winX = 0;
    int winY = 0;
    unsigned int mask = 0;
    return XQueryPointer(display_, root_, &rootReturn, &childReturn, &x, &y, &winX, &winY, &mask) != 0;
}

int X11AvatarWindow::screenWidth() const {
    return DisplayWidth(display_, screen_);
}

int X11AvatarWindow::screenHeight() const {
    return DisplayHeight(display_, screen_);
}

bool X11AvatarWindow::takeSetupRequest() {
    const bool requested = setupRequested_;
    setupRequested_ = false;
    return requested;
}
