// V_mini_me - lightweight Windows build
// -------------------------------------
// This Windows port intentionally uses Win32, GDI+, and WinMM directly rather
// than Qt, .NET, SDL, Electron, or a game engine. The executable has no C/C++
// runtime dependency: it uses a tiny set of helper functions and Windows system
// DLLs only. The goal is low idle memory/CPU use and broad compatibility.

extern "C" void* memset(void* destination, int value, SIZE_T count) {
    BYTE* out = (BYTE*)destination;
    for (SIZE_T i = 0; i < count; ++i) out[i] = (BYTE)value;
    return destination;
}

extern "C" void* memcpy(void* destination, const void* source, SIZE_T count) {
    BYTE* out = (BYTE*)destination;
    const BYTE* in = (const BYTE*)source;
    for (SIZE_T i = 0; i < count; ++i) out[i] = in[i];
    return destination;
}

namespace {

constexpr int kPathCapacity = 512;
constexpr int kLineCapacity = 512;
constexpr int kDirectionCount = 9;
constexpr int kScaleBase = 1000;
constexpr int kThresholdBase = 1000000;

constexpr LONG ACTION_NONE = 0;
constexpr LONG ACTION_SETUP = 1;
constexpr LONG ACTION_RELOAD = 2;
constexpr LONG ACTION_QUIT = 3;

constexpr int DIR_CENTER = 0;
constexpr int DIR_LEFT = 1;
constexpr int DIR_RIGHT = 2;
constexpr int DIR_UP = 3;
constexpr int DIR_DOWN = 4;
constexpr int DIR_UP_LEFT = 5;
constexpr int DIR_UP_RIGHT = 6;
constexpr int DIR_DOWN_LEFT = 7;
constexpr int DIR_DOWN_RIGHT = 8;

enum DirectionMode {
    MODE_HORIZONTAL = 0,
    MODE_FOURWAY = 1,
    MODE_QUADRANTS = 2,
    MODE_EIGHTWAY = 3
};

const wchar_t* kDirectionNames[kDirectionCount] = {
    L"center", L"left", L"right", L"up", L"down",
    L"up_left", L"up_right", L"down_left", L"down_right"
};

struct AppConfig {
    wchar_t avatarDir[kPathCapacity];
    DirectionMode directionMode;
    int deadzoneMilli;
    int updateMs;
    int scaleMilli;
    int windowX;
    int windowY;
    bool alwaysOnTop;
    bool draggable;
    bool showSetupOnStart;
    bool microphoneEnabled;
    int microphoneDevice;
    int microphoneThresholdMicro;
    int microphoneReleaseMs;
    bool talkingBounce;
    int bouncePixels;
    int bounceIntervalMs;
};

struct ImageBitmap {
    BYTE* pixels;
    int width;
    int height;
    bool loaded;
};

struct AvatarSet {
    ImageBitmap idle[kDirectionCount];
    ImageBitmap talking[kDirectionCount];
    int maxWidth;
    int maxHeight;
};

struct MicrophoneMonitor {
    HWAVEIN handle;
    WAVEHDR headers[2];
    short* samples[2];
    bool available;
    int thresholdMicro;
};

struct WindowContext {
    HWND hwnd;
    HDC memoryDc;
    HBITMAP canvasBitmap;
    HGDIOBJ oldBitmap;
    BYTE* canvasBits;
    int width;
    int height;
    int bounceMargin;
    int baseX;
    int baseY;
    bool draggable;
    bool dragging;
    int dragMouseX;
    int dragMouseY;
    int dragBaseX;
    int dragBaseY;
    AvatarSet* avatar;
    AppConfig* config;
    int direction;
    bool talking;
    int bounce;
};

volatile LONG gRequestedAction = ACTION_NONE;
volatile LONG gSessionResult = ACTION_NONE;
WindowContext* gWindowContext = nullptr;
bool gWindowClassRegistered = false;
ULONG_PTR gGdiPlusToken = 0;
HINSTANCE gInstance = nullptr;
wchar_t gExeDirectory[kPathCapacity] = {};
wchar_t gConfigPath[kPathCapacity] = {};

// -------------------------------------------------------------------------
// Small string/console helpers
// -------------------------------------------------------------------------

int wlen(const wchar_t* text) {
    if (!text) return 0;
    int n = 0;
    while (text[n]) ++n;
    return n;
}

int alen(const char* text) {
    if (!text) return 0;
    int n = 0;
    while (text[n]) ++n;
    return n;
}

void wcopy(wchar_t* destination, int capacity, const wchar_t* source) {
    if (capacity <= 0) return;
    int i = 0;
    if (source) {
        while (source[i] && i < capacity - 1) {
            destination[i] = source[i];
            ++i;
        }
    }
    destination[i] = 0;
}

void wappend(wchar_t* destination, int capacity, const wchar_t* source) {
    int at = wlen(destination);
    int i = 0;
    while (source && source[i] && at < capacity - 1) destination[at++] = source[i++];
    destination[at] = 0;
}

bool wequalInsensitive(const wchar_t* a, const wchar_t* b) {
    int i = 0;
    while (a[i] && b[i]) {
        wchar_t ca = a[i];
        wchar_t cb = b[i];
        if (ca >= L'A' && ca <= L'Z') ca += L'a' - L'A';
        if (cb >= L'A' && cb <= L'Z') cb += L'a' - L'A';
        if (ca != cb) return false;
        ++i;
    }
    return a[i] == 0 && b[i] == 0;
}

bool akeyEqual(const char* text, int textLength, const char* key) {
    int keyLength = alen(key);
    if (textLength != keyLength) return false;
    for (int i = 0; i < keyLength; ++i) {
        char a = text[i];
        char b = key[i];
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return true;
}

void writeText(const wchar_t* text) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    WriteConsoleW(out, text, (DWORD)wlen(text), &written, nullptr);
}

void writeLine(const wchar_t* text) {
    writeText(text);
    writeText(L"\r\n");
}

void writeInt(int value) {
    wchar_t buffer[32];
    int pos = 0;
    unsigned int number;
    if (value < 0) {
        buffer[pos++] = L'-';
        number = (unsigned int)(-(long long)value);
    } else {
        number = (unsigned int)value;
    }
    wchar_t digits[16];
    int count = 0;
    do {
        digits[count++] = (wchar_t)(L'0' + (number % 10));
        number /= 10;
    } while (number && count < 16);
    while (count > 0) buffer[pos++] = digits[--count];
    buffer[pos] = 0;
    writeText(buffer);
}

void writeFixedMilli(int value) {
    int whole = value / 1000;
    int frac = value % 1000;
    writeInt(whole);
    writeText(L".");
    wchar_t digits[4];
    digits[0] = (wchar_t)(L'0' + (frac / 100) % 10);
    digits[1] = (wchar_t)(L'0' + (frac / 10) % 10);
    digits[2] = (wchar_t)(L'0' + frac % 10);
    digits[3] = 0;
    // Trim trailing zeroes while leaving at least one decimal digit.
    if (digits[2] == L'0') digits[2] = 0;
    if (digits[1] == L'0' && digits[2] == 0) digits[1] = 0;
    writeText(digits);
}

bool readLine(wchar_t* buffer, int capacity) {
    if (capacity <= 1) return false;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD read = 0;
    if (!ReadConsoleW(input, buffer, (DWORD)(capacity - 1), &read, nullptr)) return false;
    int length = (int)read;
    while (length > 0 && (buffer[length - 1] == L'\r' || buffer[length - 1] == L'\n')) --length;
    buffer[length] = 0;
    return true;
}

int parseIntW(const wchar_t* text, int fallback) {
    if (!text || !text[0]) return fallback;
    int i = 0;
    bool negative = false;
    if (text[i] == L'-') { negative = true; ++i; }
    int value = 0;
    bool any = false;
    while (text[i] >= L'0' && text[i] <= L'9') {
        value = value * 10 + (text[i] - L'0');
        any = true;
        ++i;
    }
    return any ? (negative ? -value : value) : fallback;
}

int parseFixedW(const wchar_t* text, int base, int fallback) {
    if (!text || !text[0]) return fallback;
    int i = 0;
    bool negative = false;
    if (text[i] == L'-') { negative = true; ++i; }
    long long whole = 0;
    bool any = false;
    while (text[i] >= L'0' && text[i] <= L'9') {
        whole = whole * 10 + (text[i] - L'0');
        any = true;
        ++i;
    }
    long long result = whole * base;
    if (text[i] == L'.') {
        ++i;
        int place = base / 10;
        while (place > 0 && text[i] >= L'0' && text[i] <= L'9') {
            result += (text[i] - L'0') * place;
            place /= 10;
            ++i;
            any = true;
        }
    }
    if (!any) return fallback;
    if (negative) result = -result;
    return (int)result;
}

int parseIntA(const char* text, int length, int fallback) {
    int i = 0;
    bool negative = false;
    if (length > 0 && text[0] == '-') { negative = true; ++i; }
    int value = 0;
    bool any = false;
    while (i < length && text[i] >= '0' && text[i] <= '9') {
        value = value * 10 + (text[i] - '0');
        any = true;
        ++i;
    }
    return any ? (negative ? -value : value) : fallback;
}

int parseFixedA(const char* text, int length, int base, int fallback) {
    int i = 0;
    bool negative = false;
    if (i < length && text[i] == '-') { negative = true; ++i; }
    long long whole = 0;
    bool any = false;
    while (i < length && text[i] >= '0' && text[i] <= '9') {
        whole = whole * 10 + (text[i] - '0');
        any = true;
        ++i;
    }
    long long result = whole * base;
    if (i < length && text[i] == '.') {
        ++i;
        int place = base / 10;
        while (place > 0 && i < length && text[i] >= '0' && text[i] <= '9') {
            result += (text[i] - '0') * place;
            place /= 10;
            ++i;
            any = true;
        }
    }
    if (!any) return fallback;
    return (int)(negative ? -result : result);
}

bool parseBoolA(const char* text, int length, bool fallback) {
    if ((length == 4 && (text[0] == 't' || text[0] == 'T')) ||
        (length == 2 && (text[0] == 'o' || text[0] == 'O') && (text[1] == 'n' || text[1] == 'N')) ||
        (length == 1 && text[0] == '1')) return true;
    if ((length == 5 && (text[0] == 'f' || text[0] == 'F')) ||
        (length == 3 && (text[0] == 'o' || text[0] == 'O') && (text[1] == 'f' || text[1] == 'F')) ||
        (length == 1 && text[0] == '0')) return false;
    return fallback;
}
