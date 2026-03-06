# Agent T-VIR - Documentation

> **Projet educatif - Epitech T-VIR**

## Table des matieres

| Document | Description |
|---|---|
| [agent.md](agent.md) | Agent principal (injector_v11) |
| [keylogger.md](keylogger.md) | Payload keylogger |
| [screenshot.md](screenshot.md) | Payload screenshot |

---

## Vue d'ensemble de l'architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         C2 SERVER                               │
│                  https://console.stock-s.fr                     │
│                                                                 │
│  POST /heartbeat/register   POST /heartbeat                     │
│  POST /api/agent/system-info  POST /keylogger  POST /screenshot │
└──────────────────────────┬──────────────────────────────────────┘
                           │ HTTPS (WinHTTP)
                           │
           ┌───────────────▼───────────────┐
           │        AGENT (injector_v11)    │
           │   Rapport_Confidentiel.pdf.exe │
           │                               │
           │  - Enregistrement C2          │
           │  - Heartbeat (jitter 1-10s)   │
           │  - Anti-sandbox               │
           │  - Injection de payloads      │
           └──────────┬────────────────────┘
                      │
          ┌───────────┴────────────┐
          │                        │
┌─────────▼──────────┐  ┌─────────▼──────────┐
│  KEYLOGGER PAYLOAD │  │ SCREENSHOT PAYLOAD  │
│  WindowsUpdate.exe │  │ MicrosoftEdge       │
│                    │  │ Update.exe          │
│  Shared Memory:    │  │                     │
│  KeyloggerSharedMem│  │  Shared Memory:     │
│                    │  │  ScreenshotSharedMem│
│  Mode: continu     │  │                     │
└────────────────────┘  │  Mode: one-shot     │
                        └─────────────────────┘
```

---

## Flux global

1. L'agent demarre, collecte les infos systeme et s'enregistre aupres du C2
2. Il envoie les informations d'architecture au C2
3. Il entre dans une boucle heartbeat (intervalle aleatoire 1-10 secondes)
4. A chaque heartbeat, le C2 peut repondre avec une tache :
   - `Inject` : deployer un payload (keylogger ou screenshot)
   - `Stop` : arreter le payload en cours
   - `Exit` : terminer l'agent
5. Si un payload est actif, l'agent collecte et envoie ses donnees au C2

---

## Compilation

```bash
# Agent principal
./build_and_sign.sh injector_v11/injector_v11.c

# Keylogger payload
cd keylogger_payload && bash build_shmem.sh

# Screenshot payload
./build_and_sign.sh screenshot_payload/screenshot_shmem.c
```

Le script `build_and_sign.sh` :
- Compile avec `x86_64-w64-mingw32-gcc`
- Signe le binaire avec `osslsigncode` (certificat `MalwrCert.pfx`)
- Copie le signe en `Rapport_Confidentiel.pdf.exe` (leurre PDF)

---

## Leurre social engineering

L'agent se presente comme un fichier PDF :
- Nom : `Rapport_Confidentiel.pdf.exe`
- Icone : PDF (embarquee via `resource.rc`)
- Signature Authenticode valide
- Fenetre console masquee (flag `-mwindows`)
