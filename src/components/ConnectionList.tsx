import type { ConnectionConfig } from "../lib/types";

interface Props {
  connections: ConnectionConfig[];
  onConnect: (name: string) => void;
  activeSession: string | null;
}

const protoIcon: Record<string, string> = {
  ssh: "\u{1F512}",
  telnet: "\u{1F4E1}",
  serial: "\u{1F4BB}",
  rdp: "\u{1F5A5}",
  vnc: "\u{1F4FA}",
};

export default function ConnectionList({ connections, onConnect }: Props) {
  // Group by group field; "Ungrouped" for empty
  const groups = new Map<string, ConnectionConfig[]>();
  for (const c of connections) {
    const g = c.group?.trim() || "Ungrouped";
    if (!groups.has(g)) groups.set(g, []);
    groups.get(g)!.push(c);
  }

  return (
    <div className="conn-list">
      {connections.length === 0 && (
        <div style={{ padding: "20px 16px", color: "var(--text-muted)", textAlign: "center", fontSize: 12 }}>
          No connections yet.
          <br />
          Click <strong>+ New</strong> to create one.
        </div>
      )}

      {Array.from(groups.entries()).map(([group, conns]) => (
        <div key={group}>
          {/* Folder header */}
          <div className="conn-group-header">
            <span className="folder-icon">{"\u{1F4C1}"}</span>
            <span>{group}</span>
            <span className="group-count">{conns.length}</span>
          </div>

          {/* Connections under folder */}
          {conns.map((c) => (
            <div
              key={c.name}
              className="conn-item"
              onClick={() => onConnect(c.name)}
            >
              <div className={`conn-icon ${c.protocol}`}>
                {protoIcon[c.protocol] || "\u{1F310}"}
              </div>
              <div className="conn-info">
                <div className="conn-name">{c.name}</div>
                <div className="conn-meta">
                  {c.username ? `${c.username}@` : ""}
                  {c.host}:{c.port}
                </div>
              </div>
            </div>
          ))}
        </div>
      ))}
    </div>
  );
}