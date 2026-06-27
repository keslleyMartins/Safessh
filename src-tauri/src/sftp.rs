use serde::Serialize;
use russh::{self, client, ChannelMsg};

#[derive(Debug, Serialize)]
pub struct SftpEntry {
    pub name: String,
    pub is_dir: bool,
    pub size: i64,
}

struct SftpClient;

impl client::Handler for SftpClient {
    type Error = anyhow::Error;
    async fn check_server_key(&mut self, _: &russh::keys::PublicKey) -> Result<bool, Self::Error> {
        Ok(true)
    }
}

pub async fn list_dir(
    host: &str,
    port: u16,
    username: &str,
    password: &str,
    path: &str,
) -> Result<Vec<SftpEntry>, String> {
    let addr: std::net::SocketAddr = format!("{}:{}", host, port)
        .parse()
        .map_err(|_| format!("Invalid address: {}:{}", host, port))?;

    let config: std::sync::Arc<client::Config> = client::Config::default().into();

    let mut session = client::connect(config, addr, SftpClient)
        .await
        .map_err(|e| format!("SSH connect failed: {}", e))?;

    session
        .authenticate_password(username, password)
        .await
        .map_err(|e| format!("Auth failed: {}", e))?;

    let mut channel = session
        .channel_open_session()
        .await
        .map_err(|e| format!("Channel open failed: {}", e))?;

    // Request SFTP subsystem
    channel
        .request_subsystem(false, "sftp")
        .await
        .map_err(|e| format!("SFTP subsystem failed: {}", e))?;

    // SFTP protocol: send SSH_FXP_INIT (version 3)
    let mut req_id: u32 = 1;
    let init = build_packet(1, req_id, &3u32.to_be_bytes()); // SSH_FXP_INIT with version 3
    channel.data(&init[..]).await.map_err(|e| e.to_string())?;

    // Read SSH_FXP_VERSION response
    let ver = read_sftp_packet(&mut channel).await?;
    if ver[0] != 2 {
        return Err("Expected SSH_FXP_VERSION".into());
    }

    // Send SSH_FXP_OPENDIR
    req_id += 1;
    let path_bytes = path.as_bytes();
    let mut opendir = Vec::new();
    opendir.push(11u8); // SSH_FXP_OPENDIR
    opendir.extend(&req_id.to_be_bytes());
    opendir.extend(&(path_bytes.len() as u32).to_be_bytes());
    opendir.extend(path_bytes);
    let opendir_pkt = build_packet(0, 0, &opendir); // type=0 (unused), we already have full data
    channel.data(&opendir_pkt[..]).await.map_err(|e| e.to_string())?;

    // Read SSH_FXP_HANDLE
    let handle_resp = read_sftp_packet(&mut channel).await?;
    if handle_resp[0] != 102 {
        let status_code = u32::from_be_bytes([handle_resp[5], handle_resp[6], handle_resp[7], handle_resp[8]]);
        let err = match status_code {
            2 => "No such file",
            3 => "Permission denied",
            4 => "General failure",
            _ => "Unknown error",
        };
        return Err(err.into());
    }

    let handle_len = u32::from_be_bytes([handle_resp[5], handle_resp[6], handle_resp[7], handle_resp[8]]);
    let handle = &handle_resp[9..9 + handle_len as usize];

    // Send SSH_FXP_READDIR
    req_id += 1;
    let mut readdir = Vec::new();
    readdir.push(12u8); // SSH_FXP_READDIR
    readdir.extend(&req_id.to_be_bytes());
    readdir.extend(&(handle.len() as u32).to_be_bytes());
    readdir.extend(handle);
    let readdir_pkt = build_packet(0, 0, &readdir);
    channel.data(&readdir_pkt[..]).await.map_err(|e| e.to_string())?;

    // Read SSH_FXP_NAME response
    let mut entries = Vec::new();
    loop {
        let resp = read_sftp_packet(&mut channel).await?;
        match resp[0] {
            104 => {
                // SSH_FXP_NAME
                let count = u32::from_be_bytes([resp[5], resp[6], resp[7], resp[8]]);
                let mut offset = 9;
                for _ in 0..count {
                    let name_len = u32::from_be_bytes([
                        resp[offset], resp[offset + 1], resp[offset + 2], resp[offset + 3],
                    ]);
                    offset += 4;
                    let name = String::from_utf8_lossy(&resp[offset..offset + name_len as usize]).to_string();
                    offset += name_len as usize;

                    // Skip longname (legacy field)
                    let longname_len = u32::from_be_bytes([
                        resp[offset], resp[offset + 1], resp[offset + 2], resp[offset + 3],
                    ]);
                    offset += 4 + longname_len as usize;

                    // Parse attributes
                    let attrs_flags = u32::from_be_bytes([
                        resp[offset], resp[offset + 1], resp[offset + 2], resp[offset + 3],
                    ]);
                    offset += 4;

                    let mut is_dir = false;
                    let mut size: i64 = 0;

                    if attrs_flags & 0x00000004 != 0 {
                        // SSH_FILEXFER_ATTR_PERMISSIONS
                        let perms = u32::from_be_bytes([
                            resp[offset], resp[offset + 1], resp[offset + 2], resp[offset + 3],
                        ]);
                        offset += 4;
                        is_dir = perms & 0x4000 != 0; // S_IFDIR
                    }

                    if attrs_flags & 0x00000001 != 0 {
                        // SSH_FILEXFER_ATTR_SIZE
                        size = u64::from_be_bytes([
                            resp[offset], resp[offset + 1], resp[offset + 2], resp[offset + 3],
                            resp[offset + 4], resp[offset + 5], resp[offset + 6], resp[offset + 7],
                        ]) as i64;
                        offset += 8;
                    }

                    if name != "." && name != ".." {
                        entries.push(SftpEntry { name, is_dir, size });
                    }
                }

                // Send another READDIR to check for more entries
                req_id += 1;
                let mut rd = Vec::new();
                rd.push(12u8);
                rd.extend(&req_id.to_be_bytes());
                rd.extend(&(handle.len() as u32).to_be_bytes());
                rd.extend(handle);
                let rd_pkt = build_packet(0, 0, &rd);
                channel.data(&rd_pkt[..]).await.map_err(|e| e.to_string())?;
            }
            101 => {
                // SSH_FXP_STATUS - check if EOF
                let status = u32::from_be_bytes([resp[5], resp[6], resp[7], resp[8]]);
                if status == 1 {
                    // SSH_FX_EOF
                    break;
                }
                return Err(format!("SFTP error: status={}", status));
            }
            _ => {
                return Err(format!("Unexpected SFTP packet type: {}", resp[0]));
            }
        }
    }

    entries.sort_by(|a, b| b.is_dir.cmp(&a.is_dir).then(a.name.cmp(&b.name)));
    Ok(entries)
}

fn build_packet(pkt_type: u8, _req_id: u32, data: &[u8]) -> Vec<u8> {
    let mut pkt = Vec::new();
    // Length includes: type(1) + data
    let len = 1 + data.len() as u32;
    pkt.extend(&len.to_be_bytes());
    pkt.push(pkt_type);
    pkt.extend_from_slice(data);
    pkt
}

async fn read_sftp_packet(channel: &mut russh::Channel<russh::client::Msg>) -> Result<Vec<u8>, String> {
    loop {
        match channel.wait().await {
            Some(ChannelMsg::Data { ref data }) => {
                // SFTP packet: uint32 length + byte type + data
                if data.len() < 5 { continue; }
                return Ok(data.to_vec());
            }
            Some(ChannelMsg::Eof) | None => {
                return Err("Connection closed".into());
            }
            _ => {}
        }
    }
}
