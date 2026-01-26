#!/bin/bash

set -e

SOURCE_FILE="${1:-first.c}"

if [ ! -f "$SOURCE_FILE" ]; then
    echo "Erreur: Le fichier '$SOURCE_FILE' n'existe pas"
    exit 1
fi

BASENAME=$(basename "$SOURCE_FILE" .c)
SOURCE_DIR=$(dirname "$SOURCE_FILE")

OUTPUT_EXE="${SOURCE_DIR}/${BASENAME}.exe"
SIGNED_EXE="${SOURCE_DIR}/${BASENAME}_signed.exe"

echo "Compilation de '$SOURCE_FILE'..."
echo "Nettoyage des anciens fichiers..."
rm -f "$OUTPUT_EXE" "$SIGNED_EXE"

echo "Compilation avec mingw32..."

ADDITIONAL_SOURCES=""

if grep -q "#include \"system_info.h\"" "$SOURCE_FILE" 2>/dev/null; then
    SYSTEM_INFO_C="${SOURCE_DIR}/system_info.c"
    if [ -f "$SYSTEM_INFO_C" ]; then
        echo "Détection de system_info.h - ajout de system_info.c à la compilation"
        ADDITIONAL_SOURCES="$SYSTEM_INFO_C"
    fi
fi

x86_64-w64-mingw32-gcc "$SOURCE_FILE" $ADDITIONAL_SOURCES -o "$OUTPUT_EXE" -s -liphlpapi -lws2_32 -lrpcrt4

echo "Signature du binaire..."
osslsigncode sign -pkcs12 MalwrCert.pfx -n "Mon Malware Epitech" -i http://www.epitech.eu -t http://timestamp.digicert.com -in "$OUTPUT_EXE" -out "$SIGNED_EXE"

echo "✓ Compilation et signature terminées avec succès!"
echo "  - Exécutable: $OUTPUT_EXE"
echo "  - Signé: $SIGNED_EXE"
