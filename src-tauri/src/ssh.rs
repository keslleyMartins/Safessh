use tauri::Emitter;
use russh::{self, client, keys, ChannelMsg};
use log::info;
use crate::commands::ConnectionInfo;

pub struct SshClient;

impl client::Handler for SshClient {
    type Error = anyhow::Error;

    async fn check_server_key(&mut self, _server_public_key: &keys::PublicKey) -> Result<bool, Self::Error> {
        Ok(true)
    }
}

pub struct SshSession {
    pub id: String,
    pub handle: Option<tokio::task::JoinHandle<()>>,
    pub write_tx: Option<tokio::sync::mpsc::UnboundedSender<Vec<u8>>>,
}

impl SshSession {
    pub fn new(id: String) -> Self {
        Self { id, handle: None, write_tx: None }
    }

    pub fn connect(
        &mut self,
        conn: &ConnectionInfo,
        password: Option<String>,
        window: tauri::Window,
    ) {
        let session_id = self.id.clone();
        let host = conn.host.clone();
        let port = conn.port;
        let username = conn.username.clone();
        let password = password.unwrap_or_default();

        let (write_tx, mut write_rx) = tokio::sync::mpsc::unbounded_channel::<Vec<u8>>();
        self.write_tx = Some(write_tx);

        let handle = tokio::spawn(async move {
            let _ = window.emit(&format!("ssh-stage-{}", session_id), "connecting");

            let addr: std::net::SocketAddr = match format!("{}:{}", host, port).parse() {
                Ok(a) => a,
                Err(_) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Invalid address: {}:{}", host, port));
                    return;
                }
            };

            let config: std::sync::Arc<client::Config> = client::Config::default().into();

            let mut session = match client::connect(config, addr, SshClient).await {
                Ok(s) => s,
                Err(e) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Connection failed: {}", e));
                    return;
                }
            };

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "auth");

            if let Err(e) = session.authenticate_password(&username, &password).await {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Auth failed: {}", e));
                return;
            }

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "shell");

            let mut channel = match session.channel_open_session().await {
                Ok(c) => c,
                Err(e) => {
                    let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Channel error: {}", e));
                    return;
                }
            };

            if let Err(e) = channel.request_pty(false, "xterm-256color", 80, 24, 0, 0, &[]).await {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("PTY error: {}", e));
                return;
            }

            if let Err(e) = channel.exec(false, "bash").await {
                let _ = window.emit(&format!("ssh-error-{}", session_id), format!("Shell error: {}", e));
                return;
            }

            let _ = window.emit(&format!("ssh-stage-{}", session_id), "connected");

            loop {
                tokio::select! {
                    msg = channel.wait() => {
                        match msg {
                            Some(ChannelMsg::Data { ref data }) => {
                                let bytes: Vec<u8> = data.as_ref().to_vec();
                                let _ = window.emit(&format!("ssh-data-{}", session_id), bytes);
                            }
                            Some(ChannelMsg::Eof) | None => break,
                            _ => {}
                        }
                    }
                    data = write_rx.recv() => {
                        match data {
                            Some(d) => {
                                if let Err(e) = channel.data(&d[..]).await {
                                    info!("SSH write error: {}", e);
                                    break;
                                }
                            }
                            None => break,
                        }
                    }
                }
            }

            let _ = channel.eof().await;
            let _ = window.emit(&format!("ssh-disconnected-{}", session_id), ());
        });

        self.handle = Some(handle);
    }

    pub fn write(&self, data: &[u8]) -> Result<(), String> {
        if let Some(tx) = &self.write_tx {
            tx.send(data.to_vec()).map_err(|e| e.to_string())
        } else {
            Err("Not connected".into())
        }
    }

    pub fn resize(&self, _cols: u32, _rows: u32) -> Result<(), String> {
        Ok(())
    }

    pub fn disconnect(&mut self) {
        if let Some(handle) = self.handle.take() {
            handle.abort();
        }
    }
}

impl Drop for SshSession {
    fn drop(&mut self) {
        self.disconnect();
    }
}
