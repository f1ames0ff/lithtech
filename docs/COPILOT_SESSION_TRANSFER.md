# Transferring an agent session into GitHub Copilot

This document describes a reproducible handoff from an agent chat (OpenCode in the tested
case) into GitHub Copilot CLI and the GitHub Copilot desktop app.

The transfer has two independent parts:

1. **Chat import** through `copilot sessions import`. This is the supported path and produces a
   session that can be resumed in Copilot CLI.
2. **Desktop-app indexing** through the local Copilot application database. The desktop app does
   not automatically list sessions created only by the CLI; its project/session index is kept in
   `%USERPROFILE%\.copilot\data.db`.

The desktop-app step uses an internal schema. Treat it as a local migration, not as a stable
public API. Back up the database and stop the app before changing it.

## Agent handoff checklist

An agent performing the transfer must:

1. Identify the source session ID, title, working directory and creation timestamp.
2. Export the user and assistant text messages into the semantic JSONL format below.
3. Run `copilot sessions import --dry-run` and fix every diagnostic before importing.
4. Import the session and record the returned `sessionId`.
5. Resume the session in CLI and verify the first and last transferred messages.
6. If desktop visibility is required, stop Copilot, back up `data.db`, register the project,
   session, workspace and alias rows, restart Copilot and verify the session appears.
7. Report whether the desktop transcript was visually verified. Do not claim it if only the
   session row was verified.

## 1. Semantic session JSONL

The CLI importer accepts one JSON object per line.

The first line must be a session header:

```json
{"type":"session","version":1,"externalId":"source-session-id","createdAt":"2026-10-21T15:38:01.030Z","cwd":"D:\\work\\project","name":"Project handoff"}
```

Required header fields:

| Field | Meaning |
|---|---|
| `type` | Always `session` |
| `version` | Must be `1` |
| `externalId` | Non-empty, unique identifier for the source session |
| `createdAt` | ISO 8601 timestamp |
| `cwd` | Working directory for the session |
| `name` | Human-readable session name |

Every following line is a message:

```json
{"type":"message","id":"source-message-id","timestamp":"2026-10-21T15:39:02.000Z","role":"user","content":[{"type":"text","text":"Message text"}]}
```

Message rules:

- `type` must be `message`.
- `id` must be a non-empty string and must be unique in the file.
- `timestamp` must be ISO 8601.
- `role` must be `user` or `assistant`.
- `content` must be a non-empty array.
- Text content is `{"type":"text","text":"..."}`.
- Attachments use `{"type":"attachment","path":"...","mediaType":"...","name":"..."}`.
- Reasoning, tool arguments and raw tool output are not part of this minimal format. Transfer
  decisions, user requests and assistant answers; omit tool noise unless it is essential context.

## 2. Export from OpenCode

OpenCode 2.x stores sessions in `%USERPROFILE%\.local\share\opencode\opencode.db`.

List candidate sessions:

```powershell
$db = "$env:USERPROFILE\.local\share\opencode\opencode.db"
@'
import sqlite3
p = r"C:\Users\YOUR_USER\.local\share\opencode\opencode.db"
con = sqlite3.connect("file:" + p.replace("\\", "/") + "?mode=ro", uri=True)
for row in con.execute("""
    select id, title, directory, time_updated
    from session_v2
    order by time_updated desc
    limit 20
"""):
    print(row)
'@ | python -
```

Export the selected session to semantic JSONL:

