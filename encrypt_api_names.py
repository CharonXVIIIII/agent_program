#!/usr/bin/env python3
"""
Script pour générer les chaînes API chiffrées avec XOR
Clé XOR: 0x35
"""

def xor_encrypt(text, key=0x35):
    """Chiffre une chaîne avec XOR"""
    encrypted = [ord(c) ^ key for c in text]
    return encrypted

def generate_c_array(name, text, key=0x35):
    """Génère un tableau C avec les bytes chiffrés"""
    encrypted = xor_encrypt(text + '\0', key)
    hex_values = ', '.join(f'0x{b:02x}' for b in encrypted)

    print(f"// {text}")
    print(f"unsigned char encrypted{name}[] = {{")
    print(f"    {hex_values}")
    print(f"}};")
    print()

# Liste des fonctions API Windows utilisées dans system_info.c
api_functions = [
    ("GetVersionEx", "GetVersionEx"),
    ("GetSystemInfo", "GetSystemInfo"),
    ("RegOpenKeyEx", "RegOpenKeyEx"),
    ("RegQueryValueEx", "RegQueryValueEx"),
    ("RegCloseKey", "RegCloseKey"),
    ("GlobalMemoryStatusEx", "GlobalMemoryStatusEx"),
    ("GetLogicalDrives", "GetLogicalDrives"),
    ("GetDriveType", "GetDriveType"),
    ("GetDiskFreeSpaceEx", "GetDiskFreeSpaceEx"),
    ("GetVolumeInformation", "GetVolumeInformation"),
    ("GetAdaptersInfo", "GetAdaptersInfo"),
    ("AllocateAndInitializeSid", "AllocateAndInitializeSid"),
    ("CheckTokenMembership", "CheckTokenMembership"),
    ("FreeSid", "FreeSid"),
    ("GetComputerName", "GetComputerName"),
    ("GetUserName", "GetUserName"),
    ("GetTickCount64", "GetTickCount64"),
    # DLLs
    ("Kernel32", "kernel32.dll"),
    ("Advapi32", "advapi32.dll"),
    ("Iphlpapi", "iphlpapi.dll"),
]

print("// ============================================================================")
print("// ENCRYPTED API STRINGS (XOR key: 0x35)")
print("// ============================================================================")
print()

for name, api_name in api_functions:
    generate_c_array(name, api_name)
