CREATE TABLE entries (
  id INTEGER PRIMARY KEY,
  captured_at_ms INTEGER NOT NULL,
  content_hash BLOB NOT NULL,
  content TEXT NOT NULL,
  pinned INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX entries_hash ON entries(content_hash);
CREATE INDEX entries_recent ON entries(captured_at_ms DESC, id DESC);
CREATE TABLE settings (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