```powershell
$out = "$env:TEMP\copilot_session.jsonl"
$sessionId = "ses_XXXXXXXXXXXXXXXXXXXXXXXX"
@'
import datetime
import json
import sqlite3
import sys

db_path, session_id, out_path = sys.argv[1:4]

def iso(ms):
    return (datetime.datetime.fromtimestamp(ms / 1000.0, datetime.timezone.utc)
            .isoformat(timespec="milliseconds")
            .replace("+00:00", "Z"))

con = sqlite3.connect("file:" + db_path.replace("\\", "/") + "?mode=ro", uri=True)
con.row_factory = sqlite3.Row
cur = con.cursor()
session = cur.execute("select * from session_v2 where id = ?", (session_id,)).fetchone()
if not session:
    raise SystemExit("session not found: " + session_id)

records = [{
    "type": "session",
    "version": 1,
    "externalId": "opencode-" + session_id,
    "createdAt": iso(session["time_created"]),
    "cwd": session["directory"].replace("/", "\\"),
    "name": session["title"] or "OpenCode handoff",
}]

for row in cur.execute(
    "select id, type, time_created, data from session_message "
    "where session_id = ? order by seq", (session_id,)
):
    try:
        data = json.loads(row["data"])
    except Exception:
        continue

    if row["type"] == "user":
        text = (data.get("text") or data.get("displayText") or "").strip()
        if text:
            records.append({
                "type": "message",
                "id": row["id"],
                "timestamp": iso(row["time_created"]),
                "role": "user",
                "content": [{"type": "text", "text": text}],
            })
    elif row["type"] == "assistant":
        parts = []
        for item in data.get("content", []):
            if item.get("type") == "text":
                text = (item.get("text") or "").strip()
                if text:
                    parts.append(text)
        if parts:
            records.append({
                "type": "message",
                "id": row["id"],
                "timestamp": iso(row["time_created"]),
                "role": "assistant",
                "content": [{"type": "text", "text": "\n\n".join(parts)}],
            })

with open(out_path, "w", encoding="utf-8", newline="\n") as stream:
    for record in records:
        stream.write(json.dumps(record, ensure_ascii=False, separators=(",", ":")) + "\n")

print("records", len(records))
print("output", out_path)
'@ | python - $db $sessionId $out
```

The resulting file contains one header and one record per transferred user/assistant message. In
the tested OpenCode session this produced 247 messages from a 2,723-record raw event stream.

## 3. Import into Copilot CLI

Locate the newest Copilot CLI in the SDK installation:

```powershell
$copilot = Get-ChildItem "$env:LOCALAPPDATA\github-copilot-sdk\cli" -Recurse -Filter copilot.exe |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1 -ExpandProperty FullName
$copilot
```

Validate first:

```powershell
& $copilot sessions import "$env:TEMP\copilot_session.jsonl" `
    --dry-run --output json `
    --working-directory "D:\work\project" `
    --name "Project handoff"
```

Import:

```powershell
& $copilot sessions import "$env:TEMP\copilot_session.jsonl" `
    --output json `
    --working-directory "D:\work\project" `
    --name "Project handoff"
```

The output contains `sessionId`. Save it:

```text
{"ok":true,"code":"session_imported","sessionId":"b59da8c0-0038-4146-8197-8dd51be242a0", ...}
```

Resume the imported chat:

```powershell
& $copilot --resume b59da8c0-0038-4146-8197-8dd51be242a0
```

At this point the chat is fully available in Copilot CLI. Its files are under:

```text
%USERPROFILE%\.copilot\session-state\<sessionId>\
```

The important file is `events.jsonl`; its line count should match the imported message count plus
the session header (plus any events generated when the session is resumed).

## 4. Make the session visible in the desktop app

The desktop app reads project and session metadata from:

```text
%USERPROFILE%\.copilot\data.db
```

A CLI-only import is not listed by the app. The app must have:

- a `projects` row for the working directory;
- a `project_checkouts` row;
- a `sessions` row with the imported `sessionId`;
- a `workspaces` row with the same id;
- a `workspace_session_aliases` row;
- a `workspace_checkout_bindings` row.

Stop GitHub Copilot before modifying the database. Use SQLite's backup API for a consistent
backup:

```powershell
$copilotDb = "$env:USERPROFILE\.copilot\data.db"
$backup = "$copilotDb.pre-import-" + (Get-Date -Format "yyyyMMddHHmmss")
@'
import sqlite3
import sys

source, destination = sys.argv[1:3]
src = sqlite3.connect("file:" + source.replace("\\", "/") + "?mode=ro", uri=True)
dst = sqlite3.connect(destination)
src.backup(dst)
dst.close()
src.close()
print(destination)
'@ | python - $copilotDb $backup
```

Register the imported session. Replace the example values with the actual IDs and paths:

