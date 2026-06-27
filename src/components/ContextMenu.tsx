import { useEffect, useRef } from "react";

interface MenuItem {
  label: string;
  icon?: React.ReactNode;
  onClick: () => void;
  danger?: boolean;
  divider?: boolean;
}

interface Props {
  x: number;
  y: number;
  items: MenuItem[];
  onClose: () => void;
}

export default function ContextMenu({ x, y, items, onClose }: Props) {
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const handle = (e: MouseEvent | TouchEvent) => {
      if (ref.current && !ref.current.contains(e.target as Node)) {
        onClose();
      }
    };
    document.addEventListener("mousedown", handle);
    document.addEventListener("touchstart", handle);
    return () => {
      document.removeEventListener("mousedown", handle);
      document.removeEventListener("touchstart", handle);
    };
  }, [onClose]);

  // Adjust position to stay within viewport
  const style: React.CSSProperties = { left: x, top: y };
  if (typeof document !== "undefined") {
    const maxX = window.innerWidth - 180;
    const maxY = window.innerHeight - items.length * 36;
    if (x > maxX) style.left = maxX;
    if (y > maxY) style.top = maxY;
  }

  return (
    <div className="ctx-menu" ref={ref} style={style}>
      {items.map((item, i) => (
        item.divider ? (
          <div key={i} className="ctx-divider" />
        ) : (
          <div
            key={i}
            className={`ctx-item ${item.danger ? "danger" : ""}`}
            onClick={() => { item.onClick(); onClose(); }}
          >
            {item.icon && <span className="ctx-icon">{item.icon}</span>}
            <span>{item.label}</span>
          </div>
        )
      ))}
    </div>
  );
}
