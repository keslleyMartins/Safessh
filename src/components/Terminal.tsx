import { useEffect, useRef, useCallback } from "react";
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

export default function Terminal({ connection, password, onDisconnect }: Props) {
  const termRef = useRef<HTMLDivElement>(null);
  const xtermRef = useRef<XTerm | null>(null);
  const sessionRef = useRef<string | null>(null);
  const unlistenersRef = useRef<UnlistenFn[]>([]);

  const cleanup = useCallback(async () => {
    for (const un of unlistenersRef.current) {
      un();
    }
    unlistenersRef.current = [];

    if (sessionRef.current) {
      try {
        await invoke("ssh_disconnect", { sessionId: sessionRef.current });
      } catch {}
      sessionRef.current = null;
    }
  }, []);

  useEffect(() => {
    if (!termRef.current) return;

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
        black: "#1a1b1e",
        red: "#ef4444",
        green: "#4ade80",
        yellow: "#facc15",
        blue: "#60a5fa",
        magenta: "#c084fc",
        cyan: "#2dd4bf",
        white: "#e4e4e7",
        brightBlack: "#71717a",
        brightRed: "#f87171",
        brightGreen: "#86efac",
        brightYellow: "#fde047",
        brightBlue: "#93c5fd",
        brightMagenta: "#d8b4fe",
        brightCyan: "#67e8f9",
        brightWhite: "#ffffff",
      },
      allowTransparency: true,
    });

    const fitAddon = new FitAddon();
    term.loadAddon(fitAddon);
    term.open(termRef.current);
    fitAddon.fit();

    const handleResize = () => fitAddon.fit();
    window.addEventListener("resize", handleResize);

    const connectSSH = async () => {
      term.write("\x1b[32mConnecting...\x1b[0m\r\n");
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

        const unlistenData = await listen<number[]>(
          `ssh-data-${sessionId}`,
          (event) => {
            const bytes = new Uint8Array(event.payload);
            term.write(bytes);
          }
        );
        unlistenersRef.current.push(unlistenData);

        const unlistenError = await listen<string>(
          `ssh-error-${sessionId}`,
          (event) => {
            term.write(`\r\n\x1b[31m${event.payload}\x1b[0m\r\n`);
          }
        );
        unlistenersRef.current.push(unlistenError);

        const unlistenDisconnected = await listen(
          `ssh-disconnected-${sessionId}`,
          () => {
            term.write("\r\n\x1b[33mDisconnected\x1b[0m\r\n");
          }
        );
        unlistenersRef.current.push(unlistenDisconnected);

        const unlistenConnected = await listen(
          `ssh-connected-${sessionId}`,
          () => {
            term.write(""); // connected signal received
          }
        );
        unlistenersRef.current.push(unlistenConnected);
      } catch (e) {
        term.write(`\r\n\x1b[31mConnection failed: ${e}\x1b[0m\r\n`);
      }
    };

    connectSSH();

    // User input → send to SSH session
    term.onData((data) => {
      if (sessionRef.current) {
        const bytes = Array.from(data).map((c) => c.charCodeAt(0));
        invoke("ssh_write", {
          sessionId: sessionRef.current,
          data: bytes,
        }).catch(() => {});
      }
    });

    // Resize → notify SSH
    term.onResize(({ cols, rows }) => {
      if (sessionRef.current) {
        invoke("ssh_resize", {
          sessionId: sessionRef.current,
          cols,
          rows,
        }).catch(() => {});
      }
    });

    xtermRef.current = term;

    return () => {
      window.removeEventListener("resize", handleResize);
      cleanup();
      term.dispose();
    };
  }, [connection, password, cleanup]);

  return (
    <div className="terminal-wrapper">
      <div className="terminal-tab-bar">
        <div className="terminal-tab active">
          <span>{connection.name}</span>
          <button className="terminal-tab-close" onClick={onDisconnect}>
            &times;
          </button>
        </div>
      </div>
      <div className="terminal-container" ref={termRef} />
    </div>
  );
}
