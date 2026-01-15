# Documentation - Obfuscation des API Windows dans system_info.c

## Vue d'ensemble

Ce document décrit les modifications apportées à `system_info.c` pour obfusquer les appels aux API Windows en utilisant un chiffrement XOR, similaire à la technique utilisée dans `injector_v4.c`.

## Objectif

L'obfuscation des appels API Windows permet de :
1. **Éviter la détection statique** : Les antivirus analysent les imports et les chaînes de caractères pour détecter les comportements suspects
2. **Masquer les intentions** : Les noms de fonctions API ne sont plus visibles en clair dans le binaire
3. **Résolution dynamique** : Les fonctions sont chargées au runtime via `GetProcAddress`

## Technique utilisée : Chiffrement XOR

### Clé de chiffrement
- **Clé XOR** : `0x35`
- Chaque caractère des noms d'API est XORé avec cette clé

### Exemple
```
"GetSystemInfo" XOR 0x35 = {0x72, 0x50, 0x41, 0x66, 0x4c, 0x46, 0x41, 0x50, 0x58, 0x7c, 0x5b, 0x53, 0x5a, 0x35}
```

## Fonctions API obfusquées

### DLLs chargées
1. **kernel32.dll** - Fonctions système de base
2. **advapi32.dll** - Fonctions de sécurité et registre
3. **iphlpapi.dll** - Fonctions réseau

### Liste des API obfusquées (17 fonctions)

#### Kernel32.dll (10 fonctions)
- `GetVersionEx` - Information sur la version de Windows
- `GetSystemInfo` - Informations système (CPU, architecture)
- `GlobalMemoryStatusEx` - Statistiques mémoire
- `GetLogicalDrives` - Liste des lecteurs
- `GetDriveType` - Type de lecteur (fixe, amovible)
- `GetDiskFreeSpaceEx` - Espace disque disponible
- `GetVolumeInformation` - Informations sur le volume
- `GetComputerName` - Nom de l'ordinateur
- `GetUserName` - Nom de l'utilisateur
- `GetTickCount64` - Temps écoulé depuis le démarrage

#### Advapi32.dll (6 fonctions)
- `RegOpenKeyEx` - Ouvrir une clé de registre
- `RegQueryValueEx` - Lire une valeur de registre
- `RegCloseKey` - Fermer une clé de registre
- `AllocateAndInitializeSid` - Créer un SID
- `CheckTokenMembership` - Vérifier appartenance à un groupe
- `FreeSid` - Libérer un SID

#### Iphlpapi.dll (1 fonction)
- `GetAdaptersInfo` - Informations sur les adaptateurs réseau

## Architecture du code

### 1. Tableaux de bytes chiffrés
```c
// GetSystemInfo
unsigned char encryptedGetSystemInfo[] = {
    0x72, 0x50, 0x41, 0x66, 0x4c, 0x46, 0x41, 0x50, 0x58, 0x7c, 0x5b, 0x53, 0x5a, 0x35
};
```

### 2. Typedefs pour les pointeurs de fonctions
```c
typedef void (WINAPI *pGetSystemInfo)(LPSYSTEM_INFO);
pGetSystemInfo myGetSystemInfo = NULL;
```

### 3. Fonction d'initialisation
La fonction `initialize_api_functions()` effectue :
1. **Déchiffrement** des noms de DLLs et fonctions
2. **Chargement** des DLLs avec `GetModuleHandleA` / `LoadLibraryA`
3. **Résolution** des adresses avec `GetProcAddress`
4. **Vérification** que toutes les fonctions sont chargées

### 4. Utilisation dans le code
```c
// Au lieu de :
GetSystemInfo(&sysInfo);

// On utilise :
myGetSystemInfo(&sysInfo);
```

## Modifications apportées

### Fichiers modifiés
- `system_info.c` - Version obfusquée (active)
- `system_info_backup.c` - Version originale (sauvegarde)
- `system_info_obfuscated.c` - Copie de la version obfusquée

### Changements clés
1. Ajout de 17 tableaux de bytes chiffrés pour les noms d'API
2. Ajout de 17 typedefs pour les pointeurs de fonctions
3. Ajout de 17 variables globales pour stocker les pointeurs
4. Ajout de la fonction `decrypt_sysinfo()` (static pour éviter les conflits)
5. Ajout de la fonction `initialize_api_functions()`
6. Remplacement de tous les appels API directs par les pointeurs

### Gestion des conflits
Pour éviter les conflits de symboles avec `injector_v4.c` :
- Fonction `decrypt` renommée en `decrypt_sysinfo` (static)
- Variable `encryptedKernel32` renommée en `encryptedKernel32_sys` (static)
- Toutes les autres variables encrypted* sont `static`

## Script de génération

Un script Python `encrypt_api_names.py` permet de générer les tableaux chiffrés :
```python
def xor_encrypt(text, key=0x35):
    encrypted = [ord(c) ^ key for c in text]
    return encrypted
```

## Compilation

### Commande
```bash
./build_and_sign.sh injector_v4/injector_v4.c
```

### Résultats
- `injector_v4.exe` (58K) - Binaire compilé
- `injector_v4_signed.exe` (66K) - Binaire signé

## Avantages de l'obfuscation

### 1. Détection statique réduite
Les scanners antivirus ne peuvent plus :
- Voir les imports API suspects dans la table d'imports
- Détecter les chaînes de caractères d'API dans le binaire

### 2. Analyse dynamique nécessaire
Un analyste doit :
- Exécuter le programme
- Débugger pour voir les appels API résolus dynamiquement
- Déchiffrer les tableaux de bytes

### 3. Empreinte modifiée
- Chaque compilation avec une clé différente génère un hash différent
- Rend la détection par signature plus difficile

## Limites

### Ce que l'obfuscation ne cache PAS :
1. **Le comportement** - Les appels API sont toujours effectués
2. **Les heuristiques** - Un AV peut détecter le pattern de résolution dynamique
3. **L'analyse dynamique** - Un sandbox peut toujours voir les appels
4. **Le déchiffrement** - La fonction decrypt est visible et simple

### Améliorations possibles :
1. **Clé dynamique** - Générer la clé à partir d'un calcul
2. **Chiffrement plus complexe** - AES, RC4, etc.
3. **Polymorphisme** - Changer la clé et les patterns à chaque compilation
4. **Packing** - Compresser/chiffrer le binaire entier
5. **Anti-debugging** - Détecter les debuggers

## Utilisation éducative

Ce code est développé dans un contexte **strictement éducatif** :
- Projet encadré par Epitech
- Environnement de lab contrôlé
- Objectif : comprendre les techniques d'évasion pour mieux s'en défendre

## Références

### Techniques similaires
- **API hashing** - Utiliser des hashs au lieu de noms
- **Heaven's Gate** - Passer de x86 à x64 pour éviter les hooks
- **Direct syscalls** - Appeler directement le kernel sans passer par les API

### Documentation
- Microsoft Windows API Documentation
- Malware Analysis Techniques
- PE Format and Import Address Table (IAT)
