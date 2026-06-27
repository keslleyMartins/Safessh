import { useState, useEffect, useCallback } from "react";
import { invoke } from "@tauri-apps/api/core";
import Terminal from "./components/Terminal";
import ConnectionDialog from "./components/ConnectionDialog";
import VaultDialog from "./components/VaultDialog";
import type { ConnectionConfig } from "./lib/types";
import { IconHome, IconLock, IconUnlock, IconFolder, IconClose, IconSSH, IconTelnet, IconSerial, IconRDP, IconVNC, IconCheck, IconX, IconPlus } from "./lib/icons";

interface Tab { id: string; conn: ConnectionConfig }
type Toast = { type: "success" | "error"; msg: string } | null;

export default function App() {
  const [connections, setConnections] = useState<ConnectionConfig[]>([]);
  const [tabs, setTabs] = useState<Tab[]>([]);
  const [activeTab, setActiveTab] = useState<string | null>(null);
  const [vaultUnlocked, setVaultUnlocked] = useState(false);
  const [showNewConn, setShowNewConn] = useState(false);
  const [showVault, setShowVault] = useState(false);
  const [toast, setToast] = useState<Toast>(null);

  useEffect(() => {
    (async () => {
      try { setConnections(JSON.parse(await invoke("load_connections"))); }
      catch {}
    })();
  }, []);

  useEffect(() => {
    (async () => {
      try { await invoke("save_connections", { data: JSON.stringify(connections) }); }
      catch (e) { console.error("save", e); }
    })();
  }, [connections]);

  useEffect(() => {
    if (!toast) return;
    const t = setTimeout(() => setToast(null), 3500);
    return () => clearTimeout(t);
  }, [toast]);

  const showToast = useCallback((type: "success" | "error", msg: string) => setToast({ type, msg }), []);
  const handleTerminalReady = useCallback((ok: boolean, msg: string) => showToast(ok ? "success" : "error", msg), [showToast]);

  const openTab = useCallback((conn: ConnectionConfig) => {
    const existing = tabs.find((t) => t.conn.name === conn.name);
    if (existing) { setActiveTab(existing.id); return; }
    const id = crypto.randomUUID();
    setTabs((p) => [...p, { id, conn }]);
    setActiveTab(id);
  }, [tabs]);

  const closeTab = useCallback((tabId: string) => {
    setTabs((prev) => {
      const idx = prev.findIndex((t) => t.id === tabId);
      const next = prev.filter((t) => t.id !== tabId);
      if (activeTab === tabId) setActiveTab(next.length > 0 ? next[Math.min(idx, next.length - 1)].id : null);
      return next;
    });
  }, [activeTab]);

  const handleCreateConnection = useCallback((cfg: ConnectionConfig) => {
    setConnections((p) => [...p, { ...cfg, name: cfg.name.trim() || cfg.host }]);
    setShowNewConn(false);
  }, []);

  const handleVaultUnlocked = useCallback(() => { setVaultUnlocked(true); setShowVault(false); }, []);

  const groups = new Map<string, ConnectionConfig[]>();
  for (const c of connections) {
    const g = c.group?.trim() || "Ungrouped";
    if (!groups.has(g)) groups.set(g, []);
    groups.get(g)!.push(c);
  }

  const activeTabData = tabs.find((t) => t.id === activeTab);

  return (
    <div className="app-shell">
      <header className="app-toolbar">
        <div className="toolbar-left">
          <button className="tb-btn" onClick={() => setShowNewConn(true)}><IconPlus /> New Connection</button>
        </div>
        <div className="toolbar-center">
          <span className="app-title">SafeSSH</span>
        </div>
        <div className="toolbar-right">
          <button className="tb-btn" onClick={() => setShowVault(true)}>
            {vaultUnlocked ? <IconLock /> : <IconUnlock />} Vault
          </button>
        </div>
      </header>

      {tabs.length > 0 && (
        <div className="browser-tabs">
          <button className="browser-tab home-btn" onClick={() => setActiveTab(null)} title="Home"><IconHome /></button>
          {tabs.map((tab) => (
            <div key={tab.id} className={`browser-tab ${activeTab === tab.id ? "active" : ""}`} onClick={() => setActiveTab(tab.id)}>
              <span className="browser-tab-icon"><IconSSH /></span>
              <span className="browser-tab-title">{tab.conn.name}</span>
              <button className="browser-tab-close" onClick={(e) => { e.stopPropagation(); closeTab(tab.id); }}><IconClose /></button>
            </div>
          ))}
        </div>
      )}

      <div className="app-content">
        {activeTabData ? (
          <Terminal key={activeTabData.id} connection={activeTabData.conn} onReady={handleTerminalReady} onDisconnect={() => closeTab(activeTabData.id)} />
        ) : (
          <div className="conn-browser">
            <div className="conn-browser-header">
              <h2>Connections</h2>
              <span className="conn-count">{connections.length} saved</span>
            </div>

            {connections.length === 0 ? (
              <div className="conn-browser-empty">
                <div className="empty-icon"><IconFolder /></div>
                <p>No connections yet</p>
                <button className="btn btn-primary" onClick={() => setShowNewConn(true)}>+ Create your first connection</button>
              </div>
            ) : (
              <div className="conn-browser-grid">
                {Array.from(groups.entries()).map(([group, conns]) => (
                  <div key={group} className="conn-folder">
                    <div className="conn-folder-header">
                      <span className="conn-folder-icon"><IconFolder /></span>
                      <span className="conn-folder-name">{group}</span>
                      <span className="conn-folder-count">{conns.length}</span>
                    </div>
                    <div className="conn-folder-items">
                      {conns.map((c) => (
                        <div key={c.name} className="conn-browser-item" onClick={() => openTab(c)}>
                          <div className={`conn-browser-icon ${c.protocol}`}>
                            {c.protocol === "ssh" ? <IconSSH /> : c.protocol === "telnet" ? <IconTelnet /> : c.protocol === "serial" ? <IconSerial /> : c.protocol === "rdp" ? <IconRDP /> : <IconVNC />}
                          </div>
                          <div className="conn-browser-info">
                            <div className="conn-browser-name">{c.name}</div>
                            <div className="conn-browser-meta">{c.username ? `${c.username}@` : ""}{c.host}:{c.port}</div>
                          </div>
                        </div>
                      ))}
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}
      </div>

      {showNewConn && <ConnectionDialog onSave={handleCreateConnection} onClose={() => setShowNewConn(false)} />}
      {showVault && <VaultDialog onUnlocked={handleVaultUnlocked} onClose={() => setShowVault(false)} />}
      {toast && <div className={`toast ${toast.type}`}><span className="toast-icon">{toast.type === "success" ? <IconCheck /> : <IconX />}</span><span className="toast-msg">{toast.msg}</span></div>}
    </div>
  );
}
