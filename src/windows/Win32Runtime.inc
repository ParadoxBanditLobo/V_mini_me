// -------------------------------------------------------------------------
// Avatar session thread and runtime console commands
// -------------------------------------------------------------------------

constexpr LONG ACTION_EXPRESSION = 4;

int idleMotionOffset(DWORD elapsed, bool enabled, int pixels, int periodMs) {
    if (!enabled || pixels <= 0 || periodMs <= 0) return 0;

    const DWORD period = (DWORD)periodMs;
    const DWORD quarter = period / 4;
    if (quarter == 0) return 0;
    const DWORD phase = elapsed % period;

    if (phase < quarter) return (int)((phase * (DWORD)pixels) / quarter);
    if (phase < quarter * 2) return pixels - (int)(((phase - quarter) * (DWORD)pixels) / quarter);
    if (phase < quarter * 3) return -(int)(((phase - quarter * 2) * (DWORD)pixels) / quarter);
    return -pixels + (int)(((phase - quarter * 3) * (DWORD)pixels) / quarter);
}

DWORD WINAPI avatarThread(LPVOID parameter) {
    AppConfig* config = (AppConfig*)parameter;
    AvatarSet avatar{};
    if (!loadAvatar(*config, avatar)) {
        InterlockedExchange(&gSessionResult, ACTION_SETUP);
        return 0;
    }

    WindowContext window{};
    if (!createAvatarWindow(window, avatar, *config)) {
        writeLine(L"Error: could not create the transparent Windows avatar window.");
        freeAvatar(avatar);
        InterlockedExchange(&gSessionResult, ACTION_SETUP);
        return 0;
    }

    MicrophoneMonitor microphone{};
    if (config->microphoneEnabled && !startMicrophone(microphone, *config)) {
        writeLine(L"Warning: microphone reaction is unavailable; continuing without it.");
        stopMicrophone(microphone);
    }

    DWORD now = GetTickCount();
    const DWORD sessionStarted = now;
    DWORD nextDirectionUpdate = now;
    DWORD lastVoiceActivity = now;
    DWORD talkingStarted = now;
    int currentDirection = DIR_CENTER;
    bool currentTalking = false;
    int currentVerticalOffset = 0;
    int currentHorizontalOffset = 0;
    gCurrentSwayOffset = 0;

    while (true) {
        MSG message{};
        bool quitMessage = false;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == 0x0012) { quitMessage = true; break; }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        LONG action = InterlockedCompareExchange(&gRequestedAction, ACTION_NONE, ACTION_NONE);
        if (quitMessage && action == ACTION_NONE) action = ACTION_QUIT;
        if (action != ACTION_NONE) {
            InterlockedExchange(&gSessionResult, action);
            break;
        }

        now = GetTickCount();
        int nextDirection = currentDirection;
        bool nextTalking = currentTalking;

        if ((long)(now - nextDirectionUpdate) >= 0) {
            POINT mouse{};
            if (GetCursorPos(&mouse)) {
                nextDirection = determineDirection(mouse.x, mouse.y,
                    GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                    config->deadzoneMilli, config->directionMode);
            }
            nextDirectionUpdate = now + (DWORD)config->updateMs;
        }

        if (microphone.available) {
            if (microphoneLoud(microphone)) {
                lastVoiceActivity = now;
                if (!nextTalking) {
                    nextTalking = true;
                    talkingStarted = now;
                }
            } else if (nextTalking && (DWORD)(now - lastVoiceActivity) >= (DWORD)config->microphoneReleaseMs) {
                nextTalking = false;
            }
        } else {
            nextTalking = false;
        }

        int talkingOffset = 0;
        if (nextTalking && config->talkingBounce && config->bouncePixels > 0) {
            DWORD phase = (now - talkingStarted) / (DWORD)config->bounceIntervalMs;
            talkingOffset = (phase % 2 == 0) ? config->bouncePixels : 0;
        }

        const DWORD idleElapsed = now - sessionStarted;
        const int bobOffset = idleMotionOffset(idleElapsed, gIdleBob, gIdleBobPixels, gIdleBobPeriodMs);
        const int swayOffset = idleMotionOffset(idleElapsed, gIdleSway, gIdleSwayPixels, gIdleSwayPeriodMs);
        const int nextVerticalOffset = talkingOffset + bobOffset;
        const int nextHorizontalOffset = swayOffset;

        if (nextDirection != currentDirection ||
            nextTalking != currentTalking ||
            nextVerticalOffset != currentVerticalOffset ||
            nextHorizontalOffset != currentHorizontalOffset) {
            currentDirection = nextDirection;
            currentTalking = nextTalking;
            currentVerticalOffset = nextVerticalOffset;
            currentHorizontalOffset = nextHorizontalOffset;
            gCurrentSwayOffset = currentHorizontalOffset;
            window.direction = currentDirection;
            window.talking = currentTalking;
            window.bounce = currentVerticalOffset;
            renderWindow(window);
        }
        Sleep(5);
    }

    gCurrentSwayOffset = 0;
    config->windowX = window.baseX;
    config->windowY = window.baseY;
    stopMicrophone(microphone);
    destroyAvatarWindow(window);
    freeAvatar(avatar);
    return 0;
}

