import { useState, useEffect, useCallback } from "react";
import { invoke } from "@tauri-apps/api/core";
import { IconFolder, IconClose, IconHome } from "../lib/icons";
import type { ConnectionConfig } from "../lib/types";

interface SftpEntry {
  name: string;
  is_dir: boolean;
  size: number;
}

interface Props {
  connection: ConnectionConfig;
  password?: string;
  onClose: () => void;
}

export default function SFTPBrowser({ connection, password, onClose }: Props) {
  const [cwd, setCwd] = useState("/");
  const [entries, setEntries] = useState<SftpEntry[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  const pw = password || connection.password || "";

  const loadDir = useCallback(async (dir: string) => {
    setLoading(true);
    setError("");
    try {
      const result = await invoke<SftpEntry[]>("sftp_list_dir", {
        conn: {
          name: connection.name,
          host: connection.host,
          port: connection.port,
          username: connection.username,
          auth_method: "password",
          identity_file: null,
          proxy_host: null,
          proxy_port: null,
        },
        password: pw,
        path: dir,
      });
      setEntries(result);
      setCwd(dir);
    } catch (e) {
      setError(String(e));
    } finally {
      setLoading(false);
    }
  }, [connection, pw]);

  useEffect(() => {
    loadDir(cwd);
  }, []); // eslint-disable-line

  const openDir = (name: string) => {
    const newPath = cwd === "/" ? `/${name}` : `${cwd}/${name}`;
    loadDir(newPath);
  };

  const goUp = () => {
    if (cwd === "/") return;
    const parent = cwd.substring(0, cwd.lastIndexOf("/"));
    loadDir(parent || "/");
  };

  const formatSize = (size: number): string => {
    if (size >= 1073741824) return `${(size / 1073741824).toFixed(1)} GB`;
    if (size >= 1048576) return `${(size / 1048576).toFixed(1)} MB`;
    if (size >= 1024) return `${(size / 1024).toFixed(1)} KB`;
    return `${size} B`;
  };

  return (
    <div className="sftp-browser">
      <div className="sftp-toolbar">
        <button className="tb-btn" onClick={() => loadDir("/")}><IconHome /></button>
        <button className="tb-btn" onClick={goUp}>{"\u{2191}"} Up</button>
        <span className="sftp-path">{cwd}</span>
        <div style={{ flex: 1 }} />
        <button className="tb-btn" onClick={onClose}><IconClose /></button>
      </div>

      {loading && <div className="sftp-loading">Loading...</div>}

      {error && (
        <div className="sftp-error">
          <p>{error}</p>
          <button className="btn btn-primary" onClick={() => loadDir("/")}>Retry</button>
        </div>
      )}

      {!loading && !error && (
        <div className="sftp-header">
          <span className="sftp-h-name">Name</span>
          <span className="sftp-h-size">Size</span>
        </div>
      )}

      {!loading && !error && (
        <div className="sftp-list">
          {cwd !== "/" && (
            <div className="sftp-item" onClick={goUp}>
              <span className="sftp-icon"><IconFolder /></span>
              <span className="sftp-name">..</span>
              <span className="sftp-size" />
            </div>
          )}
          {entries.map((e) => (
            <div
              key={e.name}
              className="sftp-item"
              onClick={() => e.is_dir && openDir(e.name)}
              style={{ cursor: e.is_dir ? "pointer" : "default" }}
            >
              <span className="sftp-icon">{e.is_dir ? <IconFolder /> : <span className="sftp-file-icon" />}</span>
              <span className="sftp-name">{e.name}</span>
              <span className="sftp-size">{e.is_dir ? "" : formatSize(e.size)}</span>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
