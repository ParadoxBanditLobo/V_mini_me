// -------------------------------------------------------------------------
// Paths and config.ini
// -------------------------------------------------------------------------

bool gIdleBob = false;
int gIdleBobPixels = 3;
int gIdleBobPeriodMs = 2400;
bool gIdleSway = false;
int gIdleSwayPixels = 2;
int gIdleSwayPeriodMs = 3000;
int gCurrentSwayOffset = 0;
wchar_t gExpressionDirs[8][kPathCapacity] = {};
bool gRuntimeAvatarOverride = false;
wchar_t gRuntimeAvatarDir[kPathCapacity] = {};

void setDefaults(AppConfig& config) {
    memset(&config, 0, sizeof(config));
    wcopy(config.avatarDir, kPathCapacity, L"avatar/default");
    config.directionMode = MODE_EIGHTWAY;
    config.deadzoneMilli = 150;
    config.updateMs = 100;
    config.scaleMilli = 1000;
    config.windowX = 40;
    config.windowY = 40;
    config.alwaysOnTop = true;
    config.draggable = true;
    config.showSetupOnStart = true;
    config.microphoneEnabled = true;
    config.microphoneDevice = -1;
    config.microphoneThresholdMicro = 35000;
    config.microphoneReleaseMs = 250;
    config.talkingBounce = true;
    config.bouncePixels = 4;
    config.bounceIntervalMs = 150;
    gIdleBob = false;
    gIdleBobPixels = 3;
    gIdleBobPeriodMs = 2400;
    gIdleSway = false;
    gIdleSwayPixels = 2;
    gIdleSwayPeriodMs = 3000;
    gCurrentSwayOffset = 0;
    for (int i = 0; i < 8; ++i) gExpressionDirs[i][0] = 0;
}

bool isAbsolutePath(const wchar_t* path) {
    if (!path || !path[0]) return false;
    if (path[0] == L'\\' || path[0] == L'/') return true;
    return path[1] == L':';
}

void joinPath(wchar_t* output, int capacity, const wchar_t* base, const wchar_t* child) {
    output[0] = 0;
    if (isAbsolutePath(child)) {
        wcopy(output, capacity, child);
        return;
    }
    wcopy(output, capacity, base);
    int length = wlen(output);
    if (length > 0 && output[length - 1] != L'\\' && output[length - 1] != L'/') wappend(output, capacity, L"\\");
    wappend(output, capacity, child);
    for (int i = 0; output[i]; ++i) if (output[i] == L'/') output[i] = L'\\';
}

void initializePaths() {
    wchar_t modulePath[kPathCapacity];
    DWORD length = GetModuleFileNameW(nullptr, modulePath, kPathCapacity);
    if (length == 0 || length >= (DWORD)kPathCapacity) {
        wcopy(gExeDirectory, kPathCapacity, L".");
    } else {
        int lastSlash = -1;
        for (int i = 0; modulePath[i]; ++i) if (modulePath[i] == L'\\' || modulePath[i] == L'/') lastSlash = i;
        if (lastSlash < 0) {
            wcopy(gExeDirectory, kPathCapacity, L".");
        } else {
            modulePath[lastSlash] = 0;
            wcopy(gExeDirectory, kPathCapacity, modulePath);
        }
    }
    joinPath(gConfigPath, kPathCapacity, gExeDirectory, L"config.ini");
}

