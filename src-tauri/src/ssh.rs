use std::sync::{Arc, Mutex};
use std::sync::atomic::{AtomicBool, Ordering};
use std::io::{Read, Write};
use std::sync::mpsc;
use ssh2::Session;
use tauri::Emitter;
use log::info;
use crate::commands::ConnectionInfo;

pub struct SshSession {
    pub id: String,
    running: Arc<AtomicBool>,
    cmd_tx: Option<mpsc::Sender<SshCommand>>,
}

enum SshCommand {
    Write(Vec<u8>),
    Resize(u32, u32),
}

impl SshSession {
    pub fn new(id: String) -> Self {
        Self { id, running: Arc::new(AtomicBool::new(false)), cmd_tx: None }
    }

    pub fn connect(
        &mut self,
        conn: &ConnectionInfo,
        password: Option<String>,
        window: tauri::Window,
    ) {
        let session_id = self.id.clone();
        let running = self.running.clone();
        let (cmd_tx, cmd_rx) = mpsc::channel::<SshCommand>();
        self.cmd_tx = Some(cmd_tx);
        running.store(true, Ordering::SeqCst);

        let host = conn.host.clone();
        let port = conn.port;
        let username = conn.username.clone();
        let password = password.unwrap_or_default();

        std::thread::spawn(move || {
            let _ = window.emit(&format!("ssh-stage-{}", session_id), "connecting");

            let addr = format!("{}:{}", host, port);
            let tcp = match std::net::TcpStream::connect(&addr) {
                Ok(s) => s,
                Err(e) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Connection failed: {}", e));
                    return;
                }
            };
            tcp.set_read_timeout(Some(std::time::Duration::from_millis(100))).ok();

            let mut session = match Session::new() {
                Ok(s) => s,
                Err(e) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Session error: {}", e));
                    return;
                }
            };

            session.set_tcp_stream(tcp);
            session.set_blocking(true);

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "handshake");
            if let Err(e) = session.handshake() {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Handshake failed: {}", e));
                return;
            }

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "auth");
            if let Err(e) = session.userauth_password(&username, &password) {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Auth failed: {}", e));
                return;
            }

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "shell");
            let mut channel = match session.channel_session() {
                Ok(c) => c,
                Err(e) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Channel: {}", e));
                    return;
                }
            };

            if let Err(e) = channel.request_pty("xterm-256color", None, None) {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("PTY: {}", e));
                return;
            }

            if let Err(e) = channel.shell() {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Shell: {}", e));
                return;
            }

            session.set_blocking(false);
            let _ = window.emit(&format!("ssh-stage-{}", session_id), "connected");

            let mut buf = [0u8; 32768];
            while running.load(Ordering::SeqCst) {
                // Read stdout
                match channel.read(&mut buf) {
                    Ok(0) => break,
                    Ok(n) => {
                        let bytes: Vec<u8> = buf[..n].to_vec();
                        let _ = window.emit(&format!("ssh-data-{}", session_id), bytes);
                    }
                    Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {}
                    Err(_) => break,
                }

                // Read stderr
                match channel.stderr().read(&mut buf) {
                    Ok(0) => break,
                    Ok(n) => {
                        let bytes: Vec<u8> = buf[..n].to_vec();
                        let _ = window.emit(&format!("ssh-data-{}", session_id), bytes);
                    }
                    Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {}
                    Err(_) => break,
                }

                // Process write commands
                while let Ok(cmd) = cmd_rx.try_recv() {
                    match cmd {
                        SshCommand::Write(data) => {
                            let _ = channel.write(&data);
                            let _ = channel.flush();
                        }
                        SshCommand::Resize(cols, rows) => {
                            let _ = channel.request_pty_size(cols, rows, None, None);
                        }
                    }
                }

                std::thread::sleep(std::time::Duration::from_millis(10));
            }

            let _ = window.emit(&format!("ssh-disconnected-{}", session_id), ());
        });
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
            Ok(())
        }
    }

    pub fn disconnect(&mut self) {
        self.running.store(false, Ordering::SeqCst);
    }
}

impl Drop for SshSession {
    fn drop(&mut self) {
        self.disconnect();
    }
}
