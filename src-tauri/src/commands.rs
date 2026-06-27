use std::sync::Mutex;
use tauri::State;
use serde::{Deserialize, Serialize};

use crate::ssh::SshSession;
use crate::vault::Vault;
use crate::serial::SerialSession;
use crate::sftp;

// ── App State ───────────────────────────────────────────────────────────

pub struct AppState {
    pub ssh_sessions: Mutex<Vec<SshSession>>,
    pub vault: Mutex<Option<Vault>>,
    pub serial_sessions: Mutex<Vec<SerialSession>>,
}

impl AppState {
    pub fn new() -> Self {
        Self {
            ssh_sessions: Mutex::new(Vec::new()),
            vault: Mutex::new(None),
            serial_sessions: Mutex::new(Vec::new()),
        }
    }
}

// ── Types ───────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ConnectionInfo {
    pub name: String,
    pub host: String,
    pub port: u16,
    pub username: String,
    pub auth_method: String,
    pub identity_file: Option<String>,
    pub proxy_host: Option<String>,
    pub proxy_port: Option<u16>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct VaultEntry {
    pub id: String,
    pub service: String,
    pub username: String,
    pub password: String,
}

// ── SSH Commands ────────────────────────────────────────────────────────

#[tauri::command]
pub async fn ssh_connect(
    state: State<'_, AppState>,
    session_id: String,
    conn: ConnectionInfo,
    password: Option<String>,
    event_window: tauri::Window,
) -> Result<(), String> {
    let mut ssh = SshSession::new(session_id.clone());
    ssh.connect(&conn, password, event_window.clone());

    let mut sessions = state.ssh_sessions.lock().map_err(|e| e.to_string())?;
    sessions.push(ssh);
    Ok(())
}

#[tauri::command]
pub async fn ssh_write(
    state: State<'_, AppState>,
    session_id: String,
    data: Vec<u8>,
) -> Result<(), String> {
    let sessions = state.ssh_sessions.lock().map_err(|e| e.to_string())?;
    if let Some(s) = sessions.iter().find(|s| s.id == session_id) {
        s.write(&data).map_err(|e| e.to_string())
    } else {
        Err("Session not found".into())
    }
}

#[tauri::command]
pub async fn ssh_resize(
    state: State<'_, AppState>,
    session_id: String,
    cols: u32,
    rows: u32,
) -> Result<(), String> {
    let sessions = state.ssh_sessions.lock().map_err(|e| e.to_string())?;
    if let Some(s) = sessions.iter().find(|s| s.id == session_id) {
        s.resize(cols, rows).map_err(|e| e.to_string())
    } else {
        Err("Session not found".into())
    }
}

#[tauri::command]
pub async fn ssh_disconnect(
    state: State<'_, AppState>,
    session_id: String,
) -> Result<(), String> {
    let mut sessions = state.ssh_sessions.lock().map_err(|e| e.to_string())?;
    sessions.retain(|s| s.id != session_id);
    Ok(())
}

// ── Vault Commands ──────────────────────────────────────────────────────

#[tauri::command]
pub async fn vault_create(
    state: State<'_, AppState>,
    password: String,
) -> Result<(), String> {
    let v = Vault::new(&password).map_err(|e| e.to_string())?;
    v.save().map_err(|e| e.to_string())?;
    let mut vault = state.vault.lock().map_err(|e| e.to_string())?;
    *vault = Some(v);
    Ok(())
}

#[tauri::command]
pub async fn vault_unlock(
    state: State<'_, AppState>,
    password: String,
) -> Result<(), String> {
    let v = Vault::load(&password).map_err(|e| e.to_string())?;
    let mut vault = state.vault.lock().map_err(|e| e.to_string())?;
    *vault = Some(v);
    Ok(())
}

#[tauri::command]
pub async fn vault_lock(state: State<'_, AppState>) -> Result<(), String> {
    let mut vault = state.vault.lock().map_err(|e| e.to_string())?;
    *vault = None;
    Ok(())
}

#[tauri::command]
pub async fn vault_list(state: State<'_, AppState>) -> Result<Vec<VaultEntry>, String> {
    let vault = state.vault.lock().map_err(|e| e.to_string())?;
    match vault.as_ref() {
        Some(v) => Ok(v.entries.clone()),
        None => Err("Vault is locked".into()),
    }
}

