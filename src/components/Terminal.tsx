import { useEffect, useRef } from "react";
import { Terminal as XTerm } from "@xterm/xterm";
import { FitAddon } from "@xterm/addon-fit";
import "@xterm/xterm/css/xterm.css";
import type { ConnectionConfig } from "../lib/types";

interface Props {
  sessionId: string;
  connection: ConnectionConfig;
  onDisconnect: () => void;
}

export default function Terminal({ sessionId, connection, onDisconnect }: Props) {
  const termRef = useRef<HTMLDivElement>(null);
  const xtermRef = useRef<XTerm | null>(null);

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

    term.write(`\x1b[32mConnecting to ${connection.host}:${connection.port}...\x1b[0m\r\n`);

    // Listen for resize
    const handleResize = () => fitAddon.fit();
    window.addEventListener("resize", handleResize);

    // TODO: wire up Tauri events and commands
    // For now, show a placeholder
    setTimeout(() => {
      term.write(`\r\n\x1b[33mWelcome to SafeSSH\x1b[0m\r\n`);
      term.write(`Connected to \x1b[36m${connection.username}@${connection.host}\x1b[0m\r\n\r\n`);
      term.write(`\x1b[32m$\x1b[0m `);
    }, 500);

    // Handle user input
    term.onData((data) => {
      // TODO: send to Tauri backend
      term.write(data);
    });

    xtermRef.current = term;

    return () => {
      window.removeEventListener("resize", handleResize);
      term.dispose();
    };
  }, [sessionId, connection]);

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
