// -------------------------------------------------------------------------
// PNG avatar loading
// -------------------------------------------------------------------------

void freeImage(ImageBitmap& image) {
    if (image.pixels) HeapFree(GetProcessHeap(), 0, image.pixels);
    image.pixels = nullptr;
    image.width = image.height = 0;
    image.loaded = false;
}

void freeAvatar(AvatarSet& avatar) {
    for (int i = 0; i < kDirectionCount; ++i) {
        freeImage(avatar.idle[i]);
        freeImage(avatar.talking[i]);
    }
    avatar.maxWidth = avatar.maxHeight = 0;
}

bool loadPngScaled(const wchar_t* path, int scaleMilli, ImageBitmap& output) {
    GpBitmap* bitmap = nullptr;
    if (GdipCreateBitmapFromFile(path, &bitmap) != 0 || !bitmap) return false;
    UINT sourceWidth = 0, sourceHeight = 0;
    if (GdipGetImageWidth((GpImage*)bitmap, &sourceWidth) != 0 ||
        GdipGetImageHeight((GpImage*)bitmap, &sourceHeight) != 0 ||
        sourceWidth == 0 || sourceHeight == 0) {
        GdipDisposeImage((GpImage*)bitmap);
        return false;
    }

    GpRect rect{};
    rect.Width = (INT)sourceWidth;
    rect.Height = (INT)sourceHeight;
    BitmapData data{};
    if (GdipBitmapLockBits(bitmap, &rect, ImageLockModeRead, PixelFormat32bppPARGB, &data) != 0) {
        GdipDisposeImage((GpImage*)bitmap);
        return false;
    }

    int targetWidth = (int)(((long long)sourceWidth * scaleMilli + 500) / 1000);
    int targetHeight = (int)(((long long)sourceHeight * scaleMilli + 500) / 1000);
    if (targetWidth < 1) targetWidth = 1;
    if (targetHeight < 1) targetHeight = 1;
    SIZE_T bytes = (SIZE_T)targetWidth * (SIZE_T)targetHeight * 4;
    BYTE* target = (BYTE*)HeapAlloc(GetProcessHeap(), 0, bytes);
    if (!target) {
        GdipBitmapUnlockBits(bitmap, &data);
        GdipDisposeImage((GpImage*)bitmap);
        return false;
    }

    // Nearest-neighbor scaling is intentionally used here. Scaling happens only
    // when an avatar is loaded, and this simple path avoids a persistent graphics
    // framework or additional resampling dependency.
    for (int y = 0; y < targetHeight; ++y) {
        int sourceY = (int)(((long long)y * sourceHeight) / targetHeight);
        BYTE* sourceRow;
        if (data.Stride >= 0) sourceRow = (BYTE*)data.Scan0 + (long long)sourceY * data.Stride;
        else sourceRow = (BYTE*)data.Scan0 + (long long)(sourceHeight - 1 - sourceY) * (-(long long)data.Stride);
        BYTE* targetRow = target + (SIZE_T)y * targetWidth * 4;
        for (int x = 0; x < targetWidth; ++x) {
            int sourceX = (int)(((long long)x * sourceWidth) / targetWidth);
            BYTE* sourcePixel = sourceRow + sourceX * 4;
            BYTE* targetPixel = targetRow + x * 4;
            targetPixel[0] = sourcePixel[0];
            targetPixel[1] = sourcePixel[1];
            targetPixel[2] = sourcePixel[2];
            targetPixel[3] = sourcePixel[3];
        }
    }

    GdipBitmapUnlockBits(bitmap, &data);
    GdipDisposeImage((GpImage*)bitmap);
    output.pixels = target;
    output.width = targetWidth;
    output.height = targetHeight;
    output.loaded = true;
    return true;
}

void buildAvatarPath(wchar_t* output, const wchar_t* avatarBase, int direction, bool talking) {
    wcopy(output, kPathCapacity, avatarBase);
    int length = wlen(output);
    if (length > 0 && output[length - 1] != L'\\' && output[length - 1] != L'/') wappend(output, kPathCapacity, L"\\");
    wappend(output, kPathCapacity, kDirectionNames[direction]);
    if (talking) wappend(output, kPathCapacity, L"_talking");
    wappend(output, kPathCapacity, L".png");
}