#[tauri::command]
pub async fn vault_set(
    state: State<'_, AppState>,
    entry: VaultEntry,
) -> Result<(), String> {
    let mut vault = state.vault.lock().map_err(|e| e.to_string())?;
    match vault.as_mut() {
        Some(v) => {
            v.upsert(entry);
            v.save().map_err(|e| e.to_string())
        }
        None => Err("Vault is locked".into()),
    }
}

#[tauri::command]
pub async fn vault_remove(
    state: State<'_, AppState>,
    id: String,
) -> Result<(), String> {
    let mut vault = state.vault.lock().map_err(|e| e.to_string())?;
    match vault.as_mut() {
        Some(v) => {
            v.entries.retain(|e| e.id != id);
            v.save().map_err(|e| e.to_string())
        }
        None => Err("Vault is locked".into()),
    }
}

// ── Serial Commands ─────────────────────────────────────────────────────

#[tauri::command]
pub async fn serial_list_ports() -> Result<Vec<String>, String> {
    crate::serial::list_ports().map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn serial_connect(
    state: State<'_, AppState>,
    port_name: String,
    baud_rate: u32,
    event_window: tauri::Window,
) -> Result<String, String> {
    let session_id = uuid::Uuid::new_v4().to_string();
    let mut sessions = state.serial_sessions.lock().map_err(|e| e.to_string())?;

    let mut serial = SerialSession::new(session_id.clone(), event_window.clone());
    serial.connect(&port_name, baud_rate)?;

    sessions.push(serial);
    Ok(session_id)
}

#[tauri::command]
pub async fn serial_write(
    state: State<'_, AppState>,
    session_id: String,
    data: Vec<u8>,
) -> Result<(), String> {
    let sessions = state.serial_sessions.lock().map_err(|e| e.to_string())?;
    if let Some(s) = sessions.iter().find(|s| s.id == session_id) {
        s.write(&data).map_err(|e| e.to_string())
    } else {
        Err("Session not found".into())
    }
}

#[tauri::command]
pub async fn serial_disconnect(
    state: State<'_, AppState>,
    session_id: String,
) -> Result<(), String> {
    let mut sessions = state.serial_sessions.lock().map_err(|e| e.to_string())?;
    sessions.retain(|s| s.id != session_id);
    Ok(())
}

// ── Connection persistence ──────────────────────────────────────────────

#[tauri::command]
pub async fn save_connections(data: String) -> Result<(), String> {
    let path = connections_path();
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent).map_err(|e| e.to_string())?;
    }
    std::fs::write(&path, &data).map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn load_connections() -> Result<String, String> {
    let path = connections_path();
    if !path.exists() {
        return Ok("[]".into());
    }
    std::fs::read_to_string(&path).map_err(|e| e.to_string())
}

fn connections_path() -> std::path::PathBuf {
    let mut path = data_dir_local();
    path.push("connections.json");
    path
}

// ── SFTP Commands ──────────────────────────────────────────────────────

#[tauri::command]
pub fn sftp_list_dir(
    conn: ConnectionInfo,
    password: String,
    path: String,
) -> Result<Vec<sftp::SftpEntry>, String> {
    sftp::list_dir(&conn, &password, &path)
}

// ── Helpers ─────────────────────────────────────────────────────────────

fn vault_exists() -> bool {
    crate::vault::vault_file_path().exists()
}

fn data_dir_local() -> std::path::PathBuf {
    #[cfg(target_os = "linux")]
    {
        std::env::var("XDG_DATA_HOME")
            .ok()
            .map(std::path::PathBuf::from)
            .unwrap_or_else(|| {
                std::env::var("HOME")
                    .map(|h| std::path::PathBuf::from(h).join(".local/share/SafeSSH"))
                    .unwrap_or_default()
            })
    }
    #[cfg(target_os = "macos")]
    {
        std::env::var("HOME")
            .map(|h| std::path::PathBuf::from(h).join("Library/Application Support/SafeSSH"))
            .unwrap_or_default()
    }
    #[cfg(target_os = "windows")]
    {
        std::env::var("APPDATA")
            .map(|a| std::path::PathBuf::from(a).join("SafeSSH"))
            .unwrap_or_default()
    }
    #[cfg(not(any(target_os = "linux", target_os = "macos", target_os = "windows")))]
    {
        std::env::var("HOME")
            .map(std::path::PathBuf::from)
            .unwrap_or_default()
    }
}
