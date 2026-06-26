import { useState } from "react";
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
  const [showVault, setShowVault] = useState(!vaultUnlocked);

  function handleConnect(name: string) {
    setActiveSession(name);
  }

  function handleDisconnect() {
    setActiveSession(null);
  }

  function handleCreateConnection(cfg: ConnectionConfig) {
    setConnections((prev) => [...prev, cfg]);
    setShowNewConn(false);
  }

  function handleVaultUnlocked() {
    setVaultUnlocked(true);
    setShowVault(false);
  }

  return (
    <div className="app-shell">
      <header className="app-toolbar">
        <div className="toolbar-left">
          <button className="tb-btn" onClick={() => setShowNewConn(true)}>
            + New
          </button>
          <button className="tb-btn" onClick={() => activeSession && handleDisconnect()}>
            Disconnect
          </button>
        </div>
        <div className="toolbar-center">
          <span className="app-title">SafeSSH</span>
        </div>
        <div className="toolbar-right">
          <button className="tb-btn" onClick={() => setShowVault(true)}>
            {vaultUnlocked ? "\u{1F512} Vault" : "\u{1F513} Unlock"}
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
          {activeSession ? (
            <Terminal
              sessionId={activeSession}
              connection={connections.find((c) => c.name === activeSession)!}
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