void printTitle() {
    writeLine(L"");
    writeLine(L" __      __                 _       _                 ");
    writeLine(L" \\ \\    / /                (_)     (_)                ");
    writeLine(L"  \\ \\  / / _ __ ___  _ __  _ _ __  _   _ __ ___   ___");
    writeLine(L"   \\ \\/ / | '_ ` _ \\| '_ \\| | '_ \\| | | '_ ` _ \\ / _ \\");
    writeLine(L"    \\  /  | | | | | | | | | | | | | | | | | | | | |  __/");
    writeLine(L"     \\/   |_| |_| |_|_| |_|_|_| |_|_| |_| |_| |_| |_|\\___|");
    writeLine(L"                       V_mini_me");
}

void printRuntimeHelp() {
    writeLine(L"");
    writeLine(L"V_mini_me is running.");
    writeLine(L"  Left-drag avatar : move it");
    writeLine(L"  Right-click avatar: return to setup");
    writeLine(L"  E / expressions   : quick expression menu");
    writeLine(L"  S / setup         : return to setup");
    writeLine(L"  R / reload        : reload config/avatar");
    writeLine(L"  H / help          : show these commands");
    writeLine(L"  Q / quit          : exit");
    writeLine(L"");
}

bool chooseRuntimeExpression() {
    writeLine(L"");
    writeLine(L"QUICK EXPRESSIONS");
    writeLine(L"");
    bool any = false;
    for (int i = 0; i < 8; ++i) {
        if (!gExpressionDirs[i][0]) continue;
        any = true;
        writeText(L"  "); writeInt(i + 1); writeText(L". "); writeLine(gExpressionDirs[i]);
    }
    if (!any) {
        writeLine(L"  No expression slots are configured.");
        writeLine(L"  Return to setup and choose Quick expressions to add some.");
        return false;
    }
    writeLine(L"  B. Back");
    writeText(L"Expression: ");
    wchar_t input[kLineCapacity];
    if (!readLine(input, kLineCapacity)) return false;
    if (wequalInsensitive(input, L"b") || wequalInsensitive(input, L"back") || !input[0]) return false;
    int slot = parseIntW(input, 0);
    if (slot < 1 || slot > 8 || !gExpressionDirs[slot - 1][0]) {
        writeLine(L"That expression slot is not configured.");
        return false;
    }
    wcopy(gRuntimeAvatarDir, kPathCapacity, gExpressionDirs[slot - 1]);
    gRuntimeAvatarOverride = true;
    writeText(L"Switched to: "); writeLine(gRuntimeAvatarDir);
    return true;
}

