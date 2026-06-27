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
        .map_err(|_| format!("Invalid address"))?;

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

    channel
        .request_subsystem(false, "sftp")
        .await
        .map_err(|e| format!("SFTP subsystem failed: {}", e))?;

    // ── SFTP INIT (version 3) ──
    // Packet: uint32 len | byte type(1=INIT) | uint32 version
    let mut init = Vec::new();
    init.extend(&(5u32).to_be_bytes());
    init.push(1);  // SSH_FXP_INIT
    init.extend(&3u32.to_be_bytes()); // version
    channel.data(&init[..]).await.map_err(|e| e.to_string())?;

    let ver = read_packet(&mut channel).await?;
    if ver[4] != 2 {
        return Err(format!("Expected SSH_FXP_VERSION(2), got type={}", ver[4]));
    }

    let mut req_id: u32 = 1;

    // ── OPENDIR ──
    // Packet: uint32 len | byte type(11=OPENDIR) | uint32 req_id | string path
    send_sftp(&mut channel, 11, req_id, &[string_bytes(path)]).await?;
    let handle_resp = read_packet(&mut channel).await?;
    if handle_resp[4] != 102 {
        let status = read_u32(&handle_resp, 9);
        return Err(match status {
            2 => "No such file or directory",
            3 => "Permission denied",
            4 => "General failure",
            1 => "End of directory",
            _ => "SFTP error",
        }.into());
    }

    let handle = read_string(&handle_resp, 9);

    let mut entries = Vec::new();
    loop {
        req_id += 1;
        // READDIR: type(12) | req_id | string handle
        send_sftp(&mut channel, 12, req_id, &[string_bytes_for(&handle)]).await?;
        let resp = read_packet(&mut channel).await?;
        let ptype = resp[4];

        if ptype == 101 {
            // SSH_FXP_STATUS
            let status = read_u32(&resp, 9);
            if status == 1 { break; } // EOF
            if status != 0 { return Err(format!("SFTP error code {}", status)); }
        }

        if ptype != 104 { break; } // not SSH_FXP_NAME

        let count = read_u32(&resp, 9) as usize;
        let mut off = 13;
        for _ in 0..count {
            if off + 8 > resp.len() { break; }
            // Parse filename
            let name = read_string_at(&resp, off);
            off += 4 + name.len();
            if off > resp.len() { break; }

            // Skip longname
            let lnamelen = read_u32(&resp, off) as usize;
            off += 4 + lnamelen;
            if off > resp.len() { break; }

            // Parse attributes
            if off + 4 > resp.len() { break; }
            let flags = read_u32(&resp, off);
            off += 4;

            let mut is_dir = false;
            let mut size: i64 = 0;

            if flags & 1 != 0 { // SSH_FILEXFER_ATTR_SIZE
                if off + 8 > resp.len() { break; }
                size = read_u64(&resp, off) as i64;
                off += 8;
            }
            if flags & 4 != 0 { // SSH_FILEXFER_ATTR_PERMISSIONS
                if off + 4 > resp.len() { break; }
                let perms = read_u32(&resp, off);
                is_dir = perms & 0x4000 != 0;
                off += 4;
            }

            if name != "." && name != ".." {
                entries.push(SftpEntry { name, is_dir, size });
            }
        }
    }

    entries.sort_by(|a, b| b.is_dir.cmp(&a.is_dir).then(a.name.cmp(&b.name)));
    Ok(entries)
}

// ── Packet helpers ─────────────────────────────────────────────────────

fn string_bytes(s: &str) -> Vec<u8> {
    let b = s.as_bytes();
    let mut v = Vec::new();
    v.extend(&(b.len() as u32).to_be_bytes());
    v.extend(b);
    v
}

fn string_bytes_for(s: &[u8]) -> Vec<u8> {
    let mut v = Vec::new();
    v.extend(&(s.len() as u32).to_be_bytes());
    v.extend(s);
    v
}

async fn send_sftp(channel: &mut russh::Channel<russh::client::Msg>, ptype: u8, req_id: u32, parts: &[Vec<u8>]) -> Result<(), String> {
    let mut payload = Vec::new();
    payload.push(ptype);
    payload.extend(&req_id.to_be_bytes());
    for p in parts {
        payload.extend(p);
    }
    let mut pkt = Vec::new();
    pkt.extend(&(payload.len() as u32).to_be_bytes());
    pkt.extend(payload);
    channel.data(&pkt[..]).await.map_err(|e| e.to_string())
}

async fn read_packet(channel: &mut russh::Channel<russh::client::Msg>) -> Result<Vec<u8>, String> {
    let mut buf = Vec::new();
    loop {
        match channel.wait().await {
            Some(ChannelMsg::Data { ref data }) => {
                buf.extend_from_slice(data);
                // Complete SFTP packet: uint32 length + payload
                while buf.len() >= 5 {
                    let pkt_len = u32::from_be_bytes([buf[0], buf[1], buf[2], buf[3]]) as usize;
                    let total = 4 + pkt_len;
                    if buf.len() >= total {
                        return Ok(buf[..total].to_vec());
                    }
                    break;
                }
            }
            Some(ChannelMsg::Eof) | None => return Err("Connection closed".into()),
            _ => {}
        }
    }
}

fn read_u32(buf: &[u8], off: usize) -> u32 {
    u32::from_be_bytes([buf[off], buf[off+1], buf[off+2], buf[off+3]])
}

fn read_u64(buf: &[u8], off: usize) -> u64 {
    u64::from_be_bytes([buf[off], buf[off+1], buf[off+2], buf[off+3],
                        buf[off+4], buf[off+5], buf[off+6], buf[off+7]])
}

fn read_string(buf: &[u8], off: usize) -> Vec<u8> {
    let len = read_u32(buf, off) as usize;
    buf[off+4..off+4+len].to_vec()
}

fn read_string_at(buf: &[u8], off: usize) -> String {
    let len = read_u32(buf, off) as usize;
    String::from_utf8_lossy(&buf[off+4..off+4+len]).to_string()
}
