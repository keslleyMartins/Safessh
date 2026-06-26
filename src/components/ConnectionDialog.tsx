import { useState } from "react";
import type { ConnectionConfig, ProtocolType } from "../lib/types";

interface Props {
  onSave: (cfg: ConnectionConfig) => void;
  onClose: () => void;
}

const defaultPorts: Record<ProtocolType, number> = {
  ssh: 22,
  telnet: 23,
  serial: 0,
  rdp: 3389,
  vnc: 5900,
};

export default function ConnectionDialog({ onSave, onClose }: Props) {
  const [protocol, setProtocol] = useState<ProtocolType>("ssh");
  const [name, setName] = useState("");
  const [host, setHost] = useState("");
  const [port, setPort] = useState(22);
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [authMethod, setAuthMethod] = useState("password");
  const [identityFile, setIdentityFile] = useState("");
  const [group, setGroup] = useState("");
  const [timeout, setTimeout_] = useState(10);

  function handleProtocolChange(p: string) {
    const proto = p as ProtocolType;
    setProtocol(proto);
    setPort(defaultPorts[proto]);
  }

  function handleSave() {
    if (!name.trim()) return;
    onSave({
      name: name.trim(),
      protocol,
      host,
      port,
      username,
      password,
      authMethod,
      identityFile,
      group,
      timeout,
    });
  }

  return (
    <div className="dialog-overlay" onClick={onClose}>
      <div className="dialog" onClick={(e) => e.stopPropagation()}>
        <h2>New Connection</h2>

        <div className="form-group">
          <label>Name</label>
          <input
            placeholder="e.g. Production Server"
            value={name}
            onChange={(e) => setName(e.target.value)}
          />
        </div>

        <div className="form-row">
          <div className="form-group">
            <label>Protocol</label>
            <select
              value={protocol}
              onChange={(e) => handleProtocolChange(e.target.value)}
            >
              <option value="ssh">SSH</option>
              <option value="telnet">Telnet</option>
              <option value="serial">Serial</option>
              <option value="rdp">RDP</option>
              <option value="vnc">VNC</option>
            </select>
          </div>
          <div className="form-group">
            <label>Group</label>
            <input
              placeholder="e.g. Production"
              value={group}
              onChange={(e) => setGroup(e.target.value)}
            />
          </div>
        </div>

        {(protocol === "ssh" || protocol === "telnet" || protocol === "rdp" || protocol === "vnc") && (
          <div className="form-row">
            <div className="form-group" style={{ flex: 3 }}>
              <label>Host</label>
              <input
                placeholder="hostname or IP"
                value={host}
                onChange={(e) => setHost(e.target.value)}
              />
            </div>
            <div className="form-group" style={{ flex: 1 }}>
              <label>Port</label>
              <input
                type="number"
                value={port}
                onChange={(e) => setPort(Number(e.target.value))}
              />
            </div>
          </div>
        )}

        {protocol === "serial" && (
          <div className="form-row">
            <div className="form-group">
              <label>Port</label>
              <input placeholder="COM3 or /dev/ttyUSB0" value={host} onChange={(e) => setHost(e.target.value)} />
            </div>
            <div className="form-group">
              <label>Baud rate</label>
              <select value={port} onChange={(e) => setPort(Number(e.target.value))}>
                {[1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600].map((b) => (
                  <option key={b} value={b}>{b}</option>
                ))}
              </select>
            </div>
          </div>
        )}

        {(protocol === "ssh" || protocol === "rdp") && (
          <>
            <div className="form-group">
              <label>Username</label>
              <input
                placeholder="root"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
              />
            </div>

            <div className="form-group">
              <label>Password</label>
              <input
                type="password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
              />
            </div>
          </>
        )}

        {protocol === "vnc" && (
          <div className="form-group">
            <label>Password</label>
            <input
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
            />
          </div>
        )}

        {protocol === "ssh" && (
          <>
            <div className="form-group">
              <label>Auth method</label>
              <select value={authMethod} onChange={(e) => setAuthMethod(e.target.value)}>
                <option value="password">Password</option>
                <option value="key">Key File</option>
                <option value="agent">Agent</option>
              </select>
            </div>

            <div className="form-group">
              <label>Identity file</label>
              <input
                placeholder="~/.ssh/id_ed25519"
                value={identityFile}
                onChange={(e) => setIdentityFile(e.target.value)}
              />
            </div>
          </>
        )}

        <div className="dialog-actions">
          <button className="btn btn-secondary" onClick={onClose}>
            Cancel
          </button>
          <button className="btn btn-primary" onClick={handleSave}>
            Save
          </button>
        </div>
      </div>
    </div>
  );
}
