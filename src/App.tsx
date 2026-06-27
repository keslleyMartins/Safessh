import { useState, useEffect, useCallback } from "react";
import { invoke } from "@tauri-apps/api/core";
import Terminal from "./components/Terminal";
import ConnectionDialog from "./components/ConnectionDialog";
import SFTPBrowser from "./components/SFTPBrowser";
import VaultDialog from "./components/VaultDialog";
import ContextMenu from "./components/ContextMenu";
import type { ConnectionConfig } from "./lib/types";
import { IconHome, IconLock, IconUnlock, IconFolder, IconClose, IconSSH, IconTelnet, IconSerial, IconRDP, IconVNC, IconCheck, IconX, IconPlus, IconEdit, IconTrash, IconMove, IconBack } from "./lib/icons";

interface Tab { id: string; conn: ConnectionConfig; type: "terminal" | "sftp" }
type Toast = { type: "success" | "error"; msg: string } | null;
type CtxMenu = { x: number; y: number; items: { label: string; icon?: React.ReactNode; onClick: () => void; danger?: boolean; divider?: boolean }[] } | null;

export default function App() {
  const [connections, setConnections] = useState<ConnectionConfig[]>([]);
  const [tabs, setTabs] = useState<Tab[]>([]);
  const [activeTab, setActiveTab] = useState<string | null>(null);
  const [vaultUnlocked, setVaultUnlocked] = useState(false);
  const [showNewConn, setShowNewConn] = useState(false);
  const [editConn, setEditConn] = useState<ConnectionConfig | null>(null);
  const [showVault, setShowVault] = useState(false);
  const [toast, setToast] = useState<Toast>(null);
  const [ctxMenu, setCtxMenu] = useState<CtxMenu>(null);
  const [selectedFolder, setSelectedFolder] = useState<string | null>(null);

  useEffect(() => {
    (async () => {
      try { const c = JSON.parse(await invoke("load_connections")); if (Array.isArray(c)) setConnections(c); }
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

  const openTab = useCallback((conn: ConnectionConfig, type: "terminal" | "sftp" = "terminal") => {
    const existing = tabs.find((t) => t.conn.name === conn.name && t.type === type);
    if (existing) { setActiveTab(existing.id); return; }
    const id = crypto.randomUUID();
    setTabs((p) => [...p, { id, conn, type }]);
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
    const finalCfg = { ...cfg, name: cfg.name.trim() || cfg.host };
    if (editConn) {
      setConnections((p) => p.map((c) => c.name === editConn.name ? finalCfg : c));
      setEditConn(null);
    } else {
      setConnections((p) => [...p, finalCfg]);
    }
    setShowNewConn(false);
  }, [editConn]);

  const deleteConnection = useCallback((name: string) => setConnections((p) => p.filter((c) => c.name !== name)), []);
  const moveToGroup = useCallback((connName: string, newGroup: string) => setConnections((p) => p.map((c) => c.name === connName ? { ...c, group: newGroup } : c)), []);
  const deleteGroup = useCallback((groupName: string) => {
    setConnections((p) => p.map((c) => (c.group === groupName || (!c.group?.trim() && groupName === "Ungrouped")) ? { ...c, group: "" } : c));
    setSelectedFolder(null);
  }, []);
  const handleVaultUnlocked = useCallback(() => { setVaultUnlocked(true); setShowVault(false); }, []);

  // Group connections
  const grouped = new Map<string, ConnectionConfig[]>();
  for (const c of connections) {
    const g = c.group?.trim() || "Ungrouped";
    if (!grouped.has(g)) grouped.set(g, []);
    grouped.get(g)!.push(c);
  }
  const folderNames = Array.from(grouped.keys()).sort();
  const currentConns = selectedFolder ? (grouped.get(selectedFolder) || []) : [];

  const activeTabData = tabs.find((t) => t.id === activeTab);

  return (
    <div className="app-shell">
      <header className="app-toolbar">
        <div className="toolbar-left">
          <button className="tb-btn" onClick={() => { setEditConn(null); setShowNewConn(true); }}><IconPlus /> New</button>
        </div>
        <div className="toolbar-center">
          <span className="app-title">SafeSSH</span>
        </div>
        <div className="toolbar-right">
          <button className="tb-btn" onClick={() => setShowVault(true)}>{vaultUnlocked ? <IconLock /> : <IconUnlock />} Vault</button>
        </div>
      </header>

      {tabs.length > 0 && (
        <div className="browser-tabs">
          <button className="browser-tab home-btn" onClick={() => { setActiveTab(null); setSelectedFolder(null); }} title="Home"><IconHome /></button>
          {tabs.map((tab) => (
            <div key={tab.id} className={`browser-tab ${activeTab === tab.id ? "active" : ""}`} onClick={() => setActiveTab(tab.id)}>
              <span className="browser-tab-icon">{tab.type === "sftp" ? <IconFolder /> : <IconSSH />}</span>
              <span className="browser-tab-title">{tab.conn.name}{tab.type === "sftp" ? " (SFTP)" : ""}</span>
              <button className="browser-tab-close" onClick={(e) => { e.stopPropagation(); closeTab(tab.id); }}><IconClose /></button>
            </div>
          ))}
        </div>
      )}

      <div className="app-content">
        {activeTabData ? (
          activeTabData.type === "sftp" ? (
            <SFTPBrowser key={activeTabData.id} connection={activeTabData.conn} onClose={() => closeTab(activeTabData.id)} />
          ) : (
            <Terminal key={activeTabData.id} connection={activeTabData.conn} onReady={handleTerminalReady} onDisconnect={() => closeTab(activeTabData.id)} onToast={(msg) => showToast("success", msg)} />
          )
        ) : (
          <div className="conn-browser">
            <div className="conn-browser-header">
              {selectedFolder ? (
                <>
                  <button className="tb-btn" onClick={() => setSelectedFolder(null)}><IconBack /> Folders</button>
                  <h2 style={{ marginLeft: 12 }}>{selectedFolder}</h2>
                  <span className="conn-count">{currentConns.length}</span>
                </>
              ) : (
                <>
                  <h2>Folders</h2>
                  <span className="conn-count">{folderNames.length} groups</span>
                </>
              )}
            </div>

            {connections.length === 0 ? (
              <div className="conn-browser-empty">
                <div className="empty-icon"><IconFolder /></div>
                <p>No connections yet</p>
                <button className="btn btn-primary" onClick={() => setShowNewConn(true)}>+ Create your first connection</button>
              </div>
            ) : !selectedFolder ? (
              /* ── Folder grid ── */
              <div className="conn-browser-grid">
                {folderNames.map((g) => {
                  const conns = grouped.get(g)!;
                  return (
                    <div key={g} className="conn-folder-card" onClick={() => setSelectedFolder(g)} onContextMenu={(e) => {
                      e.preventDefault();
                      setCtxMenu({
                        x: e.clientX, y: e.clientY,
                        items: [{ label: "Delete Group", icon: <IconTrash />, onClick: () => deleteGroup(g), danger: true }],
                      });
                    }}>
                      <div className="conn-folder-card-icon"><IconFolder /></div>
                      <div className="conn-folder-card-info">
                        <div className="conn-folder-card-name">{g}</div>
                        <div className="conn-folder-card-count">{conns.length} host{conns.length > 1 ? "s" : ""}</div>
                      </div>
                    </div>
                  );
                })}
              </div>
            ) : (
              /* ── Hosts in folder ── */
              <div className="conn-browser-hosts">
                {currentConns.map((c) => (
                  <div key={c.name} className="conn-browser-item"
                    onClick={() => openTab(c)}
                    onContextMenu={(e) => {
                      e.preventDefault();
                      setCtxMenu({
                        x: e.clientX, y: e.clientY,
                        items: [
                          { label: "Connect", icon: <IconSSH />, onClick: () => openTab(c) },
                          { label: "SFTP", icon: <IconFolder />, onClick: () => openTab(c, "sftp") },
                          { divider: true, label: "", onClick: () => {} },
                          { label: "Edit", icon: <IconEdit />, onClick: () => { setEditConn(c); setShowNewConn(true); } },
                          { label: "Move to...", icon: <IconMove />, onClick: () => { const g = prompt("Move to group:", c.group || ""); if (g !== null) moveToGroup(c.name, g.trim() || ""); }},
                          { divider: true, label: "", onClick: () => {} },
                          { label: "Delete", icon: <IconTrash />, onClick: () => deleteConnection(c.name), danger: true },
                        ]
                      });
                    }}
                  >
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
            )}
          </div>
        )}
      </div>

      {ctxMenu && <ContextMenu x={ctxMenu.x} y={ctxMenu.y} items={ctxMenu.items} onClose={() => setCtxMenu(null)} />}
      {showNewConn && <ConnectionDialog onSave={handleCreateConnection} onClose={() => { setShowNewConn(false); setEditConn(null); }} editConfig={editConn} />}
      {showVault && <VaultDialog onUnlocked={handleVaultUnlocked} onClose={() => setShowVault(false)} />}
      {toast && <div className={`toast ${toast.type}`}><span className="toast-icon">{toast.type === "success" ? <IconCheck /> : <IconX />}</span><span className="toast-msg">{toast.msg}</span></div>}
    </div>
  );
}
