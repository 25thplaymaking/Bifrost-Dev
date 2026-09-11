# Bifrost Workbench Editor Operations Manual

Version: 2026-09-05
Applies to: Arma Reforger Workbench / Bifrost MCP `0.15.0-bifrost.1`

This is Bifrost's first-party reference for Workbench integration. It records what the engine exposes, what Bifrost can automate, the expected outcome of each operation, and where an apparent operation is not backed by a public API.

See [Codex Workbench workflow](CODEX_WORKBENCH.md) for `wb_capabilities`, ordered `wb_batch` calls, structured outcomes, and project source search/paging/exact edits. Capabilities distinguish configured defaults from the actual live project.

The [native agent workflow](NATIVE_AGENT_WORKFLOW.md) adds container inspection/navigation and verified scalar writes across all ten modules, Navmesh Generator navigation, bridge installation, local documentation deployment, conflict-checked Script Editor buffers, runtime log cursors, and debugger dispatch with observed UI verification. The public native API remains the capability boundary for each module.

## Evidence policy

An operation is supported only when grounded by all applicable sources:

1. Enfusion public API index (`data/api/enfusion-classes.json` and `data/api/arma-classes.json`).
2. Bundled official BI Wiki capture (`data/wiki/pages.json`: 271 pages; newest official revision 2026-08-20).
3. Packed base-game scripts and resources from `data007.pak`, inspected with PAC1CLI.
4. A protocol, parser, or Workbench compile check appropriate to the operation.

Bohemia does not distribute Workbench's native C++ implementation. “Source reviewed” therefore means the complete public script surface, shipped WorkbenchGame plugin scripts, packed serialized resources, official documentation, and live NET API behavior—not unavailable proprietary native code.

## Common editor control

All editor modules inherit `WBModuleDef`.

| MCP operation | Enfusion call | Expected outcome | Failure meaning |
|---|---|---|---|
| `wb_editor_module action=open` | `Workbench.OpenModule(EditorType)` | Module window opens | Module cannot open in the current state |
| `action=state` | `GetModule` + `GetNumContainers` | Availability and container count | Module is not open/available |
| `action=openResource` | `SetOpenedResource(path)` | Resource opens in that editor | Wrong editor, invalid/unregistered resource, or unsupported type |
| `action=save` | `Save()` | Native save returned success; verify disk | Module refused, no saveable state, or unnamed World Editor scene (use prefab operations or observed Save As) |
| `action=executeAction` | `ExecuteAction(path, keepFocus)` | Exact module-local menu action executes | Unknown path, unavailable action, or wrong context |

`keepFocus` defaults to `false` for editor-local actions, but the engine's open-module and open-resource calls provide no focus-control parameter. Those operations, plus arbitrary menu actions, therefore require `allowForeground: true`; callers cannot activate them accidentally during background work. Script compile/reload menu paths are always rejected inside custom handlers and route through `wb_validate_scripts` or `wb_reload`. A returned `false` is an error; Bifrost never presents it as success.

New `NetApiHandler` class names register when Workbench starts. Editing an existing handler can be reloaded; adding `EMCP_WB_EditorModule` requires one restart of the existing Workbench before first use. Bifrost never launches a duplicate instance to force this.

## Native NET API operations

These native endpoints are documented by the official Workbench NET API page:

| Endpoint | Purpose | Bifrost use |
|---|---|---|
| `IsWorkbenchRunning` | Health and `ScriptsCompiled` | Connection diagnosis |
| `IsWorldEditorRunning` | World Editor health and compilation flag | Editor diagnosis |
| `OpenResource` | Open a registered resource | Live resource control |
| `BringModuleWindowToFront` | Raise a named module | Opt-in only; not needed by background tools |
| `ValidateScripts` | Validate a configuration with structured diagnostics | `wb_validate_scripts`, primary script compiler/validator |

`wb_compile_status` remains the bounded `script.log` history/fallback. `wb_validate_scripts` returns file, absolute path when available, addon, line, severity, and message.

## Resource Manager

Public surface: `ResourceManager` plus common module methods. Packed WorkbenchGame source contains 21 Resource Manager plugin scripts.

Supported operations:

- `wb_resources getInfo`, `register`, `rebuild`, and `open`.
- `wb_editor_module` open/state/resource/save/action control.
- `game_browse`, `game_read`, `asset_search`, and `game_duplicate` for UI-free discovery and copying.

Re-registering an existing file is surfaced as an error, matching Workbench. The previous `browse` handler action was removed because the handler never implemented a real browse operation; discovery belongs to the indexed filesystem/PAK tools.

Reviewed shipped behavior includes resource-context plugins through `OnResourceContextMenu`, batch processing, resave tools, linked-resource discovery, and layout-class generation.

## Script Editor

Public surface: current file/line, line count, line read/write/insert/remove, open resource, save, and actions. Packed source contains 15 Script Editor plugins.

Supported operations:

- `wb_script_editor getCurrentFile|getLinesCount|getLine|setLine|insertLine|removeLine|openFile`.
- `wb_reload` uses Workbench 1.8's native `ReloadScripts` endpoint, then requires both a new compile marker and handlers returning. The native request may disconnect or time out while Workbench rebuilds the module; neither transport outcome is success until both confirmation signals arrive. It never reloads scripts inside a custom NET API reply and has no window, focus, or UI Automation dependency. Plugin-only reload remains in the custom handler because it does not tear down that handler's script module.
- `wb_validate_scripts` calls native `ValidateScripts` with a 60-second budget.
- `wb_compile_status` reads bounded compile history without changing/focusing Workbench.

Engine behavior: line indices are zero-based; each text edit creates its own undo step; a new handler class name needs one Workbench restart; a reload timeout returns diagnostics and never launches another Workbench.

The MCP Script Editor schema now explicitly uses zero-based indices and preserves the handler's `currentFile`, `lineText`, and `linesCount` response fields. Disk-source `project read` uses one-based display lines.

Reviewed shipped plugins include autocomplete, formatting, class rename, new-script creation, Doxygen filler, templates, and autotest execution.

## World Editor

Public surface: `WorldEditor` and `WorldEditorAPI` (84 indexed methods), plus common module control. Packed source contains 61 World Editor scripts/tools.

Supported families include entities, components, layers, terrain, scenarios, prefabs, project state, play/edit state, and common editor control. Related entity mutations are wrapped with `BeginEntityAction()` / `EndEntityAction()` so the editor remains consistent and undo/redo treats them as one batch.

`wb_prefabs locate` scans local source files on the MCP host with explicit bounds and partial-coverage markers. It does not call shipped `LocatePrefabsFromPath`, whose `FileIO.FindFiles` can block Workbench behind a script authorization dialog. `getGuid` uses the verified `prefabPath` parameter. `wb_projects list` recognizes the native `Loaded Projects` response; `locate` sends an absolute `path` and handles a project root's trailing separator.

Reviewed tool families include coordinates, forests, ground manipulation, lakes, navmesh, object brush, parallel shapes, power lines, prefab generation/management, rivers, roads, rotation, shape areas, terrain creation, vectors, and walls. These names are not assumed to be menu paths; a live action is exposed only after its exact path and result are verified.

## Localization Editor

Public surface: 19 indexed editor methods plus common module control and plugin hooks.

`wb_localization` covers the exposed table/translation operations; `wb_editor_module` opens resources, saves, and runs verified actions. Disk CSV/string-table edits remain separate and require serialization validation before registration.

## Animation Editor

Editor-specific script surface: common `WBModuleDef` only. Packed formats confirm:

| Extension | Role |
|---|---|
| `.agr` | source/control graph, variables, commands, graph links |
| `.ast` | animation set template |
| `.agf` | runtime/node graph |
| `.asi` | animation set instance and animation mappings |
| `.aw` | workspace |

`animation_graph` authors, inspects, cross-validates, guides setup, suggests improvements, and covers vehicle variables, seats, suspension IK, shock absorbers, steering linkage, wheel chains, and turret options.

Workflow: author/inspect → register/rebuild → open `animationEditor` → open resource → verified actions → save. Bifrost does not fabricate compiled graph caches; Workbench owns compilation.

## Audio Editor

Editor-specific script surface: common `WBModuleDef` only. Packed `.acp` files and official pages define the graph.

Verified node families include sounds, mixers, constants, shaders, amplitudes, local banks/samples, spatiality, connections, signals, variables, generators, playlists, aux outputs, and final-mix routing.

