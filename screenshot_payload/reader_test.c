// ============================================================================
// READER TEST - Screenshot Shared Memory
// ============================================================================
// Lance screenshot_shmem.exe, puis ce reader_test.exe pour vérifier
// que le payload signale bien les screenshots via la shared memory.
// ============================================================================

#include <windows.h>
#include <stdio.h>

#define SHARED_MEMORY_NAME "Local\\ScreenshotSharedMem"

typedef struct {
    volatile LONG has_new_data;
    volatile LONG is_active;
    volatile LONG screenshot_size;
    char screenshot_path[MAX_PATH];
} SharedScreenshotData;

int main(void) {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    printf("========================================\n");
    printf("  Screenshot Shared Memory Reader Test\n");
    printf("========================================\n");
    printf("[*] Attente de la shared memory: %s\n\n", SHARED_MEMORY_NAME);

    // Attendre que le payload démarre
    HANDLE hMap = NULL;
    while (hMap == NULL) {
        hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEMORY_NAME);
        if (hMap == NULL) {
            printf("[~] Payload pas encore démarré, attente...\n");
            Sleep(1000);
        }
    }
    printf("[+] Shared memory trouvée!\n\n");

    SharedScreenshotData* pData = (SharedScreenshotData*)MapViewOfFile(
        hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedScreenshotData));
    if (!pData) {
        printf("[ERROR] MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(hMap);
        getchar();
        return -1;
    }

    printf("[+] Payload actif: %ld\n", pData->is_active);
    printf("[+] Chemin screenshot: %s\n\n", pData->screenshot_path);

    int screenshots_received = 0;

    while (1) {
        if (pData->has_new_data == 1) {
            screenshots_received++;
            printf("[+] === Screenshot #%d reçu ===\n", screenshots_received);
            printf("    Taille déclarée: %ld bytes\n", pData->screenshot_size);
            printf("    Chemin: %s\n", pData->screenshot_path);

            // Vérifier le fichier
            HANDLE hFile = CreateFileA(pData->screenshot_path, GENERIC_READ,
                                       FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                DWORD fileSize = GetFileSize(hFile, NULL);
                printf("    Taille réelle fichier: %lu bytes\n", fileSize);

                // Lire les premiers bytes (header BMP)
                unsigned char header[6] = {0};
                DWORD read;
                ReadFile(hFile, header, sizeof(header), &read, NULL);
                CloseHandle(hFile);

                // Vérifier signature BMP "BM"
                if (header[0] == 'B' && header[1] == 'M') {
                    printf("    Format BMP: OK (signature 'BM' valide)\n");
                } else {
                    printf("    [WARNING] Signature inattendue: %02X %02X\n", header[0], header[1]);
                }
                printf("    Fichier accessible et valide!\n");
            } else {
                printf("    [ERROR] Impossible d'ouvrir le fichier: %lu\n", GetLastError());
            }

            // Acquitter la lecture (remettre has_new_data à 0)
            InterlockedExchange(&pData->has_new_data, 0);
            printf("    [+] has_new_data remis à 0 (acquittement)\n\n");
        } else {
            printf("[~] En attente d'un nouveau screenshot... (is_active=%ld)\n",
                   pData->is_active);
        }

        if (!pData->is_active) {
            printf("[+] Payload arrêté, fin du test.\n");
            break;
        }

        Sleep(2000);
    }

    UnmapViewOfFile(pData);
    CloseHandle(hMap);

    printf("\n========================================\n");
    printf("  Total screenshots reçus: %d\n", screenshots_received);
    printf("========================================\n");
    printf("Appuyez sur Entrée pour quitter...\n");
    getchar();
    return 0;
}