LONG runSession(AppConfig& config) {
    InterlockedExchange(&gRequestedAction, ACTION_NONE);
    InterlockedExchange(&gSessionResult, ACTION_NONE);
    DWORD threadId = 0;
    HANDLE thread = CreateThread(nullptr, 0, avatarThread, &config, 0, &threadId);
    if (!thread) {
        writeLine(L"Error: could not start avatar session thread.");
        return ACTION_QUIT;
    }
    printRuntimeHelp();
    HANDLE waits[2] = {thread, GetStdHandle(STD_INPUT_HANDLE)};

    while (true) {
        DWORD result = WaitForMultipleObjects(2, waits, FALSE_VALUE, INFINITE_WAIT);
        if (result == WAIT_OBJECT_0) break;
        if (result == WAIT_OBJECT_0 + 1) {
            wchar_t command[kLineCapacity];
            if (!readLine(command, kLineCapacity)) {
                InterlockedExchange(&gRequestedAction, ACTION_QUIT);
                continue;
            }
            if (wequalInsensitive(command, L"e") || wequalInsensitive(command, L"expressions") || wequalInsensitive(command, L"expression")) {
                if (chooseRuntimeExpression()) InterlockedExchange(&gRequestedAction, ACTION_EXPRESSION);
            } else if (wequalInsensitive(command, L"s") || wequalInsensitive(command, L"setup")) {
                InterlockedExchange(&gRequestedAction, ACTION_SETUP);
            } else if (wequalInsensitive(command, L"r") || wequalInsensitive(command, L"reload")) {
                InterlockedExchange(&gRequestedAction, ACTION_RELOAD);
            } else if (wequalInsensitive(command, L"q") || wequalInsensitive(command, L"quit")) {
                InterlockedExchange(&gRequestedAction, ACTION_QUIT);
            } else if (wequalInsensitive(command, L"h") || wequalInsensitive(command, L"help")) {
                printRuntimeHelp();
            } else if (command[0]) {
                writeLine(L"Unknown command. Type H for help.");
            }
        }
    }
    WaitForSingleObject(thread, INFINITE_WAIT);
    CloseHandle(thread);
    LONG result = InterlockedCompareExchange(&gSessionResult, ACTION_NONE, ACTION_NONE);
    saveConfig(config);
    return result == ACTION_NONE ? ACTION_QUIT : result;
}

// -------------------------------------------------------------------------
// Text-only setup menu
// -------------------------------------------------------------------------

void configureQuickExpressions() {
    while (true) {
        writeLine(L"");
        writeLine(L"QUICK EXPRESSIONS");
        writeLine(L"");
        for (int i = 0; i < 8; ++i) {
            writeText(L"  "); writeInt(i + 1); writeText(L". ");
            writeLine(gExpressionDirs[i][0] ? gExpressionDirs[i] : L"[empty]");
        }
        writeLine(L"");
        writeLine(L"Select a slot to assign/change, or B to go back.");
        writeText(L"Select: ");
        wchar_t input[kLineCapacity];
        if (!readLine(input, kLineCapacity)) return;
        if (wequalInsensitive(input, L"b") || wequalInsensitive(input, L"back") || !input[0]) return;
        int slot = parseIntW(input, 0);
        if (slot < 1 || slot > 8) {
            writeLine(L"Choose a slot from 1 to 8.");
            continue;
        }
        writeLine(L"Enter an avatar folder path. Leave blank to clear the slot.");
        writeText(L"Path: ");
        if (!readLine(input, kLineCapacity)) return;
        if (!input[0]) {
            gExpressionDirs[slot - 1][0] = 0;
            continue;
        }
        wchar_t avatarBase[kPathCapacity];
        joinPath(avatarBase, kPathCapacity, gExeDirectory, input);
        wchar_t centerPath[kPathCapacity];
        buildAvatarPath(centerPath, avatarBase, DIR_CENTER, false);
        if (!fileExists(centerPath)) {
            writeLine(L"That folder does not contain center.png. No change made.");
            continue;
        }
        wcopy(gExpressionDirs[slot - 1], kPathCapacity, input);
    }
}