`editor_asset createAudio` creates an original minimal graph:

`trigger → sample bank + amplitude + 3D spatiality → shader → sound event → FinalMix`

The sample must be a registered `{GUID}path.wav`. Volume is bounded to -96..+12 dB, looping is explicit, output parses before write, and overwrite is off by default. Advanced DSP, randomization, variables, signals, directivity, and aux tuning remain inspectable but are not guessed by the generator.

## Particle Editor

Editor-specific script surface: common `WBModuleDef` only. Packed `.ptc` files serialize as `EffectDef` with `EmitterDef` blocks.

Verified properties include shape, cone, birth rate, material, size, rotation, velocity, air resistance, wind, gravity, animation FPS, lifetime, curves, emitting time, and repeat.

`editor_asset createParticle` produces a conservative one-emitter effect with bounded rate/lifetime and a registered material reference. Complex multi-emitter/collision/light behaviors require their exact packed schema before new operations are added.

## Procedural Animation Editor

Editor-specific script surface: common `WBModuleDef` only.

- `.siga` is `ProcAnimSignalResClass`: inputs, operations, outputs.
- `.pap` is `ProcAnimProjectClass`: bone transforms referencing a model and signal graph.

`editor_asset createProcSignal` builds input → smoother → output. `createProcRotation` builds signal → rotation maker → rotation setter for one named bone. Both omit compiled caches. References must have real Workbench GUIDs, preventing placeholders from being presented as complete assets.

## Behavior Editor

Editor-specific script surface: common `WBModuleDef` only. The Wiki documents the editor and nodes, but no behavior graph extension or serialized behavior resource was found in the inspected `data007.pak` set.

Bifrost supports open/state/resource/save/action control but does not fabricate a behavior graph. The authoring gate is: find real resources, inspect multiple variants, map node/link semantics, round-trip them, then add creation.

## Structured resource inspection

`editor_asset inspect` reads `.acp`, `.ptc`, `.pap`, or `.siga` from a mod or packed game data. It enforces a 2 MB bound, parses Enfusion serialization, and reports root, node/property counts, and node-type frequency. Animation formats use the richer `animation_graph inspect` path.

## Safety and focus contract

- Ordinary tools fail fast and never auto-launch Workbench.
- Only explicit `wb_launch` may install handlers or launch.
- Existing Workbench is reused; no duplicate/headless instance is created.
- Editor actions default `keepFocus=false`.
- Handler error responses become MCP errors.
- Uppercase native `ERROR`, blocked mode guards, and unconfirmed save timeouts are errors too; common tools include structured engine data and outcomes.
- Destructive global menu paths stay blocked.
- Generated assets use contained paths, validated identifiers/references, parse-before-write, and no overwrite by default.
- MCP registration uses direct Node plus the built server—no `cmd /c` or `npx` wrapper.

## Acceptance gate for new operations

1. Record the public symbol/signature.
2. Identify the bundled Wiki reference.
3. Inspect shipped script or multiple real resources when a format is involved.
4. Define both success and false/error outcomes.
5. Pass parser and protocol smoke tests.
6. Compile changed Workbench scripts in the already-running Workbench.
7. Report registration/restart requirements without opening another Workbench.

## Implementation map

| Concern | Source |
|---|---|
| NET API/fail-fast behavior | `src/workbench/client.ts` |
| Common editor control | `src/tools/wb-editor-module.ts`, `EMCP_WB_EditorModule.c` |
| Native script validation | `src/tools/wb-script-compile.ts` |
| Compile log correlation | `src/workbench/compile-log.ts`, `src/tools/wb-compile-status.ts` |
| Audio/particle/procedural tools | `src/tools/editor-assets.ts` |
| Animation/IK suite | `src/tools/animation-graph.ts`, `src/animation/*` |
| Resource operations | `src/tools/wb-resources.ts`, `EMCP_WB_Resources.c` |
| Wiki bundle/updater | `data/wiki/*`, `scripts/apply-wiki-capture.mjs` |
| End-to-end smoke | `scripts/smoke-mcp.mjs` |

This manual is versioned with the server. Changes to the engine API, Wiki capture, packed schema, or observed live behavior must update the corresponding section and evidence date.
