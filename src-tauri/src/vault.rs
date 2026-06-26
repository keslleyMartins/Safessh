use aes_gcm::{Aes256Gcm, Key, Nonce};
use aes_gcm::aead::{Aead, KeyInit, OsRng};
use argon2::{Argon2, PasswordHash, PasswordHasher, PasswordVerifier};
use argon2::password_hash::SaltString;
use serde::{Deserialize, Serialize};
use base64::Engine;

use crate::commands::VaultEntry;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Vault {
    pub salt: String,
    pub entries_encrypted: String,
    #[serde(skip)]
    pub entries: Vec<VaultEntry>,
    #[serde(skip)]
    key: Option<Vec<u8>>,
}

impl Vault {
    pub fn new(master_password: &str) -> Result<Self, String> {
        let salt = SaltString::generate(&mut OsRng);
        let argon2 = Argon2::default();
        let hash = argon2
            .hash_password(master_password.as_bytes(), &salt)
            .map_err(|e| format!("Hash failed: {}", e))?;

        let key_bytes = hash.hash.unwrap().as_bytes().to_vec();
        let key = Key::<Aes256Gcm>::from_slice(&key_bytes[..32]);

        let mut nonce_bytes = [0u8; 12];
        rand::rngs::OsRng.fill_bytes(&mut nonce_bytes);
        let nonce = Nonce::from_slice(&nonce_bytes);

        let cipher = Aes256Gcm::new(key);
        let empty: Vec<u8> = vec![];
        let ciphertext = cipher
            .encrypt(nonce, empty.as_ref())
            .map_err(|e| format!("Encryption failed: {}", e))?;

        Ok(Self {
            salt: salt.to_string(),
            entries_encrypted: base64::engine::general_purpose::STANDARD
                .encode(&[&nonce_bytes[..], &ciphertext[..]].concat()),
            entries: Vec::new(),
            key: Some(key_bytes),
        })
    }

    pub fn load(master_password: &str) -> Result<Self, String> {
        let path = vault_file_path();
        let data = std::fs::read_to_string(&path)
            .map_err(|e| format!("Cannot read vault at {:?}: {}", path, e))?;
        let mut vault: Vault =
            serde_json::from_str(&data).map_err(|e| format!("Invalid vault: {}", e))?;

        let salt = SaltString::from_b64(&vault.salt)
            .map_err(|e| format!("Invalid salt: {}", e))?;
        let argon2 = Argon2::default();
        let hash = argon2
            .hash_password(master_password.as_bytes(), &salt)
            .map_err(|e| format!("Hash failed: {}", e))?;

        let key_bytes = hash.hash.unwrap().as_bytes().to_vec();
        let key = Key::<Aes256Gcm>::from_slice(&key_bytes[..32]);

        // Decrypt entries
        let raw = base64::engine::general_purpose::STANDARD
            .decode(&vault.entries_encrypted)
            .map_err(|e| format!("Invalid encoding: {}", e))?;

        if raw.len() < 12 {
            return Err("Invalid vault data".into());
        }

        let (nonce_bytes, ciphertext) = raw.split_at(12);
        let nonce = Nonce::from_slice(nonce_bytes);

        let cipher = Aes256Gcm::new(key);
        let plaintext = cipher
            .decrypt(nonce, ciphertext)
            .map_err(|_| "Wrong password".to_string())?;

        // If plaintext is not empty, parse as entries
        if !plaintext.is_empty() {
            vault.entries = serde_json::from_slice(&plaintext)
                .unwrap_or_default();
        }

        vault.key = Some(key_bytes);
        Ok(vault)
    }

    pub fn save(&self) -> Result<(), String> {
        let path = self.vault_path();

        let mut data = self.clone();
        data.key = None;

        if let Some(key_bytes) = &self.key {
            let key = Key::<Aes256Gcm>::from_slice(&key_bytes[..32]);

            let mut nonce_bytes = [0u8; 12];
            rand::rngs::OsRng.fill_bytes(&mut nonce_bytes);
            let nonce = Nonce::from_slice(&nonce_bytes);

            let plaintext = serde_json::to_vec(&self.entries)
                .map_err(|e| format!("Serialize error: {}", e))?;
            let cipher = Aes256Gcm::new(key);
            let ciphertext = cipher
                .encrypt(nonce, plaintext.as_ref())
                .map_err(|e| format!("Encrypt error: {}", e))?;

            data.entries_encrypted = base64::engine::general_purpose::STANDARD
                .encode(&[&nonce_bytes[..], &ciphertext[..]].concat());
        }

        if let Some(parent) = path.parent() {
            let _ = std::fs::create_dir_all(parent);
        }
        let json = serde_json::to_string_pretty(&data)
            .map_err(|e| format!("JSON error: {}", e))?;
        std::fs::write(&path, json).map_err(|e| format!("Write error: {}", e))
    }

    pub fn upsert(&mut self, entry: VaultEntry) {
        if let Some(pos) = self.entries.iter().position(|e| e.id == entry.id) {
            self.entries[pos] = entry;
        } else {
            self.entries.push(entry);
        }
    }

    fn vault_path(&self) -> std::path::PathBuf {
        vault_file_path()
    }
}

pub fn vault_file_path() -> std::path::PathBuf {
    let mut path = data_dir();
    let _ = std::fs::create_dir_all(&path);
    path.push("safessh_vault.dat");
    path
}

fn data_dir() -> std::path::PathBuf {
    #[cfg(target_os = "linux")]
    {
        std::env::var("XDG_DATA_HOME")
            .ok()
            .map(std::path::PathBuf::from)
            .unwrap_or_else(|| {
                std::env::var("HOME")
                    .map(|h| std::path::PathBuf::from(h).join(".local/share"))
                    .unwrap_or_default()
            })
    }
    #[cfg(target_os = "macos")]
    {
        std::env::var("HOME")
            .map(|h| std::path::PathBuf::from(h).join("Library/Application Support"))
            .unwrap_or_default()
    }
    #[cfg(target_os = "windows")]
    {
        std::env::var("APPDATA")
            .map(std::path::PathBuf::from)
            .unwrap_or_default()
    }
    #[cfg(not(any(target_os = "linux", target_os = "macos", target_os = "windows")))]
    {
        std::env::var("HOME").map(std::path::PathBuf::from).unwrap_or_default()
    }
}
