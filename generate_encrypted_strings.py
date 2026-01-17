#!/usr/bin/env python3
"""
Script pour générer des chaînes obfusquées avec XOR pour l'injector
"""

def xor_encrypt(string, key):
    """Chiffre une chaîne avec XOR"""
    return [ord(c) ^ key for c in string]

def generate_c_array(name, encrypted_bytes):
    """Génère le code C pour un tableau"""
    hex_values = ", ".join([f"0x{b:02x}" for b in encrypted_bytes])
    return f"unsigned char encrypted{name}[] = {{\n    {hex_values}\n}};"

KEY = 0x1b

apis = [
    "explorer.exe"
]

print("// Chaînes chiffrées pour les API Windows (clé XOR: 0x{:02X})".format(KEY))
print("// Généré automatiquement par generate_encrypted_strings.py\n")

for api in apis:
    encrypted = xor_encrypt(api + '\0', KEY)  # Ajoute le \0 de fin
    print(generate_c_array(api.replace('.', '_').replace('-', '_'), encrypted))
    print()

print("\n// Pour déchiffrer, utilisez la fonction decrypt() avec la clé 0x{:02X}".format(KEY))
