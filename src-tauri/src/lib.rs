mod commands;
mod ssh;
mod vault;
mod serial;
mod sftp;

use tauri::Manager;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    env_logger::init();

    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .manage(commands::AppState::new())
        .setup(|app| {
            #[cfg(debug_assertions)]
            {
                let window = app.get_webview_window("main").unwrap();
                window.open_devtools();
            }
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            commands::ssh_connect,
            commands::ssh_write,
            commands::ssh_resize,
            commands::ssh_disconnect,
            commands::vault_create,
            commands::vault_unlock,
            commands::vault_lock,
            commands::vault_list,
            commands::vault_set,
            commands::vault_remove,
            commands::serial_list_ports,
            commands::serial_connect,
            commands::serial_write,
            commands::serial_disconnect,
            commands::save_connections,
            commands::load_connections,
            commands::sftp_list_dir,
            commands::save_session_log,
            commands::list_session_logs,
            commands::get_session_log,
            commands::delete_session_log,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