void printSetup(const AppConfig& config) {
    printTitle();
    writeLine(L"");
    writeLine(L"========================================");
    writeLine(L"                 SETUP");
    writeLine(L"========================================");
    writeText(L"  1. Avatar folder      "); writeLine(config.avatarDir);
    writeText(L"  2. Scale              "); writeFixedMilli(config.scaleMilli); writeLine(L"");
    writeText(L"  3. Direction mode     "); writeLine(modeNameW(config.directionMode));
    writeText(L"  4. Microphone         "); writeLine(config.microphoneEnabled ? L"ON" : L"OFF");
    writeText(L"  5. Talking bounce     "); writeLine(config.talkingBounce ? L"ON" : L"OFF");
    writeText(L"  6. Idle bob           "); writeLine(gIdleBob ? L"ON" : L"OFF");
    writeText(L"  7. Idle sway          "); writeLine(gIdleSway ? L"ON" : L"OFF");
    writeLine(L"  8. Quick expressions");
    writeText(L"  9. Setup on startup   "); writeLine(config.showSetupOnStart ? L"ON" : L"OFF");
    writeLine(L"");
    writeLine(L"  S. Save and start");
    writeLine(L"  Q. Save and quit");
    writeLine(L"");
    writeText(L"Select: ");
}

bool runSetup(AppConfig& config) {
    while (true) {
        printSetup(config);
        wchar_t input[kLineCapacity];
        if (!readLine(input, kLineCapacity)) return false;
        if (wequalInsensitive(input, L"s")) {
            clampConfig(config);
            saveConfig(config);
            return true;
        }
        if (wequalInsensitive(input, L"q")) {
            clampConfig(config);
            saveConfig(config);
            return false;
        }
        int option = parseIntW(input, 0);
        if (option == 1) {
            writeLine(L"Enter an avatar folder path.");
            writeLine(L"Relative paths are resolved from the V_mini_me.exe folder.");
            writeText(L"Path: ");
            if (readLine(input, kLineCapacity) && input[0]) wcopy(config.avatarDir, kPathCapacity, input);
        } else if (option == 2) {
            writeLine(L"Scale examples: 0.5 = half, 1.0 = original, 2.0 = double");
            writeText(L"Scale (0.10 to 8.00): ");
            if (readLine(input, kLineCapacity)) config.scaleMilli = parseFixedW(input, 1000, config.scaleMilli);
            clampConfig(config);
        } else if (option == 3) {
            writeLine(L"1. horizontal   2. fourway   3. quadrants   4. eightway");
            writeText(L"Mode: ");
            if (readLine(input, kLineCapacity)) {
                int mode = parseIntW(input, 4);
                if (mode >= 1 && mode <= 4) config.directionMode = (DirectionMode)(mode - 1);
            }
        } else if (option == 4) {
            config.microphoneEnabled = !config.microphoneEnabled;
        } else if (option == 5) {
            config.talkingBounce = !config.talkingBounce;
        } else if (option == 6) {
            gIdleBob = !gIdleBob;
        } else if (option == 7) {
            gIdleSway = !gIdleSway;
        } else if (option == 8) {
            configureQuickExpressions();
        } else if (option == 9) {
            config.showSetupOnStart = !config.showSetupOnStart;
        }
    }
}

bool commandHasSetupFlag() {
    const wchar_t* command = GetCommandLineW();
    const wchar_t* token = L"--setup";
    int tokenLength = wlen(token);
    for (int i = 0; command && command[i]; ++i) {
        int j = 0;
        while (j < tokenLength && command[i + j] == token[j]) ++j;
        if (j == tokenLength) return true;
    }
    return false;
}

} // namespace

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

    bool showSetup = commandHasSetupFlag() || config.showSetupOnStart;
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
