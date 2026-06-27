import { useState, useEffect, useCallback } from "react";
import { invoke } from "@tauri-apps/api/core";
import ConnectionList from "./components/ConnectionList";
import Terminal from "./components/Terminal";
import ConnectionDialog from "./components/ConnectionDialog";
import VaultDialog from "./components/VaultDialog";
import type { ConnectionConfig } from "./lib/types";

export default function App() {
  const [connections, setConnections] = useState<ConnectionConfig[]>([]);
  const [activeSession, setActiveSession] = useState<string | null>(null);
  const [vaultUnlocked, setVaultUnlocked] = useState(false);
  const [showNewConn, setShowNewConn] = useState(false);
  const [showVault, setShowVault] = useState(false);

  // Load connections from disk on startup
  useEffect(() => {
    (async () => {
      try {
        const raw = await invoke<string>("load_connections");
        const parsed = JSON.parse(raw);
        if (Array.isArray(parsed)) setConnections(parsed);
      } catch {}
    })();
  }, []);

  // Save connections whenever they change
  useEffect(() => {
    (async () => {
      try {
        await invoke("save_connections", { data: JSON.stringify(connections, null, 2) });
      } catch {}
    })();
  }, [connections]);

  const handleConnect = useCallback((name: string) => {
    setActiveSession(name);
  }, []);

  const handleDisconnect = useCallback(() => {
    setActiveSession(null);
  }, []);

  const handleCreateConnection = useCallback((cfg: ConnectionConfig) => {
    setConnections((prev) => [...prev, cfg]);
    setShowNewConn(false);
  }, []);

  const handleVaultUnlocked = useCallback(() => {
    setVaultUnlocked(true);
    setShowVault(false);
  }, []);

  const activeConn = connections.find((c) => c.name === activeSession);

  return (
    <div className="app-shell">
      <header className="app-toolbar">
        <div className="toolbar-left">
          <button className="tb-btn" onClick={() => setShowNewConn(true)}>+ New</button>
          <button className="tb-btn" onClick={() => activeSession && handleDisconnect()}>
            Disconnect
          </button>
        </div>
        <div className="toolbar-center">
          <span className="app-title">SafeSSH</span>
        </div>
        <div className="toolbar-right">
          <button className="tb-btn" onClick={() => setShowVault(true)}>
            {vaultUnlocked ? "\u{1F512}" : "\u{1F513}"} Vault
          </button>
        </div>
      </header>

      <div className="app-body">
        <aside className="sidebar">
          <div className="sidebar-header">Connections</div>
          <ConnectionList
            connections={connections}
            onConnect={handleConnect}
            activeSession={activeSession}
          />
        </aside>

        <main className="main-content">
          {activeConn ? (
            <Terminal
              connection={activeConn}
              onDisconnect={handleDisconnect}
            />
          ) : (
            <div className="welcome">
              <h1>SafeSSH</h1>
              <p>Select a connection to start</p>
            </div>
          )}
        </main>
      </div>

      {showNewConn && (
        <ConnectionDialog
          onSave={handleCreateConnection}
          onClose={() => setShowNewConn(false)}
        />
      )}

      {showVault && (
        <VaultDialog
          onUnlocked={handleVaultUnlocked}
          onClose={() => setShowVault(false)}
        />
      )}
    </div>
  );
}
