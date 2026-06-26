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

export default function ConnectionList({
  connections,
  onConnect,
  activeSession,
}: Props) {
  const groups = new Map<string, ConnectionConfig[]>();

  for (const c of connections) {
    const g = c.group || "";
    if (!groups.has(g)) groups.set(g, []);
    groups.get(g)!.push(c);
  }

  return (
    <div className="conn-list">
      {groups.size === 0 && (
        <div style={{ padding: "16px", color: "var(--text-muted)", textAlign: "center" }}>
          No connections yet
        </div>
      )}
      {Array.from(groups.entries()).map(([group, conns]) => (
        <div key={group}>
          {group && (
            <div
              style={{
                padding: "8px 12px 4px",
                fontSize: "11px",
                fontWeight: 600,
                color: "var(--text-muted)",
                textTransform: "uppercase",
                letterSpacing: "0.5px",
              }}
            >
              {group}
            </div>
          )}
          {conns.map((c) => (
            <div
              key={c.name}
              className={`conn-item ${activeSession === c.name ? "active" : ""}`}
              onClick={() => onConnect(c.name)}
            >
              <div className={`conn-icon ${c.protocol}`}>
                {protoIcon[c.protocol] || "\u{1F310}"}
              </div>
              <div className="conn-info">
                <div className="conn-name">{c.name}</div>
                <div className="conn-meta">
                  {c.username}@{c.host}:{c.port}
                </div>
              </div>
            </div>
          ))}
        </div>
      ))}
    </div>
  );
}
