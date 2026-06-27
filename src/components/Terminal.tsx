import { useEffect, useRef, useState, useCallback } from "react";
import { Terminal as XTerm } from "@xterm/xterm";
import { FitAddon } from "@xterm/addon-fit";
import "@xterm/xterm/css/xterm.css";
import { invoke } from "@tauri-apps/api/core";
import { listen, type UnlistenFn } from "@tauri-apps/api/event";
import type { ConnectionConfig } from "../lib/types";

interface Props {
  connection: ConnectionConfig;
  password?: string;
  onDisconnect: () => void;
}

type ConnStage = "resolving" | "connecting" | "handshake" | "auth" | "shell" | "connected" | "error";

const stageLabels: Record<ConnStage, string> = {
  resolving: "Resolving hostname...",
  connecting: "Connecting to host...",
  handshake: "SSH handshake in progress...",
  auth: "Authenticating...",
  shell: "Opening shell session...",
  connected: "Connected",
  error: "Connection failed",
};

export default function Terminal({ connection, password, onDisconnect }: Props) {
  const termRef = useRef<HTMLDivElement>(null);
  const xtermRef = useRef<XTerm | null>(null);
  const sessionRef = useRef<string | null>(null);
  const unlistenersRef = useRef<UnlistenFn[]>([]);
  const [stage, setStage] = useState<ConnStage>("resolving");
  const [stageMsg, setStageMsg] = useState("");

  const cleanup = useCallback(async () => {
    for (const un of unlistenersRef.current) un();
    unlistenersRef.current = [];
    if (sessionRef.current) {
      try { await invoke("ssh_disconnect", { sessionId: sessionRef.current }); } catch {}
      sessionRef.current = null;
    }
  }, []);

  useEffect(() => {
    const container = termRef.current;
    if (!container) return;

    const term = new XTerm({
      cursorBlink: true,
      cursorStyle: "block",
      fontSize: 14,
      fontFamily: '"SF Mono", "Fira Code", "Cascadia Code", monospace',
      theme: {
        background: "#0d0d0f",
        foreground: "#e4e4e7",
        cursor: "#e4e4e7",
        selectionBackground: "#5b6ee244",
        black: "#1a1b1e", red: "#ef4444", green: "#4ade80",
        yellow: "#facc15", blue: "#60a5fa", magenta: "#c084fc",
        cyan: "#2dd4bf", white: "#e4e4e7",
        brightBlack: "#71717a", brightRed: "#f87171", brightGreen: "#86efac",
        brightYellow: "#fde047", brightBlue: "#93c5fd", brightMagenta: "#d8b4fe",
        brightCyan: "#67e8f9", brightWhite: "#ffffff",
      },
      allowTransparency: true,
    });

    const fitAddon = new FitAddon();
    term.loadAddon(fitAddon);

    // Defer fit until container has size
    const ro = new ResizeObserver(() => {
      try { fitAddon.fit(); } catch {}
    });
    ro.observe(container);

    // Small delay to ensure DOM layout
    requestAnimationFrame(() => {
      try { fitAddon.fit(); } catch {}
    });

    term.open(container);

    const connectSSH = async () => {
      setStage("resolving");
      setStageMsg(`Connecting to ${connection.host}:${connection.port}...`);

      try {
        const sessionId = await invoke<string>("ssh_connect", {
          conn: {
            name: connection.name,
            host: connection.host,
            port: connection.port,
            username: connection.username,
            auth_method: connection.authMethod || "password",
            identity_file: connection.identityFile || null,
            proxy_host: null,
            proxy_port: null,
          },
          password: password || connection.password || null,
        });

        sessionRef.current = sessionId;

        const unlistenData = await listen<number[]>(`ssh-data-${sessionId}`, (event) => {
          setStage("connected");
          const bytes = new Uint8Array(event.payload);
          term.write(bytes);
        });

        const unlistenError = await listen<string>(`ssh-error-${sessionId}`, (event) => {
          setStage("error");
          setStageMsg(event.payload);
          term.write(`\r\n\x1b[31m${event.payload}\x1b[0m\r\n`);
        });

        const unlistenDisconnected = await listen(`ssh-disconnected-${sessionId}`, () => {
          setStage("error");
          setStageMsg("Disconnected");
          term.write("\r\n\x1b[33mDisconnected\x1b[0m\r\n");
        });

        const unlistenStage = await listen<string>(`ssh-stage-${sessionId}`, (event) => {
          const s = event.payload as ConnStage;
          setStage(s);
          setStageMsg(stageLabels[s] || s);
        });

        const unlistenConnected = await listen(`ssh-connected-${sessionId}`, () => {
          setStage("connected");
        });

        unlistenersRef.current.push(unlistenData, unlistenError, unlistenDisconnected, unlistenStage, unlistenConnected);
      } catch (e) {
        setStage("error");
        setStageMsg(String(e));
        term.write(`\r\n\x1b[31mConnection failed: ${e}\x1b[0m\r\n`);
      }
    };

    connectSSH();

    term.onData((data) => {
      if (sessionRef.current) {
        const bytes = Array.from(data).map((c) => c.charCodeAt(0));
        invoke("ssh_write", { sessionId: sessionRef.current, data: bytes }).catch(() => {});
      }
    });

    term.onResize(({ cols, rows }) => {
      if (sessionRef.current) {
        invoke("ssh_resize", { sessionId: sessionRef.current, cols, rows }).catch(() => {});
      }
    });

    xtermRef.current = term;

    return () => {
      ro.disconnect();
      cleanup();
      term.dispose();
    };
  }, [connection, password, cleanup]);

  const isError = stage === "error";

  return (
    <div className="terminal-wrapper">
      <div className="terminal-tab-bar">
        <div className="terminal-tab active">
          <span>{connection.name}</span>
          <button className="terminal-tab-close" onClick={onDisconnect}>&times;</button>
        </div>
      </div>

      {stage !== "connected" && (
        <div className={`conn-overlay ${isError ? "error" : ""}`}>
          <div className="conn-spinner" />
          <div className="conn-stage">{stageLabels[stage]}</div>
          {stageMsg && <div className="conn-stage-msg">{stageMsg}</div>}
        </div>
      )}

      <div className="terminal-container" ref={termRef} />
    </div>
  );
}
