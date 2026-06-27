use ssh2::Session;
use serde::Serialize;
use std::io::Read;

#[derive(Debug, Serialize)]
pub struct SftpEntry {
    pub name: String,
    pub is_dir: bool,
    pub size: i64,
}

pub fn list_dir(
    host: &str,
    port: u16,
    username: &str,
    password: &str,
    path: &str,
) -> Result<Vec<SftpEntry>, String> {
    let addr = format!("{}:{}", host, port);
    let tcp = std::net::TcpStream::connect(&addr)
        .map_err(|e| format!("Connection failed: {}", e))?;
    tcp.set_read_timeout(Some(std::time::Duration::from_secs(15))).ok();

    let mut session = Session::new()
        .map_err(|e| format!("Session error: {}", e))?;

    session.set_tcp_stream(tcp);
    session.set_blocking(true);

    session.handshake()
        .map_err(|e| format!("Handshake failed: {}", e))?;

    session.userauth_password(username, password)
        .map_err(|e| format!("Auth failed: {}", e))?;

    let sftp = session.sftp()
        .map_err(|e| format!("SFTP init: {}", e))?;

    let entries = sftp.readdir(std::path::Path::new(path))
        .map_err(|e| format!("Read dir '{}': {}", path, e))?;

    let mut result: Vec<SftpEntry> = entries
        .into_iter()
        .map(|(p, stat)| SftpEntry {
            name: p.file_name().unwrap_or_default().to_string_lossy().to_string(),
            is_dir: stat.is_dir(),
            size: stat.size.unwrap_or(0) as i64,
        })
        .collect();

    result.sort_by(|a, b| b.is_dir.cmp(&a.is_dir).then(a.name.cmp(&b.name)));
    Ok(result)
}
