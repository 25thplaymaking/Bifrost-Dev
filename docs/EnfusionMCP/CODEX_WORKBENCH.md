# Codex / Astra Workbench workflow

This server exposes the same capabilities to all MCP clients. There is no model-name allowlist, separate Astra daemon, or extra permission service.

## Start with evidence

Call `wb_capabilities` with `live=true`. It reports the configured project root/default separately from actual loaded addon IDs, world state, and the ten supported editor module probes. A default folder is not evidence of the loaded project. Module availability does not mean a resource is open or an operation was tested.

The [native agent workflow](NATIVE_AGENT_WORKFLOW.md) covers plugin installation, native container/property inspection and editing, conflict-checked script buffers, debugging, and documentation deployment. `wb_bridge` updates the verified loaded addon without launching; `wb_docs` exposes and deploys the local guides; `wb_debug` combines runtime log cursors with native debugger control and explicit observation boundaries.

`wb_projects` with `action=list` returns actual addon IDs. With `action=locate`, supply `path` as an absolute path inside a loaded project; the native endpoint identifies its owner. The old `name` parameter is accepted as a path alias, not as a guessed addon-name lookup.

The shipped lookup compares path text case-sensitively, even on Windows. Retain the casing of the path opened by Workbench. The adapter normalizes separators and handles a project-root path with or without its trailing slash.

`wb_prefabs locate` discovers `.et` source files directly on the MCP host, with a default 200-result limit and explicit partial-scan metadata. It does not use the shipped `LocatePrefabsFromPath`: that endpoint invokes `FileIO.FindFiles` inside Workbench and can block the NET API behind a Script Authorization Required dialog. Source discovery does not prove resource registration; use `getGuid` for a selected resource. Remote engine paths must also be accessible to the MCP host for local discovery.

Pass the intended absolute `projectPath` to authoring tools. Read the relevant API through `api_search`, consult `wiki_search` / `wiki_read`, then inspect shipped resources through `game_read` / `game_browse`. Never invent graph formats or engine method signatures.

## Source work across editors

`project` supports scripts, layouts, prefabs/configs/world layers, localization text, `.meta`/`.emat`, audio `.acp`, particles `.ptc`, procedural animation `.pap`/`.siga`, and animation `.agr`/`.ast`/`.agf`/`.asi`/`.aw`.

- `read`: returns text and a SHA-256. Use `startLine` and `maxLines` for numbered excerpts of larger sources. `nextLine` reports continuation. These display lines are **1-based**.
- `search`: literal, recursive text search with file/line matches; `pattern` filters an extension. Case-insensitive by default. Coverage limits and skipped sources are explicit, so an incomplete search cannot establish absence.
- `edit`: replace exact `oldText` with `newText`; defaults to exactly one match. Use `expectedMatches` for intentional repeated replacements. An optional `expectedSha256` prevents applying a read-time decision to changed content. `dryRun=true` previews without writing.
- `write`: creates/replaces a supported text source using an adjacent temporary file and rename. Supports `expectedSha256`, `expectedMissing` and `dryRun`. Writes do not silently compile/register resources.

Reads/edits support UTF-8, preserving BOM and existing newline bytes outside replacements. A source is limited to 16 MiB, one read response to 512 KB, and ranged reads to 2,000 lines. Search is bounded to 2,000 source files / 32 MiB / 10,000 directory entries / 200 results. Descendant symlinks/junctions are skipped or rejected; select the intended target root explicitly. Binary resources remain engine-owned.

The live `wb_script_editor` uses **zero-based engine indices**: line `0` is the first line. Its edits affect the open editor buffer; disk edits and unsaved editor buffers are distinct. Save or reconcile open buffers before editing the same source on disk.

## Ordered live operations

`wb_batch` accepts up to 25 steps:

```json
{
  "steps": [
    { "tool": "wb_projects", "arguments": { "action": "list" } },
    { "tool": "wb_editor_module", "arguments": { "module": "audioEditor", "action": "state" } }
  ]
}
```

Eligible names come from the existing tool registry and are listed by `wb_capabilities`. Every step's schema and usage requirements are checked before the first operation. Runtime mode/error checks still run for each step. The default stops on the first error or unconfirmed operation; `stopOnError=false` is suitable only for independent steps.

This is **not an atomic transaction**. Earlier mutations remain applied. Inspect each result before retrying. `executed` counts attempted steps; inspect each step's outcome to distinguish completed operations from runtime blocks/errors. No retry, rollback, or implicit undo is performed. Cancellation, a 60-second between-call budget, or a 240,000-character output budget stops later steps; an in-flight request retains its normal timeout. Launch/reload/play/stop and offline authoring are individual tools.

Common Workbench tools now return their raw engine data alongside readable text as `structuredContent`, with `completed`, `blocked`, or `error` outcomes. A failed mode guard, uppercase native `ERROR`, or unconfirmed save is an MCP error. Save timeouts do not prove that a dialog opened or that the file saved.

## Editor boundaries

World, script, resource, and localization operations use their existing dedicated interfaces. Audio, particle, animation, procedural animation, and behavior editors expose shared module control through `wb_editor_module`. Structured generators and source editing provide additional authoring where formats are verified.

No public live graph-node editing API is invented. Behavior graph generation, general terrain sculpting, and navmesh generation remain explicit coverage gaps. `wb_capabilities` reports these limits. Graph source edits must still pass appropriate format/engine validation. Workbench compilation does not establish in-game or multiplayer behavior.

Foreground-sensitive opens/actions require `allowForeground=true` when the user has authorized window activity. Existing authorization is sufficient; callers need not ask the same question again. Ordinary discovery and source work remain in the background. Native `wb_validate_scripts` / `wb_reload` preserve the safe compile lifecycle.

## Codex registration and updates

Use one direct Node registration with an explicit startup timeout:

```toml
[mcp_servers.enfusion-mcp]
command = 'C:\Program Files\nodejs\node.exe'
args = ['C:\path\to\enfusion-mcp-bifrost\dist\index.js']
startup_timeout_sec = 60
enabled = true
```

Keep existing environment settings from the installation. Check for an existing registration before adding one. Do not introduce shell wrappers or visible consoles.

After rebuilding this MCP process, reconnect it or start a fresh Codex task so the client discovers updated schemas. An already-running Node server keeps its previously loaded modules. Install the updated engine bundle with `wb_bridge`, then validate/reload it. The new operations use existing NET API handlers; helper classes are reloaded with the bundle.

Run the regression suite and build serially. `node scripts/smoke-mcp.mjs --live` checks MCP instructions/discovery, isolated source/asset authoring, ordered calls, loaded projects, and all ten module state probes. It closes its child MCP server and removes its own temporary fixtures. It does not save, reload, open, or modify the active Workbench project.

Add `--project-path <absolute-loaded-project-root>` to verify native project location, local prefab discovery, and a registered GUID lookup against a project containing prefabs. This extra check is also read-only.
