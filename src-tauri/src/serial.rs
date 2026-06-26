use std::io::Read;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread;
use log::info;
use serialport::SerialPort;
use tauri::Emitter;

pub struct SerialSession {
    pub id: String,
    pub window: tauri::Window,
    handle: Option<thread::JoinHandle<()>>,
    running: Arc<AtomicBool>,
}

impl SerialSession {
    pub fn new(id: String, window: tauri::Window) -> Self {
        Self {
            id,
            window,
            handle: None,
            running: Arc::new(AtomicBool::new(false)),
        }
    }

    pub fn connect(&mut self, port_name: &str, baud_rate: u32) -> Result<(), String> {
        let session_id = self.id.clone();
        let window = self.window.clone();
        let running = self.running.clone();
        let port_name = port_name.to_string();

        running.store(true, Ordering::SeqCst);

        let handle = thread::spawn(move || {
            let mut port = match serialport::new(&port_name, baud_rate)
                .timeout(std::time::Duration::from_millis(50))
                .open()
            {
                Ok(p) => p,
                Err(e) => {
                    let _ = window.emit(&format!("serial-error-{}", session_id), format!("Open failed: {}", e));
                    return;
                }
            };

            let _ = window.emit(&format!("serial-connected-{}", session_id), ());

            let mut buf = [0u8; 1024];
            while running.load(Ordering::SeqCst) {
                match port.read(&mut buf) {
                    Ok(0) => break,
                    Ok(n) => {
                        let data: Vec<u8> = buf[..n].to_vec();
                        let _ = window.emit(&format!("serial-data-{}", session_id), data);
                    }
                    Err(e) if e.kind() == std::io::ErrorKind::TimedOut => {}
                    Err(_) => break,
                }
                thread::sleep(std::time::Duration::from_millis(10));
            }

            let _ = window.emit(&format!("serial-disconnected-{}", session_id), ());
        });

        self.handle = Some(handle);
        Ok(())
    }

    pub fn write(&self, data: &[u8]) -> Result<(), String> {
        info!("Serial write: {} bytes", data.len());
        Ok(())
    }
}

impl Drop for SerialSession {
    fn drop(&mut self) {
        self.running.store(false, Ordering::SeqCst);
    }
}

pub fn list_ports() -> Result<Vec<String>, String> {
    let ports = serialport::available_ports()
        .map_err(|e| format!("No ports: {}", e))?;
    Ok(ports.into_iter().map(|p| p.port_name).collect())
}