bool loadAvatar(const AppConfig& config, AvatarSet& avatar) {
    memset(&avatar, 0, sizeof(avatar));
    wchar_t avatarBase[kPathCapacity];
    joinPath(avatarBase, kPathCapacity, gExeDirectory, config.avatarDir);

    wchar_t centerPath[kPathCapacity];
    buildAvatarPath(centerPath, avatarBase, DIR_CENTER, false);
    if (!fileExists(centerPath)) {
        writeLine(L"Error: avatar folder is missing required center.png");
        writeText(L"Folder: "); writeLine(avatarBase);
        return false;
    }

    for (int direction = 0; direction < kDirectionCount; ++direction) {
        wchar_t path[kPathCapacity];
        buildAvatarPath(path, avatarBase, direction, false);
        if (fileExists(path) && loadPngScaled(path, config.scaleMilli, avatar.idle[direction])) {
            if (avatar.idle[direction].width > avatar.maxWidth) avatar.maxWidth = avatar.idle[direction].width;
            if (avatar.idle[direction].height > avatar.maxHeight) avatar.maxHeight = avatar.idle[direction].height;
        }
        buildAvatarPath(path, avatarBase, direction, true);
        if (fileExists(path) && loadPngScaled(path, config.scaleMilli, avatar.talking[direction])) {
            if (avatar.talking[direction].width > avatar.maxWidth) avatar.maxWidth = avatar.talking[direction].width;
            if (avatar.talking[direction].height > avatar.maxHeight) avatar.maxHeight = avatar.talking[direction].height;
        }
    }
    if (!avatar.idle[DIR_CENTER].loaded) {
        freeAvatar(avatar);
        writeLine(L"Error: center.png could not be decoded.");
        return false;
    }
    return true;
}

const ImageBitmap* imageFor(const AvatarSet& avatar, int direction, bool talking) {
    if (talking && avatar.talking[direction].loaded) return &avatar.talking[direction];
    if (avatar.idle[direction].loaded) return &avatar.idle[direction];
    if (talking && avatar.talking[DIR_CENTER].loaded) return &avatar.talking[DIR_CENTER];
    return &avatar.idle[DIR_CENTER];
}

// -------------------------------------------------------------------------
// Direction calculation
// -------------------------------------------------------------------------

int determineDirection(int mouseX, int mouseY, int screenWidth, int screenHeight, int deadzoneMilli, DirectionMode mode) {
    int dx = mouseX - screenWidth / 2;
    int dy = mouseY - screenHeight / 2;
    int absX = dx < 0 ? -dx : dx;
    int absY = dy < 0 ? -dy : dy;

    if (mode == MODE_HORIZONTAL) {
        int dead = ((screenWidth / 2) * deadzoneMilli) / 1000;
        if (absX <= dead) return DIR_CENTER;
        return dx < 0 ? DIR_LEFT : DIR_RIGHT;
    }

    int minDimension = screenWidth < screenHeight ? screenWidth : screenHeight;
    long long deadRadius = ((long long)(minDimension / 2) * deadzoneMilli) / 1000;
    long long distanceSquared = (long long)dx * dx + (long long)dy * dy;
    if (distanceSquared <= deadRadius * deadRadius) return DIR_CENTER;

    if (mode == MODE_FOURWAY) {
        if (absX >= absY) return dx < 0 ? DIR_LEFT : DIR_RIGHT;
        return dy < 0 ? DIR_UP : DIR_DOWN;
    }

    if (mode == MODE_QUADRANTS) {
        if (dx < 0 && dy < 0) return DIR_UP_LEFT;
        if (dx >= 0 && dy < 0) return DIR_UP_RIGHT;
        if (dx < 0 && dy >= 0) return DIR_DOWN_LEFT;
        return DIR_DOWN_RIGHT;
    }

    // Eight-way sectors without atan2(): tan(22.5 degrees) ~= 0.414 and
    // tan(67.5 degrees) ~= 2.414. Integer comparisons avoid a math runtime.
    if ((long long)absY * 1000000 <= (long long)absX * 414214) return dx < 0 ? DIR_LEFT : DIR_RIGHT;
    if ((long long)absY * 1000000 >= (long long)absX * 2414214) return dy < 0 ? DIR_UP : DIR_DOWN;
    if (dx < 0 && dy < 0) return DIR_UP_LEFT;
    if (dx >= 0 && dy < 0) return DIR_UP_RIGHT;
    if (dx < 0 && dy >= 0) return DIR_DOWN_LEFT;
    return DIR_DOWN_RIGHT;
}
