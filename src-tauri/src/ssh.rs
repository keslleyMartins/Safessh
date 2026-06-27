use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::mpsc;
use std::thread;
use std::io::{Read, Write};
use ssh2::Session;
use tauri::Emitter;
use log::{info, error};

use crate::commands::ConnectionInfo;

enum SshCommand {
    Write(Vec<u8>),
    Resize(u32, u32),
    Disconnect,
}

pub struct SshSession {
    pub id: String,
    cmd_tx: Option<std::sync::mpsc::Sender<SshCommand>>,
    running: Arc<AtomicBool>,
}

impl SshSession {
    pub fn new(id: String) -> Self {
        Self {
            id,
            cmd_tx: None,
            running: Arc::new(AtomicBool::new(false)),
        }
    }

    pub async fn connect(&mut self, conn: &ConnectionInfo, password: Option<String>, window: tauri::Window) -> Result<(), String> {
        let session_id = self.id.clone();
        let running = self.running.clone();
        let (cmd_tx, cmd_rx) = std::sync::mpsc::channel::<SshCommand>();
        self.cmd_tx = Some(cmd_tx);
        running.store(true, Ordering::SeqCst);

        let host = conn.host.clone();
        let port = conn.port;
        let username = conn.username.clone();
        let auth_method = conn.auth_method.clone();
        let identity_file = conn.identity_file.clone();
        let password = password.clone();

        thread::spawn(move || {
            let addr = format!("{}:{}", host, port);

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "connecting");
            let tcp = match std::net::TcpStream::connect(&addr) {
                Ok(s) => s,
                Err(e) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Connection failed: {}", e));
                    return;
                }
            };

            let mut session = match Session::new() {
                Ok(s) => s,
                Err(e) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Session error: {}", e));
                    return;
                }
            };

            tcp.set_read_timeout(Some(std::time::Duration::from_secs(10))).ok();
            tcp.set_write_timeout(Some(std::time::Duration::from_secs(10))).ok();
            session.set_tcp_stream(tcp);
            session.set_blocking(true);

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "handshake");
            if let Err(e) = session.handshake() {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Handshake failed: {}", e));
                return;
            }

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "auth");
            let auth_result = match auth_method.as_str() {
                "agent" => session.userauth_agent(&username),
                "key" => {
                    let path = identity_file.unwrap_or_else(|| {
                        dirs::home_dir()
                            .map(|p| p.join(".ssh/id_rsa").to_string_lossy().to_string())
                            .unwrap_or_default()
                    });
                    session.userauth_pubkey_file(&username, None, std::path::Path::new(&path), password.as_deref())
                }
                _ => {
                    if let Some(pw) = &password {
                        session.userauth_password(&username, pw)
                    } else {
                        session.userauth_password(&username, "")
                    }
                }
            };

            if let Err(e) = auth_result {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Auth failed: {}", e));
                return;
            }

            let mut channel = match session.channel_session() {
                Ok(c) => c,
                Err(e) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Channel error: {}", e));
                    return;
                }
            };

            if let Err(e) = channel.request_pty("xterm-256color", None, Some((80, 24, 0, 0))) {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("PTY error: {}", e));
                return;
            }

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "shell");
            if let Err(e) = channel.shell() {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Shell error: {}", e));
                return;
            }

            session.set_blocking(false);
            let _ = window.emit(&format!("ssh-stage-{}", session_id), "connected");
            let _ = window.emit(&format!("ssh-connected-{}", session_id), ());

            let mut buf = [0u8; 32768];
            while running.load(Ordering::SeqCst) {
                // Read from channel
                match channel.read(&mut buf) {
                    Ok(0) => break,
                    Ok(n) => {
                        let data: Vec<u8> = buf[..n].to_vec();
                        let _ = window.emit(&format!("ssh-data-{}", session_id), data);
                    }
                    Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {}
                    Err(e) => {
                        error!("SSH read error: {}", e);
                        break;
                    }
                }

                // Stderr
                match channel.stderr().read(&mut buf) {
                    Ok(0) => break,
                    Ok(n) => {
                        let data: Vec<u8> = buf[..n].to_vec();
                        let _ = window.emit(&format!("ssh-data-{}", session_id), data);
                    }
                    Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {}
                    Err(_) => break,
                }

                // Process commands
                while let Ok(cmd) = cmd_rx.try_recv() {
                    match cmd {
                        SshCommand::Write(data) => {
                            if let Err(e) = channel.write(&data) {
                                error!("SSH write error: {}", e);
                            }
                            let _ = channel.flush();
                        }
                        SshCommand::Resize(cols, rows) => {
                            let _ = channel.request_pty_size(cols, rows, None, None);
                        }
                        SshCommand::Disconnect => {
                            running.store(false, Ordering::SeqCst);
                        }
                    }
                }

                thread::sleep(std::time::Duration::from_millis(10));
            }

            let _ = window.emit(&format!("ssh-disconnected-{}", session_id), ());
            let _ = channel.close();
            let _ = channel.wait_close();
        });

        Ok(())
    }

    pub fn write(&self, data: &[u8]) -> Result<(), String> {
        if let Some(tx) = &self.cmd_tx {
            tx.send(SshCommand::Write(data.to_vec())).map_err(|e| e.to_string())
        } else {
            Err("Not connected".into())
        }
    }

    pub fn resize(&self, cols: u32, rows: u32) -> Result<(), String> {
        if let Some(tx) = &self.cmd_tx {
            tx.send(SshCommand::Resize(cols, rows)).map_err(|e| e.to_string())
        } else {
            Err("Not connected".into())
        }
    }
}

impl Drop for SshSession {
    fn drop(&mut self) {
        if let Some(tx) = &self.cmd_tx {
            let _ = tx.send(SshCommand::Disconnect);
        }
    }
}
