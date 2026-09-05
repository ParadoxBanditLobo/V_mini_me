#include "Win32ApiLite.hpp"

// Release/test entry used by the optional Windows GUI launcher.
// It keeps the normal native Windows core intact, but adds two small pieces of
// launcher compatibility without changing the runtime architecture:
//   1) stdin may be a pipe instead of a real console;
//   2) --no-setup suppresses the startup setup menu for GUI-controlled runs.

#define readLine vminiOriginalConsoleReadLine
#include "Win32Core.inc"
#undef readLine

bool readLine(wchar_t* buffer, int capacity) {
    if (capacity <= 1) return false;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD read = 0;

    // Normal direct launch: preserve the original Unicode console behavior.
    if (ReadConsoleW(input, buffer, (DWORD)(capacity - 1), &read, nullptr)) {
        int length = (int)read;
        while (length > 0 && (buffer[length - 1] == L'\r' || buffer[length - 1] == L'\n')) --length;
        buffer[length] = 0;
        return true;
    }

    // GUI launcher: stdin is an anonymous byte pipe. Commands sent by the
    // launcher are UTF-8/ASCII lines (e, slot number, r, q).
    char bytes[kLineCapacity * 4];
    int count = 0;
    while (count < (int)sizeof(bytes) - 1) {
        char ch = 0;
        DWORD got = 0;
        if (!ReadFile(input, &ch, 1, &got, nullptr) || got == 0) {
            buffer[0] = 0;
            return count > 0;
        }
        if (ch == '\n') break;
        if (ch == '\r') continue;
        bytes[count++] = ch;
    }
    bytes[count] = 0;

    int converted = MultiByteToWideChar(CP_UTF8, 0, bytes, count, buffer, capacity - 1);
    if (converted < 0) converted = 0;
    buffer[converted] = 0;
    return true;
}

#include "Win32Config.inc"
#include "Win32Avatar.inc"
#include "Win32AudioWindow.inc"

#define mainCRTStartup vminiOriginalMainCRTStartup
#include "Win32Runtime.inc"
#undef mainCRTStartup

bool commandHasNoSetupFlag() {
    const wchar_t* command = GetCommandLineW();
    const wchar_t* token = L"--no-setup";
    int tokenLength = wlen(token);
    for (int i = 0; command && command[i]; ++i) {
        int j = 0;
        while (j < tokenLength && command[i + j] == token[j]) ++j;
        if (j == tokenLength) return true;
    }
    return false;
}

extern "C" void mainCRTStartup() {
    initializePaths();
    gInstance = (HINSTANCE)GetModuleHandleW(nullptr);

    GdiplusStartupInput gdiplusInput{};
    gdiplusInput.GdiplusVersion = 1;
    if (GdiplusStartup(&gGdiPlusToken, &gdiplusInput, nullptr) != 0) {
        writeLine(L"V_mini_me could not initialize Windows PNG support (GDI+).");
        ExitProcess(1);
    }

    AppConfig config{};
    bool existed = loadConfig(config);
    if (!existed) {
        setDefaults(config);
        saveConfig(config);
    }

    bool showSetup = !commandHasNoSetupFlag() && (commandHasSetupFlag() || config.showSetupOnStart);
    bool running = true;
    while (running) {
        if (showSetup) {
            gRuntimeAvatarOverride = false;
            if (!runSetup(config)) break;
        }
        LONG result = runSession(config);
        if (result == ACTION_QUIT) {
            running = false;
        } else if (result == ACTION_EXPRESSION) {
            showSetup = false;
        } else if (result == ACTION_SETUP) {
            gRuntimeAvatarOverride = false;
            loadConfig(config);
            showSetup = true;
        } else if (result == ACTION_RELOAD) {
            gRuntimeAvatarOverride = false;
            loadConfig(config);
            showSetup = false;
        } else {
            running = false;
        }
    }

    GdiplusShutdown(gGdiPlusToken);
    ExitProcess(0);
}
