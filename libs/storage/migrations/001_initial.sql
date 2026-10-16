-- Design migration only. Automatic migration execution is not wired yet.
PRAGMA foreign_keys = ON;
CREATE TABLE IF NOT EXISTS entries (
  id INTEGER PRIMARY KEY,
  captured_at_ms INTEGER NOT NULL,
  payload BLOB NOT NULL
);
CREATE TABLE IF NOT EXISTS settings (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
