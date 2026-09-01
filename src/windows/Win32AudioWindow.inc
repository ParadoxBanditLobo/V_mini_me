// -------------------------------------------------------------------------
// Microphone capture through the old, lightweight WinMM waveIn API.
// -------------------------------------------------------------------------

bool startMicrophone(MicrophoneMonitor& monitor, const AppConfig& config) {
    memset(&monitor, 0, sizeof(monitor));
    monitor.thresholdMicro = config.microphoneThresholdMicro;
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = 16000;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = 32000;
    format.cbSize = 0;
    UINT device = config.microphoneDevice < 0 ? WAVE_MAPPER : (UINT)config.microphoneDevice;
    if (waveInOpen(&monitor.handle, device, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) return false;

    constexpr int sampleCount = 256;
    for (int i = 0; i < 2; ++i) {
        monitor.samples[i] = (short*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sampleCount * sizeof(short));
        if (!monitor.samples[i]) return false;
        monitor.headers[i].lpData = (LPSTR)monitor.samples[i];
        monitor.headers[i].dwBufferLength = sampleCount * sizeof(short);
        if (waveInPrepareHeader(monitor.handle, &monitor.headers[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) return false;
        if (waveInAddBuffer(monitor.handle, &monitor.headers[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) return false;
    }
    if (waveInStart(monitor.handle) != MMSYSERR_NOERROR) return false;
    monitor.available = true;
    return true;
}

void stopMicrophone(MicrophoneMonitor& monitor) {
    if (monitor.handle) {
        waveInReset(monitor.handle);
        for (int i = 0; i < 2; ++i) {
            if (monitor.headers[i].lpData) waveInUnprepareHeader(monitor.handle, &monitor.headers[i], sizeof(WAVEHDR));
        }
        waveInClose(monitor.handle);
    }
    for (int i = 0; i < 2; ++i) {
        if (monitor.samples[i]) HeapFree(GetProcessHeap(), 0, monitor.samples[i]);
    }
    memset(&monitor, 0, sizeof(monitor));
}

bool microphoneLoud(MicrophoneMonitor& monitor) {
    if (!monitor.available) return false;
    bool loud = false;
    for (int index = 0; index < 2; ++index) {
        WAVEHDR& header = monitor.headers[index];
        if ((header.dwFlags & WHDR_DONE) == 0) continue;
        int sampleCount = (int)(header.dwBytesRecorded / sizeof(short));
        if (sampleCount > 0) {
            unsigned long long sumSquares = 0;
            for (int i = 0; i < sampleCount; ++i) {
                long long sample = monitor.samples[index][i];
                sumSquares += (unsigned long long)(sample * sample);
            }
            unsigned long long threshold = (unsigned long long)monitor.thresholdMicro * 32768ULL;
            threshold /= kThresholdBase;
            unsigned long long thresholdSquared = threshold * threshold;
            if (sumSquares / (unsigned int)sampleCount >= thresholdSquared) loud = true;
        }
        header.dwBytesRecorded = 0;
        header.dwFlags &= ~WHDR_DONE;
        waveInAddBuffer(monitor.handle, &header, sizeof(WAVEHDR));
    }
    return loud;
}

// -------------------------------------------------------------------------
// Transparent Win32 avatar window
// -------------------------------------------------------------------------

void renderWindow(WindowContext& context) {
    const ImageBitmap* image = imageFor(*context.avatar, context.direction, context.talking);
    if (!image || !image->loaded) return;
    SIZE_T canvasBytes = (SIZE_T)context.width * context.height * 4;
    memset(context.canvasBits, 0, canvasBytes);

    int startX = (context.width - image->width) / 2;
    int startY = context.bounceMargin + (context.avatar->maxHeight - image->height) / 2 - context.bounce;
    for (int y = 0; y < image->height; ++y) {
        int dstY = startY + y;
        if (dstY < 0 || dstY >= context.height) continue;
        int copyStartX = startX;
        int srcStartX = 0;
        int copyWidth = image->width;
        if (copyStartX < 0) { srcStartX = -copyStartX; copyWidth -= srcStartX; copyStartX = 0; }
        if (copyStartX + copyWidth > context.width) copyWidth = context.width - copyStartX;
        if (copyWidth <= 0) continue;
        BYTE* destination = context.canvasBits + ((SIZE_T)dstY * context.width + copyStartX) * 4;
        const BYTE* source = image->pixels + ((SIZE_T)y * image->width + srcStartX) * 4;
        memcpy(destination, source, (SIZE_T)copyWidth * 4);
    }

    POINT destination{(LONG)context.baseX, (LONG)context.baseY};
    SIZE size{(LONG)context.width, (LONG)context.height};
    POINT source{0, 0};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(context.hwnd, nullptr, &destination, &size, context.memoryDc, &source, 0, &blend, ULW_ALPHA);
}

LRESULT CALLBACK avatarWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    WindowContext* context = gWindowContext;
    if (!context) return DefWindowProcW(hwnd, message, wParam, lParam);

    if (message == WM_LBUTTONDOWN && context->draggable) {
        POINT point{};
        if (GetCursorPos(&point)) {
            context->dragging = true;
            context->dragMouseX = point.x;
            context->dragMouseY = point.y;
            context->dragBaseX = context->baseX;
            context->dragBaseY = context->baseY;
            SetCapture(hwnd);
        }
        return 0;
    }
    if (message == WM_MOUSEMOVE && context->dragging && context->draggable && (wParam & MK_LBUTTON)) {
        POINT point{};
        if (GetCursorPos(&point)) {
            context->baseX = context->dragBaseX + (point.x - context->dragMouseX);
            context->baseY = context->dragBaseY + (point.y - context->dragMouseY);
            renderWindow(*context);
        }
        return 0;
    }
    if (message == WM_LBUTTONUP && context->dragging) {
        context->dragging = false;
        ReleaseCapture();
        return 0;
    }
    if (message == WM_RBUTTONUP) {
        InterlockedExchange(&gRequestedAction, ACTION_SETUP);
        DestroyWindow(hwnd);
        return 0;
    }
    if (message == WM_CLOSE) {
        InterlockedExchange(&gRequestedAction, ACTION_QUIT);
        DestroyWindow(hwnd);
        return 0;
    }
    if (message == WM_DESTROY) {
        context->hwnd = nullptr;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool ensureWindowClass() {
    if (gWindowClassRegistered) return true;
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = avatarWindowProc;
    wc.hInstance = gInstance;
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)(ULONG_PTR)32512);
    wc.lpszClassName = L"V_mini_me_Window";
    if (!RegisterClassExW(&wc)) return false;
    gWindowClassRegistered = true;
    return true;
}

bool createAvatarWindow(WindowContext& context, AvatarSet& avatar, AppConfig& config) {
    memset(&context, 0, sizeof(context));
    if (!ensureWindowClass()) return false;
    context.avatar = &avatar;
    context.config = &config;
    const int talkingMargin = config.talkingBounce ? config.bouncePixels : 0;
    const int idleMargin = gIdleBob ? gIdleBobPixels : 0;
    context.bounceMargin = talkingMargin + idleMargin;
    context.width = avatar.maxWidth > 0 ? avatar.maxWidth : 1;
    context.height = avatar.maxHeight + context.bounceMargin * 2;
    if (context.height < 1) context.height = 1;
    context.baseX = config.windowX;
    context.baseY = config.windowY;
    context.draggable = config.draggable;
    context.direction = DIR_CENTER;

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    if (config.alwaysOnTop) exStyle |= WS_EX_TOPMOST;
    context.hwnd = CreateWindowExW(exStyle, L"V_mini_me_Window", L"V_mini_me", WS_POPUP,
                                   context.baseX, context.baseY, context.width, context.height,
                                   nullptr, nullptr, gInstance, nullptr);
    if (!context.hwnd) return false;

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = context.width;
    info.bmiHeader.biHeight = -context.height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    context.memoryDc = CreateCompatibleDC(nullptr);
    if (!context.memoryDc) {
        DestroyWindow(context.hwnd);
        context.hwnd = nullptr;
        return false;
    }
    context.canvasBitmap = CreateDIBSection(context.memoryDc, &info, DIB_RGB_COLORS, (LPVOID*)&context.canvasBits, nullptr, 0);
    if (!context.canvasBitmap || !context.canvasBits) {
        if (context.canvasBitmap) DeleteObject(context.canvasBitmap);
        DeleteDC(context.memoryDc);
        DestroyWindow(context.hwnd);
        context.hwnd = nullptr;
        context.memoryDc = nullptr;
        return false;
    }
    context.oldBitmap = SelectObject(context.memoryDc, context.canvasBitmap);

    gWindowContext = &context;
    renderWindow(context);
    ShowWindow(context.hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(context.hwnd);
    return true;
}

void destroyAvatarWindow(WindowContext& context) {
    if (context.hwnd) DestroyWindow(context.hwnd);
    if (context.memoryDc && context.oldBitmap) SelectObject(context.memoryDc, context.oldBitmap);
    if (context.canvasBitmap) DeleteObject(context.canvasBitmap);
    if (context.memoryDc) DeleteDC(context.memoryDc);
    if (gWindowContext == &context) gWindowContext = nullptr;
    memset(&context, 0, sizeof(context));
}
