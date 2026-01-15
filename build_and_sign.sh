#!/bin/bash

# Script pour compiler et signer un exécutable Windows
# Usage: ./build_and_sign.sh [fichier.c]
# Si aucun fichier n'est spécifié, utilise first.c par défaut

set -e  # Arrête le script en cas d'erreur

# Récupère le nom du fichier source (par défaut: first.c)
SOURCE_FILE="${1:-first.c}"

# Vérifie que le fichier source existe
if [ ! -f "$SOURCE_FILE" ]; then
    echo "Erreur: Le fichier '$SOURCE_FILE' n'existe pas"
    exit 1
fi

# Extrait le nom de base sans extension
BASENAME=$(basename "$SOURCE_FILE" .c)

# Définit les noms des fichiers de sortie
OUTPUT_EXE="${BASENAME}.exe"
SIGNED_EXE="${BASENAME}_signed.exe"

echo "Compilation de '$SOURCE_FILE'..."
echo "Nettoyage des anciens fichiers..."
rm -f "$OUTPUT_EXE" "$SIGNED_EXE"

echo "Compilation avec mingw32..."

# Detect if source file needs system_info.c (check if it includes system_info.h)
ADDITIONAL_SOURCES=""
SOURCE_DIR=$(dirname "$SOURCE_FILE")

if grep -q "#include \"system_info.h\"" "$SOURCE_FILE" 2>/dev/null; then
    SYSTEM_INFO_C="${SOURCE_DIR}/system_info.c"
    if [ -f "$SYSTEM_INFO_C" ]; then
        echo "Détection de system_info.h - ajout de system_info.c à la compilation"
        ADDITIONAL_SOURCES="$SYSTEM_INFO_C"
    fi
fi

x86_64-w64-mingw32-gcc "$SOURCE_FILE" $ADDITIONAL_SOURCES -o "$OUTPUT_EXE" -s -liphlpapi -lws2_32

echo "Signature du binaire..."
osslsigncode sign -pkcs12 MalwrCert.pfx -n "Mon Malware Epitech" -i http://www.epitech.eu -t http://timestamp.digicert.com -in "$OUTPUT_EXE" -out "$SIGNED_EXE"

echo "✓ Compilation et signature terminées avec succès!"
echo "  - Exécutable: $OUTPUT_EXE"
echo "  - Signé: $SIGNED_EXE"