```powershell
$sessionId = "b59da8c0-0038-4146-8197-8dd51be242a0"
$repo = "D:\work\project"
@'
import datetime
import pathlib
import sqlite3
import sys
import uuid

db_path, session_id, repo = sys.argv[1:4]
now = (datetime.datetime.now(datetime.timezone.utc)
       .isoformat(timespec="milliseconds")
       .replace("+00:00", "Z"))

con = sqlite3.connect(db_path, timeout=15)
con.execute("PRAGMA foreign_keys=ON")
cur = con.cursor()
cur.execute("BEGIN IMMEDIATE")

project = cur.execute(
    "select id from projects where main_repo_path = ?", (repo,)
).fetchone()
project_id = project[0] if project else str(uuid.uuid4())

if not project:
    cur.execute("""
        insert into projects
        (id, name, container_kind, main_repo_path, default_branch,
         created_at, last_opened_at)
        values (?, ?, 'folder', ?, 'main', ?, ?)
    """, (project_id, pathlib.Path(repo).name, repo, now, now))

cur.execute("""
    insert or ignore into project_checkouts
    (project_id, host_id, root_resource_uri)
    values (?, 'local', ?)
""", (project_id, pathlib.Path(repo).resolve().as_uri()))

cur.execute("""
    insert or replace into sessions
    (id, title, session_type, mode, is_running, was_interrupted,
     created_at, updated_at, auto_approve, execution_location,
     title_source, provisional)
    values (?, 'Project handoff', 'project', 'interactive', 0, 0,
            ?, ?, 1, 'local', 'user', 0)
""", (session_id, now, now))

cur.execute("""
    insert or replace into workspaces
    (id, project_id, workspace_type, branch, name, session_id,
     host_id, name_source, explicitly_opened, is_initialized,
     lifecycle_generation)
    values (?, ?, 'folder', 'main', ?, ?, 'local', 'user', 1, 1, 1)
""", (session_id, project_id, pathlib.Path(repo).name, session_id))

cur.execute("""
    insert or replace into workspace_session_aliases
    (session_id, workspace_id, created_at)
    values (?, ?, ?)
""", (session_id, session_id, now))

cur.execute("""
    insert or replace into workspace_checkout_bindings
    (workspace_id, repo_path, repo_full_name, display_name,
     checkout_kind, checkout_path, worktree_id)
    values (?, ?, null, ?, 'in_place', ?, null)
""", (session_id, repo, pathlib.Path(repo).name, repo))

con.commit()
print("registered", session_id)
'@ | python - $copilotDb $sessionId $repo
```

Start GitHub Copilot again. The working directory should appear as a project and the session
should appear under it.

The desktop-app index is version-specific. Before applying the script, inspect the schema:

```powershell
@'
import sqlite3
p = r"C:\Users\YOUR_USER\.copilot\data.db"
con = sqlite3.connect("file:" + p.replace("\\", "/") + "?mode=ro", uri=True)
for table in ("projects", "sessions", "workspaces", "workspace_session_aliases"):
    print(table)
    for row in con.execute(f"pragma table_info('{table}')"):
        print(" ", row[1], row[2], row[3])
'@ | python -
```

If a required column or a `CHECK` constraint differs, stop and adapt the script to the local
schema. Do not copy a database from another machine.

## 5. Verification

CLI:

```powershell
& $copilot --resume <sessionId>
```

Desktop app:

- The project group named after the working directory must be present.
- The imported session name must be listed under it.
- The event count and message count should match the import output.

In the tested environment, the CLI transcript was complete and the desktop app listed the
project/session. The desktop app's transcript rendering depends on its own runtime and event
version; if it opens an empty conversation, continue in the CLI session using
`copilot --resume <sessionId>`. Do not claim a full desktop-app transcript transfer unless it has
been visually verified.

## 6. Privacy and lifecycle

- Copilot CLI syncs sessions to the GitHub account by default. The imported transcript may contain
  repository paths, decisions and code excerpts.
- The import is a copy. Later messages in the source agent do not update the Copilot session.
- The source session remains untouched.
- Do not commit either the semantic JSONL, the Copilot databases or the session-state directory to
  the repository.
- Keep the generated JSONL and database backups outside the worktree.

## 7. Tested reference

- Source: OpenCode 2.1.0, session `ses_ea0e344a4ffdadiiDb2ZTRsywH`.
- Exported: 248 JSONL records (1 header, 247 messages).
- Copilot CLI: `1.0.90-0`, `sessions import --dry-run` and real import succeeded.
- CLI session ID: `b59da8c0-0038-4146-8197-8dd51be242a0`.
- Desktop app: project `lithtech`, session `F.E.A.R. port` listed after indexing `data.db`.
