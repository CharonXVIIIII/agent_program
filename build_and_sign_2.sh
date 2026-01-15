#!/bin/bash
set -e

SOURCE_FILE="${1:-first.c}"
BASENAME=$(basename "$SOURCE_FILE" .c)

OBF_C="${BASENAME}_obf.c"
OUTPUT_EXE="${BASENAME}.exe"
SIGNED_EXE="${BASENAME}_signed.exe"

echo "[*] Nettoyage..."
rm -f "$OBF_C" "$OUTPUT_EXE" "$SIGNED_EXE"

echo "[*] Obfuscation Tigress (fonctions ciblées)..."

tigress \
  --Environment=x86_64:Windows:Gcc \
  --Transform=Flatten \
  --Functions=sandbox_checks,decrypt \
  --out="$OBF_C" \
  "$SOURCE_FILE"

echo "[*] Compilation MinGW (release)..."

x86_64-w64-mingw32-gcc \
  "$OBF_C" \
  -O2 \
  -DNDEBUG \
  -fno-ident \
  -fno-asynchronous-unwind-tables \
  -s \
  -o "$OUTPUT_EXE"

echo "[*] Signature Authenticode..."

osslsigncode sign \
  -pkcs12 MalwrCert.pfx \
  -n "Projet Sécurité – Epitech" \
  -i http://www.epitech.eu \
  -t http://timestamp.digicert.com \
  -in "$OUTPUT_EXE" \
  -out "$SIGNED_EXE"

echo "[✓] Build terminé"
echo "    - EXE     : $OUTPUT_EXE"
echo "    - Signé   : $SIGNED_EXE"
