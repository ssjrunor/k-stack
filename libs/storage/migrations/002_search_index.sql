-- Design experiment only. Do not apply to private history: FTS5 exposes search tokens on disk.
-- A privacy-preserving search strategy must be selected before this migration is enabled.
CREATE VIRTUAL TABLE IF NOT EXISTS entries_fts USING fts5(content);
