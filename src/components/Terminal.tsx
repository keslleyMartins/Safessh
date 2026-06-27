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
  onReady: (ok: boolean, msg: string) => void;
  onDisconnect: () => void;
}

type ConnStage = "resolving" | "connecting" | "handshake" | "auth" | "shell" | "connected" | "error";

const stageLabels: Record<ConnStage, string> = {
  resolving: "Resolving hostname...",
  connecting: "Establishing TCP connection...",
  handshake: "SSH handshake in progress...",
  auth: "Authenticating...",
  shell: "Opening shell session...",
  connected: "Connected",
  error: "Connection failed",
};

export default function Terminal({ connection, password, onReady, onDisconnect }: Props) {
  const termRef = useRef<HTMLDivElement>(null);
  const xtermRef = useRef<XTerm | null>(null);
  const unlistenersRef = useRef<UnlistenFn[]>([]);
  const reportedRef = useRef(false);

  const [stage, setStage] = useState<ConnStage>("resolving");
  const [stageMsg, setStageMsg] = useState("");
  const [showOverlay, setShowOverlay] = useState(true);

  const cleanup = useCallback(async () => {
    for (const un of unlistenersRef.current) un();
    unlistenersRef.current = [];
  }, []);

  useEffect(() => {
    const container = termRef.current;
    if (!container) return;

    const sessionId = crypto.randomUUID();
    reportedRef.current = false;

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

    // Defer opening until container has size
    const init = () => {
      try {
        term.open(container);
        fitAddon.fit();
      } catch {}
    };

    if (container.clientWidth > 0 && container.clientHeight > 0) {
      init();
    } else {
      const ro = new ResizeObserver((entries) => {
        const e = entries[0];
        if (e.contentRect.width > 0 && e.contentRect.height > 0) {
          init();
          ro.disconnect();
        }
      });
      ro.observe(container);
    }

    const resizeRo = new ResizeObserver(() => {
      try { fitAddon.fit(); } catch {}
    });
    resizeRo.observe(container);

    // ── Listeners BEFORE connect (race condition fix) ──
    const setupListeners = async () => {
      const unData = await listen<number[]>(`ssh-data-${sessionId}`, (e) => {
        setStage("connected");
        setShowOverlay(false);
        if (!reportedRef.current) {
          reportedRef.current = true;
          onReady(true, "Connected");
        }
        term.write(new Uint8Array(e.payload));
      });

      const unStage = await listen<string>(`ssh-stage-${sessionId}`, (e) => {
        const s = e.payload as ConnStage;
        if (s !== "error") {
          setStage(s);
          setStageMsg(stageLabels[s] || s);
        }
      });

      const unError = await listen<string>(`ssh-error-${sessionId}`, (e) => {
        setStage("error");
        setStageMsg(e.payload);
        setShowOverlay(true);
        if (!reportedRef.current) {
          reportedRef.current = true;
          onReady(false, e.payload);
        }
      });

      const unDisconn = await listen(`ssh-disconnected-${sessionId}`, () => {
        if (!reportedRef.current) {
          reportedRef.current = true;
          onReady(false, "Disconnected");
        }
        setStage("error");
        setStageMsg("Disconnected");
        setShowOverlay(true);
      });

      unlistenersRef.current.push(unData, unStage, unError, unDisconn);

      try {
        await invoke("ssh_connect", {
          sessionId,
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
      } catch (e) {
        const msg = String(e);
        setStage("error");
        setStageMsg(msg);
        setShowOverlay(true);
        if (!reportedRef.current) {
          reportedRef.current = true;
          onReady(false, msg);
        }
      }
    };

    setupListeners();

    // Copy on selection
    term.onSelectionChange(() => {
      const sel = term.getSelection();
      if (sel) navigator.clipboard.writeText(sel).catch(() => {});
    });

    // Paste on right-click
    const handleCtx = (e: MouseEvent) => {
      e.preventDefault();
      navigator.clipboard.readText().then((t) => {
        invoke("ssh_write", { sessionId, data: Array.from(t).map((c) => c.charCodeAt(0)) }).catch(() => {});
      }).catch(() => {});
    };
    container.addEventListener("contextmenu", handleCtx);

    // Paste on middle-click / double-tap touchpad
    const handleMousedown = (e: MouseEvent) => {
      if (e.button === 1) {
        e.preventDefault();
        navigator.clipboard.readText().then((t) => {
          invoke("ssh_write", { sessionId, data: Array.from(t).map((c) => c.charCodeAt(0)) }).catch(() => {});
        }).catch(() => {});
      }
    };
    container.addEventListener("mousedown", handleMousedown);

    term.onData((data) => {
      const bytes = Array.from(data).map((c) => c.charCodeAt(0));
      invoke("ssh_write", { sessionId, data: bytes }).catch(() => {});
    });

    term.onResize(({ cols, rows }) => {
      invoke("ssh_resize", { sessionId, cols, rows }).catch(() => {});
    });

    xtermRef.current = term;

    return () => {
      resizeRo.disconnect();
      cleanup();
      invoke("ssh_disconnect", { sessionId }).catch(() => {});
      term.dispose();
    };
  }, [connection.name, connection.host, connection.port, connection.username, connection.authMethod, connection.identityFile, connection.password, password, onReady, cleanup]);

  const isError = stage === "error";

  return (
    <div className="terminal-wrapper">
      {showOverlay && (
        <div className={`conn-overlay ${isError ? "error" : ""}`}>
          {!isError && <div className="conn-spinner" />}
          {isError && <div className="conn-error-icon">!</div>}
          <div className="conn-stage">{stageLabels[stage]}</div>
          {stageMsg && <div className="conn-stage-msg">{stageMsg}</div>}
          {isError && (
            <button className="btn btn-primary" style={{ marginTop: 12 }} onClick={onDisconnect}>
              Close
            </button>
          )}
        </div>
      )}
      <div className="terminal-container" ref={termRef} style={{ height: "100%" }} />
    </div>
  );
}