# Module de Détection d'Architecture et d'Information Système

## Description

Le module system_info.h permet de détecter l'architecture système et de collecter des informations détaillées sur la machine hôte. Il est compatible Windows et Linux.

## Fonctionnalités

### Détection d'architecture
- x86 (32-bit)
- x86_64 (64-bit)
- ARM (32-bit)
- ARM64 (64-bit)
- MIPS
- PowerPC

### Informations collectées

1. **Système**
   - Architecture du processeur
   - Système d'exploitation et version
   - Hostname
   - Nom d'utilisateur
   - Privilèges (admin/root)
   - Détection de machine virtuelle
   - Uptime

2. **CPU**
   - Modèle du processeur
   - Nombre de processeurs physiques
   - Nombre de cœurs logiques
   - Fréquence (Linux uniquement)

3. **Mémoire**
   - RAM totale
   - RAM utilisée
   - RAM disponible
   - Pourcentage d'utilisation

4. **Disques**
   - Liste des disques/partitions
   - Points de montage
   - Système de fichiers
   - Espace total/libre/disponible

5. **Réseau**
   - Interfaces réseau
   - Adresses IP
   - Adresses MAC
   - État (UP/DOWN)

## Compilation

### Linux
```bash
make
```

### Windows (MinGW)
```bash
make OS=Windows_NT
```

### Compilation manuelle Linux
```bash
gcc -Wall -Wextra -o test_system_info system_info.c test_system_info.c
```

### Compilation manuelle Windows
```bash
gcc -Wall -Wextra -o test_system_info.exe system_info.c test_system_info.c -liphlpapi -ladvapi32
```

## Utilisation

### Programme de test
```bash
./test_system_info
```

### Intégration dans votre code

```c
#include "system_info.h"

int main(void) {
    SystemInfo info;

    // Collecter les informations
    if (collect_system_info(&info) != 0) {
        fprintf(stderr, "Erreur lors de la collecte\n");
        return 1;
    }

    // Afficher les informations
    print_system_info(&info);

    // Ou sérialiser en JSON
    char json[8192];
    system_info_to_json(&info, json, sizeof(json));
    printf("%s\n", json);

    // Accéder aux informations individuelles
    printf("Architecture: %s\n", architecture_to_string(info.architecture));
    printf("RAM totale: %llu MB\n", info.total_ram_mb);
    printf("Nombre de CPUs: %d\n", info.nb_processors);

    return 0;
}
```

## Détection de Sandbox

Le module peut être utilisé pour détecter des environnements de sandbox :

```c
SystemInfo info;
collect_system_info(&info);

// Vérifier les indicateurs suspects
if (info.nb_processors < 2) {
    printf("Attention: Nombre de CPU suspect\n");
}

if (info.total_ram_mb < 2048) {
    printf("Attention: RAM insuffisante\n");
}

if (info.is_vm) {
    printf("Attention: Machine virtuelle détectée\n");
}
```

## Structure de données

```c
typedef struct {
    Architecture architecture;      // Architecture du CPU
    OperatingSystem os;            // Système d'exploitation
    char os_version[256];          // Version de l'OS
    char hostname[256];            // Nom de la machine

    int nb_processors;             // Nombre de processeurs
    int nb_logical_cores;          // Nombre de cœurs logiques
    char cpu_model[256];           // Modèle du CPU
    uint64_t cpu_frequency_mhz;    // Fréquence en MHz

    uint64_t total_ram_mb;         // RAM totale en MB
    uint64_t available_ram_mb;     // RAM disponible en MB
    uint64_t used_ram_mb;          // RAM utilisée en MB
    int ram_usage_percent;         // Pourcentage d'utilisation

    int nb_disks;                  // Nombre de disques
    DiskInfo disks[MAX_DISKS];     // Informations sur les disques

    int nb_interfaces;             // Nombre d'interfaces réseau
    NetworkInterface interfaces[MAX_INTERFACES];

    uint64_t uptime_seconds;       // Temps de fonctionnement
    char username[256];            // Nom d'utilisateur
    int is_admin;                  // Privilèges admin/root
    int is_vm;                     // Détection de VM
} SystemInfo;
```

## Compatibilité

- **Windows**: Windows 7 et supérieur
- **Linux**: Kernel 2.6+
- **Architecture**: x86, x86_64, ARM, ARM64

## Dépendances

### Windows
- iphlpapi.lib
- advapi32.lib

### Linux
- Aucune dépendance externe (utilise libc standard)

## Notes de sécurité

Ce module collecte des informations système sensibles. Utilisez-le uniquement dans un contexte autorisé :
- Tests de pénétration autorisés
- Recherche en sécurité
- Développement d'agents de monitoring légitimes
- Compétitions CTF

## Exemple de sortie

```
========================================
         SYSTEM INFORMATION
========================================

[+] SYSTEM
    Architecture:    x86_64 (64-bit)
    OS:              Linux
    Version:         Linux 6.14.0-37-generic
    Hostname:        my-computer
    Username:        odessa
    Admin/Root:      No
    Virtual Machine: No
    Uptime:          3600 seconds

[+] CPU
    Model:           Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz
    Processors:      12
    Logical Cores:   12
    Frequency:       2600 MHz

[+] MEMORY
    Total RAM:       16384 MB
    Used RAM:        8192 MB
    Available RAM:   8192 MB
    Usage:           50%

[+] DISKS (2 total)
    [0] /dev/sda1 (/)
        Filesystem:  ext4
        Total:       500.00 GB
        Free:        250.00 GB
        Available:   250.00 GB

[+] NETWORK INTERFACES (2 total)
    [0] eth0
        IP:          192.168.1.100
        MAC:         00:11:22:33:44:55
        Status:      UP
```

## Intégration avec injector_v3.c

Pour intégrer ce module avec votre injecteur existant, ajoutez simplement :

```c
#include "system_info.h"

int main(void) {
    // Collecter les informations système
    SystemInfo info;
    collect_system_info(&info);

    // Vérifications anti-sandbox améliorées
    if (info.nb_processors < TRHESHOLD_MAX_CPU) {
        DEBUG("Suspicious CPU count: %d\n", info.nb_processors);
        return 0;
    }

    if (info.total_ram_mb < TRHESHOLD_MIN_RAM_MB) {
        DEBUG("Suspicious RAM: %llu MB\n", info.total_ram_mb);
        return 0;
    }

    if (info.is_vm) {
        DEBUG("Virtual machine detected\n");
        return 0;
    }

}
```
