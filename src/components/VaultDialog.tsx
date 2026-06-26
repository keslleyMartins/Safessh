import { useState, useEffect } from "react";

interface Props {
  onUnlocked: () => void;
  onClose: () => void;
}

export default function VaultDialog({ onUnlocked, onClose }: Props) {
  const [password, setPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [creating, setCreating] = useState(false);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // TODO: check if vault file exists via Tauri command
    // For now, assume it doesn't exist
    setCreating(true);
    setLoading(false);
  }, []);

  async function handleSubmit() {
    setError("");

    if (!password) {
      setError("Password is required");
      return;
    }

    if (creating && password !== confirmPassword) {
      setError("Passwords do not match");
      return;
    }

    try {
      // TODO: call Tauri commands
      // if (creating) await invoke("vault_create", { password });
      // else await invoke("vault_unlock", { password });
      onUnlocked();
    } catch (e) {
      setError(String(e));
    }
  }

  if (loading) return null;

  return (
    <div className="dialog-overlay" onClick={onClose}>
      <div className="dialog" onClick={(e) => e.stopPropagation()} style={{ minWidth: 360 }}>
        <h2>{creating ? "Create Vault" : "Unlock Vault"}</h2>

        <p style={{ color: "var(--text-secondary)", marginBottom: 16, fontSize: 13, lineHeight: 1.5 }}>
          {creating
            ? "Create a master password to encrypt your credentials securely."
            : "Enter your master password to unlock the vault."}
        </p>

        <div className="form-group">
          <label>Master password</label>
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
          />
        </div>

        {creating && (
          <div className="form-group">
            <label>Confirm password</label>
            <input
              type="password"
              value={confirmPassword}
              onChange={(e) => setConfirmPassword(e.target.value)}
            />
          </div>
        )}

        {error && (
          <div style={{ color: "var(--danger)", fontSize: 12, marginBottom: 8 }}>
            {error}
          </div>
        )}

        <div className="dialog-actions">
          <button className="btn btn-secondary" onClick={onClose}>
            Cancel
          </button>
          <button className="btn btn-primary" onClick={handleSubmit}>
            {creating ? "Create Vault" : "Unlock"}
          </button>
        </div>
      </div>
    </div>
  );
}
