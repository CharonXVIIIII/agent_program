#!/usr/bin/env python3
# ============================================================================
# Script de conversion EXE vers bytes hexadécimaux
# Pour injection via le C2
# ============================================================================

import sys
import os

def exe_to_hex(exe_path, output_path=None):
    """
    Convertit un fichier EXE en chaîne hexadécimale

    Args:
        exe_path: Chemin vers le fichier .exe
        output_path: Chemin vers le fichier de sortie (optionnel)
    """

    if not os.path.exists(exe_path):
        print(f"[-] Erreur: Le fichier {exe_path} n'existe pas")
        return None

    print(f"[+] Lecture du fichier: {exe_path}")

    # Lire le fichier binaire
    with open(exe_path, 'rb') as f:
        data = f.read()

    file_size = len(data)
    print(f"[+] Taille du fichier: {file_size} bytes ({file_size / 1024:.2f} KB)")

    # Convertir en hexadécimal
    hex_string = data.hex().upper()

    print(f"[+] Longueur de la chaîne hex: {len(hex_string)} caractères")

    # Sauvegarder dans un fichier si spécifié
    if output_path:
        with open(output_path, 'w') as f:
            f.write(hex_string)
        print(f"[+] Sauvegardé dans: {output_path}")

    # Afficher un aperçu
    preview_length = 100
    print(f"\n[+] Aperçu (premiers {preview_length} caractères):")
    print(hex_string[:preview_length] + "...")

    # Format pour C2
    print("\n" + "="*70)
    print("[+] Format pour le C2:")
    print("="*70)
    print(f'payload: "{hex_string[:50]}...{hex_string[-50:]}"')
    print(f"\nLongueur totale: {len(hex_string)} caractères")

    return hex_string


def create_c_array(exe_path, output_path=None):
    """
    Convertit un fichier EXE en tableau C
    Utile pour tester l'injection localement

    Args:
        exe_path: Chemin vers le fichier .exe
        output_path: Chemin vers le fichier de sortie .h (optionnel)
    """

    if not os.path.exists(exe_path):
        print(f"[-] Erreur: Le fichier {exe_path} n'existe pas")
        return

    with open(exe_path, 'rb') as f:
        data = f.read()

    # Créer le tableau C
    c_array = "unsigned char payload[] = {\n    "

    for i, byte in enumerate(data):
        c_array += f"0x{byte:02X}"

        if i < len(data) - 1:
            c_array += ", "

        # Nouvelle ligne tous les 12 bytes
        if (i + 1) % 12 == 0 and i < len(data) - 1:
            c_array += "\n    "

    c_array += "\n};\n"
    c_array += f"size_t payload_size = {len(data)};\n"

    if output_path:
        with open(output_path, 'w') as f:
            f.write(c_array)
        print(f"[+] Tableau C sauvegardé dans: {output_path}")
    else:
        print(c_array[:500] + "...\n")


def main():
    print("="*70)
    print("  Convertisseur EXE vers Bytes Hexadécimaux")
    print("  EPITECH T-VIR CTF - Keylogger Payload")
    print("="*70)
    print()

    if len(sys.argv) < 2:
        print("Usage:")
        print("  python exe_to_bytes.py <fichier.exe> [output.txt]")
        print()
        print("Exemples:")
        print("  python exe_to_bytes.py keylogger.exe")
        print("  python exe_to_bytes.py keylogger.exe keylogger_hex.txt")
        print("  python exe_to_bytes.py keylogger.exe keylogger.h --c-array")
        return

    exe_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else None

    # Vérifier si on veut un tableau C
    if '--c-array' in sys.argv:
        create_c_array(exe_path, output_path)
    else:
        # Mode normal: conversion en hex
        if not output_path:
            output_path = exe_path.replace('.exe', '_hex.txt')

        hex_string = exe_to_hex(exe_path, output_path)

        if hex_string:
            print("\n[+] Conversion réussie!")
            print(f"[+] Pour envoyer au C2, utilisez le contenu de: {output_path}")


if __name__ == "__main__":
    main()
