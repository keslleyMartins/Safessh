export interface ConnectionInfo {
  name: string;
  host: string;
  port: number;
  username: string;
  auth_method: string;
  identity_file?: string;
  proxy_host?: string;
  proxy_port?: number;
}

export interface VaultEntry {
  id: string;
  service: string;
  username: string;
  password: string;
}

export type ProtocolType = "ssh" | "telnet" | "serial" | "rdp" | "vnc";

export interface ConnectionConfig {
  name: string;
  protocol: ProtocolType;
  host: string;
  port: number;
  username: string;
  password: string;
  authMethod: string;
  identityFile: string;
  group: string;
  timeout: number;
}