bool fileExists(const wchar_t* path) {
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

void decodeUtf8(const char* input, int length, wchar_t* output, int capacity) {
    if (capacity <= 0) return;
    int count = MultiByteToWideChar(CP_UTF8, 0, input, length, output, capacity - 1);
    if (count < 0) count = 0;
    output[count] = 0;
}

int encodeUtf8(const wchar_t* input, char* output, int capacity) {
    if (capacity <= 0) return 0;
    int count = WideCharToMultiByte(CP_UTF8, 0, input, -1, output, capacity, nullptr, nullptr);
    if (count <= 0) { output[0] = 0; return 0; }
    return count - 1;
}

DirectionMode parseModeA(const char* value, int length, DirectionMode fallback) {
    if (akeyEqual(value, length, "horizontal") || (length == 1 && value[0] == '3')) return MODE_HORIZONTAL;
    if (akeyEqual(value, length, "fourway") || akeyEqual(value, length, "4way") || (length == 1 && value[0] == '4')) return MODE_FOURWAY;
    if (akeyEqual(value, length, "quadrants") || akeyEqual(value, length, "quadrant")) return MODE_QUADRANTS;
    if (akeyEqual(value, length, "eightway") || akeyEqual(value, length, "8way") || (length == 1 && value[0] == '8')) return MODE_EIGHTWAY;
    return fallback;
}

const char* modeNameA(DirectionMode mode) {
    if (mode == MODE_HORIZONTAL) return "horizontal";
    if (mode == MODE_FOURWAY) return "fourway";
    if (mode == MODE_QUADRANTS) return "quadrants";
    return "eightway";
}

const wchar_t* modeNameW(DirectionMode mode) {
    if (mode == MODE_HORIZONTAL) return L"horizontal";
    if (mode == MODE_FOURWAY) return L"fourway";
    if (mode == MODE_QUADRANTS) return L"quadrants";
    return L"eightway";
}

int expressionIndexA(const char* key, int keyLength) {
    if (keyLength != 12) return -1;
    if (!akeyEqual(key, 11, "expression_")) return -1;
    if (key[11] < '1' || key[11] > '8') return -1;
    return key[11] - '1';
}

void applyConfigValue(AppConfig& config, const char* key, int keyLength, const char* value, int valueLength) {
    if (akeyEqual(key, keyLength, "avatar_dir")) {
        decodeUtf8(value, valueLength, config.avatarDir, kPathCapacity);
    } else if (akeyEqual(key, keyLength, "direction_mode")) {
        config.directionMode = parseModeA(value, valueLength, config.directionMode);
    } else if (akeyEqual(key, keyLength, "deadzone")) {
        config.deadzoneMilli = parseFixedA(value, valueLength, 1000, config.deadzoneMilli);
    } else if (akeyEqual(key, keyLength, "update_ms")) {
        config.updateMs = parseIntA(value, valueLength, config.updateMs);
    } else if (akeyEqual(key, keyLength, "scale")) {
        config.scaleMilli = parseFixedA(value, valueLength, 1000, config.scaleMilli);
    } else if (akeyEqual(key, keyLength, "window_x")) {
        config.windowX = parseIntA(value, valueLength, config.windowX);
    } else if (akeyEqual(key, keyLength, "window_y")) {
        config.windowY = parseIntA(value, valueLength, config.windowY);
    } else if (akeyEqual(key, keyLength, "always_on_top")) {
        config.alwaysOnTop = parseBoolA(value, valueLength, config.alwaysOnTop);
    } else if (akeyEqual(key, keyLength, "draggable")) {
        config.draggable = parseBoolA(value, valueLength, config.draggable);
    } else if (akeyEqual(key, keyLength, "show_setup_on_start")) {
        config.showSetupOnStart = parseBoolA(value, valueLength, config.showSetupOnStart);
    } else if (akeyEqual(key, keyLength, "mic_enabled")) {
        config.microphoneEnabled = parseBoolA(value, valueLength, config.microphoneEnabled);
    } else if (akeyEqual(key, keyLength, "mic_device")) {
        if (valueLength > 0) config.microphoneDevice = parseIntA(value, valueLength, -1);
    } else if (akeyEqual(key, keyLength, "mic_threshold")) {
        config.microphoneThresholdMicro = parseFixedA(value, valueLength, kThresholdBase, config.microphoneThresholdMicro);
    } else if (akeyEqual(key, keyLength, "mic_release_ms")) {
        config.microphoneReleaseMs = parseIntA(value, valueLength, config.microphoneReleaseMs);
    } else if (akeyEqual(key, keyLength, "talk_bounce")) {
        config.talkingBounce = parseBoolA(value, valueLength, config.talkingBounce);
    } else if (akeyEqual(key, keyLength, "bounce_pixels")) {
        config.bouncePixels = parseIntA(value, valueLength, config.bouncePixels);
    } else if (akeyEqual(key, keyLength, "bounce_interval_ms")) {
        config.bounceIntervalMs = parseIntA(value, valueLength, config.bounceIntervalMs);
    } else if (akeyEqual(key, keyLength, "idle_bob")) {
        gIdleBob = parseBoolA(value, valueLength, gIdleBob);
    } else if (akeyEqual(key, keyLength, "idle_bob_pixels")) {
        gIdleBobPixels = parseIntA(value, valueLength, gIdleBobPixels);
    } else if (akeyEqual(key, keyLength, "idle_bob_period_ms")) {
        gIdleBobPeriodMs = parseIntA(value, valueLength, gIdleBobPeriodMs);
    } else if (akeyEqual(key, keyLength, "idle_sway")) {
        gIdleSway = parseBoolA(value, valueLength, gIdleSway);
    } else if (akeyEqual(key, keyLength, "idle_sway_pixels")) {
        gIdleSwayPixels = parseIntA(value, valueLength, gIdleSwayPixels);
    } else if (akeyEqual(key, keyLength, "idle_sway_period_ms")) {
        gIdleSwayPeriodMs = parseIntA(value, valueLength, gIdleSwayPeriodMs);
    } else {
        int expressionIndex = expressionIndexA(key, keyLength);
        if (expressionIndex >= 0) decodeUtf8(value, valueLength, gExpressionDirs[expressionIndex], kPathCapacity);
    }
}

void clampConfig(AppConfig& config) {
    if (config.deadzoneMilli < 0) config.deadzoneMilli = 0;
    if (config.deadzoneMilli > 950) config.deadzoneMilli = 950;
    if (config.updateMs < 10) config.updateMs = 10;
    if (config.updateMs > 10000) config.updateMs = 10000;
    if (config.scaleMilli < 100) config.scaleMilli = 100;
    if (config.scaleMilli > 8000) config.scaleMilli = 8000;
    if (config.microphoneThresholdMicro < 100) config.microphoneThresholdMicro = 100;
    if (config.microphoneThresholdMicro > kThresholdBase) config.microphoneThresholdMicro = kThresholdBase;
    if (config.microphoneReleaseMs < 0) config.microphoneReleaseMs = 0;
    if (config.bouncePixels < 0) config.bouncePixels = 0;
    if (config.bouncePixels > 1000) config.bouncePixels = 1000;
    if (config.bounceIntervalMs < 20) config.bounceIntervalMs = 20;
    if (gIdleBobPixels < 0) gIdleBobPixels = 0;
    if (gIdleBobPixels > 200) gIdleBobPixels = 200;
    if (gIdleBobPeriodMs < 200) gIdleBobPeriodMs = 200;
    if (gIdleBobPeriodMs > 30000) gIdleBobPeriodMs = 30000;
    if (gIdleSwayPixels < 0) gIdleSwayPixels = 0;
    if (gIdleSwayPixels > 200) gIdleSwayPixels = 200;
    if (gIdleSwayPeriodMs < 200) gIdleSwayPeriodMs = 200;
    if (gIdleSwayPeriodMs > 30000) gIdleSwayPeriodMs = 30000;
}

bool loadConfig(AppConfig& config) {
    setDefaults(config);
    HANDLE file = CreateFileW(gConfigPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(file, nullptr);
    if (size == 0xFFFFFFFFUL || size > 65535) { CloseHandle(file); return false; }
    char* bytes = (char*)HeapAlloc(GetProcessHeap(), 0, (SIZE_T)size + 1);
    if (!bytes) { CloseHandle(file); return false; }
    DWORD read = 0;
    bool ok = ReadFile(file, bytes, size, &read, nullptr) != 0;
    CloseHandle(file);
    bytes[read] = 0;
    if (!ok) { HeapFree(GetProcessHeap(), 0, bytes); return false; }

    int position = 0;
    while (position < (int)read) {
        int lineStart = position;
        while (position < (int)read && bytes[position] != '\r' && bytes[position] != '\n') ++position;
        int lineEnd = position;
        while (position < (int)read && (bytes[position] == '\r' || bytes[position] == '\n')) ++position;

        while (lineStart < lineEnd && (bytes[lineStart] == ' ' || bytes[lineStart] == '\t')) ++lineStart;
        while (lineEnd > lineStart && (bytes[lineEnd - 1] == ' ' || bytes[lineEnd - 1] == '\t')) --lineEnd;
        if (lineStart >= lineEnd || bytes[lineStart] == '#' || bytes[lineStart] == ';') continue;

        int equals = lineStart;
        while (equals < lineEnd && bytes[equals] != '=') ++equals;
        if (equals >= lineEnd) continue;
        int keyEnd = equals;
        while (keyEnd > lineStart && (bytes[keyEnd - 1] == ' ' || bytes[keyEnd - 1] == '\t')) --keyEnd;
        int valueStart = equals + 1;
        while (valueStart < lineEnd && (bytes[valueStart] == ' ' || bytes[valueStart] == '\t')) ++valueStart;
        applyConfigValue(config, bytes + lineStart, keyEnd - lineStart, bytes + valueStart, lineEnd - valueStart);
    }
    HeapFree(GetProcessHeap(), 0, bytes);
    clampConfig(config);
    return true;
}

void appendA(char* buffer, int capacity, int& at, const char* text) {
    int i = 0;
    while (text && text[i] && at < capacity - 1) buffer[at++] = text[i++];
    buffer[at] = 0;
}

void appendIntA(char* buffer, int capacity, int& at, int value) {
    char temp[32];
    int p = 0;
    unsigned int number;
    if (value < 0) { buffer[at++] = '-'; number = (unsigned int)(-(long long)value); }
    else number = (unsigned int)value;
    do { temp[p++] = (char)('0' + number % 10); number /= 10; } while (number && p < 20);
    while (p > 0 && at < capacity - 1) buffer[at++] = temp[--p];
    buffer[at] = 0;
}

void appendFixedA(char* buffer, int capacity, int& at, int value, int base, int decimals) {
    if (value < 0) { appendA(buffer, capacity, at, "-"); value = -value; }
    appendIntA(buffer, capacity, at, value / base);
    appendA(buffer, capacity, at, ".");
    int fraction = value % base;
    int divisor = base / 10;
    for (int i = 0; i < decimals; ++i) {
        int digit = divisor > 0 ? (fraction / divisor) % 10 : 0;
        if (at < capacity - 1) buffer[at++] = (char)('0' + digit);
        divisor /= 10;
    }
    buffer[at] = 0;
}

bool saveConfig(const AppConfig& config) {
    constexpr int outputCapacity = 12288;
    char* output = (char*)HeapAlloc(GetProcessHeap(), 0, outputCapacity);
    char* pathUtf8 = (char*)HeapAlloc(GetProcessHeap(), 0, 1536);
    if (!output || !pathUtf8) {
        if (output) HeapFree(GetProcessHeap(), 0, output);
        if (pathUtf8) HeapFree(GetProcessHeap(), 0, pathUtf8);
        return false;
    }
    int at = 0;
    output[0] = 0;
    encodeUtf8(config.avatarDir, pathUtf8, 1536);

    appendA(output, outputCapacity, at, "# V_mini_me configuration\r\n# Paths are relative to the executable unless absolute.\r\n\r\navatar_dir=");
    appendA(output, outputCapacity, at, pathUtf8);
    appendA(output, outputCapacity, at, "\r\ndirection_mode="); appendA(output, outputCapacity, at, modeNameA(config.directionMode));
    appendA(output, outputCapacity, at, "\r\ndeadzone="); appendFixedA(output, outputCapacity, at, config.deadzoneMilli, 1000, 3);
    appendA(output, outputCapacity, at, "\r\nupdate_ms="); appendIntA(output, outputCapacity, at, config.updateMs);
    appendA(output, outputCapacity, at, "\r\nscale="); appendFixedA(output, outputCapacity, at, config.scaleMilli, 1000, 3);
    appendA(output, outputCapacity, at, "\r\nwindow_x="); appendIntA(output, outputCapacity, at, config.windowX);
    appendA(output, outputCapacity, at, "\r\nwindow_y="); appendIntA(output, outputCapacity, at, config.windowY);
    appendA(output, outputCapacity, at, "\r\nalways_on_top="); appendA(output, outputCapacity, at, config.alwaysOnTop ? "true" : "false");
    appendA(output, outputCapacity, at, "\r\ndraggable="); appendA(output, outputCapacity, at, config.draggable ? "true" : "false");
    appendA(output, outputCapacity, at, "\r\nshow_setup_on_start="); appendA(output, outputCapacity, at, config.showSetupOnStart ? "true" : "false");
    appendA(output, outputCapacity, at, "\r\n\r\n# Microphone reaction\r\nmic_enabled="); appendA(output, outputCapacity, at, config.microphoneEnabled ? "true" : "false");
    appendA(output, outputCapacity, at, "\r\nmic_device="); if (config.microphoneDevice >= 0) appendIntA(output, outputCapacity, at, config.microphoneDevice);
    appendA(output, outputCapacity, at, "\r\nmic_threshold="); appendFixedA(output, outputCapacity, at, config.microphoneThresholdMicro, kThresholdBase, 6);
    appendA(output, outputCapacity, at, "\r\nmic_release_ms="); appendIntA(output, outputCapacity, at, config.microphoneReleaseMs);
    appendA(output, outputCapacity, at, "\r\ntalk_bounce="); appendA(output, outputCapacity, at, config.talkingBounce ? "true" : "false");
    appendA(output, outputCapacity, at, "\r\nbounce_pixels="); appendIntA(output, outputCapacity, at, config.bouncePixels);
    appendA(output, outputCapacity, at, "\r\nbounce_interval_ms="); appendIntA(output, outputCapacity, at, config.bounceIntervalMs);
    appendA(output, outputCapacity, at, "\r\n\r\n# Idle motion\r\nidle_bob="); appendA(output, outputCapacity, at, gIdleBob ? "true" : "false");
    appendA(output, outputCapacity, at, "\r\nidle_bob_pixels="); appendIntA(output, outputCapacity, at, gIdleBobPixels);
    appendA(output, outputCapacity, at, "\r\nidle_bob_period_ms="); appendIntA(output, outputCapacity, at, gIdleBobPeriodMs);
    appendA(output, outputCapacity, at, "\r\nidle_sway="); appendA(output, outputCapacity, at, gIdleSway ? "true" : "false");
    appendA(output, outputCapacity, at, "\r\nidle_sway_pixels="); appendIntA(output, outputCapacity, at, gIdleSwayPixels);
    appendA(output, outputCapacity, at, "\r\nidle_sway_period_ms="); appendIntA(output, outputCapacity, at, gIdleSwayPeriodMs);
    appendA(output, outputCapacity, at, "\r\n\r\n# Quick expressions\r\n");
    for (int i = 0; i < 8; ++i) {
        appendA(output, outputCapacity, at, "expression_"); appendIntA(output, outputCapacity, at, i + 1); appendA(output, outputCapacity, at, "=");
        encodeUtf8(gExpressionDirs[i], pathUtf8, 1536); appendA(output, outputCapacity, at, pathUtf8); appendA(output, outputCapacity, at, "\r\n");
    }

    HANDLE file = CreateFileW(gConfigPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        HeapFree(GetProcessHeap(), 0, pathUtf8);
        HeapFree(GetProcessHeap(), 0, output);
        return false;
    }
    DWORD written = 0;
    BOOL ok = WriteFile(file, output, (DWORD)at, &written, nullptr);
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, pathUtf8);
    HeapFree(GetProcessHeap(), 0, output);
    return ok != 0 && written == (DWORD)at;
}
