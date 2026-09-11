# Native Workbench workflow for Codex and Astra

Use the existing MCP and native Enforce bridge to take a task from evidence to a verified compiler result. All models use the same interface; there is no model allowlist. The user's existing authorization covers ordinary editor work. Opening windows uses `allowForeground: true` when that activity is authorized.

## Establish the actual target

1. Call `wb_capabilities` with `live: true`. The native project list remains available when custom handlers are missing.
2. Use `wb_projects locate` with the intended absolute addon root. Match its native project ID; configured defaults alone are insufficient.
3. Use `wb_bridge status` with that root. `install` updates the managed scripts in `Scripts/WorkbenchGame/EnfusionMCP`, preflights the whole bundle and reads back each change. `dryRun` previews installation.
4. Validate with `wb_validate_scripts` (`configuration: "WORKBENCH"`), then use `wb_reload` and verify `wb_editor_module state` reports `bridgeVersion: "0.15.0"`.

Installation is separate from compilation and handler availability. A live native endpoint with missing handlers never justifies starting another editor. New handler classes can require a process restart; this upgrade routes its new operations through existing handlers. Preserve user edits before any necessary restart. Normal tool calls never launch Workbench.

On local Windows connections, the MCP checks for duplicate Workbench processes before native requests, using a bounded two-second cache. Multiple sessions are rejected rather than choosing whichever instance owns the NET API port. This does not prevent a person or Steam from launching another instance; preserve unsaved work and keep one intended session before continuing.

## Inspect, navigate, edit, verify

`wb_editor_module` covers Resource Manager, Script Editor, World Editor, String/Localization Editor, Animation, Audio, Particle, Behavior, Procedural Animation and Navmesh Generator. Start with `state`; use `open` or `openResource` if needed, then `containers` and `inspect`. `close` uses the native module lifecycle and can present an unsaved-change dialog; save or inspect those changes first.

Inspection returns native values and property types, widget hints, defaults, local overrides, range limits, enum choices, object base classes, child/component counts, and paths for nested objects. A default is never substituted for an unreadable current value. `readable: false` is a real boundary; use source inspection or the editor UI for that type. Strings are bounded, and native type codes are retained for less common types.

Navigate using the returned `nodes[].path`, such as `component:0/object:Config/array:Items:0`. Pass the same root selector (`containerIndex` or `entityId`) with each path. The path selects an object relative to that root. `offset` pages properties; `childOffset` pages their combined object/array/child/component list. Keep the property offset unchanged while following `nextNodeOffset`; `nodeCount` describes that list. Each reply has at most `limit` properties and `limit` nodes. `available: false` means an empty editor slot, not an inspectable object. World entities can initially be inspected by index; use the returned opaque `entityId` thereafter. Prefab mode includes the world root at container 0; inspect the other containers instead of assuming slot 0 is the prefab.

For a property change pass:

```json
{
  "module": "worldEditor",
  "action": "setProperty",
  "entityId": "<ID returned by inspect>",
  "objectPath": "component:0",
  "propertyKey": "<name returned by inspect>",
  "valueType": "number",
  "value": 2.5,
  "expectedTarget": "<target returned for this exact object>",
  "expectedValue": "<raw value returned by inspect>"
}
```

The plugin rejects changed targets, changed values, incompatible types, and numbers outside the native ranges/enum choices. It supports strings, resources, integers, numbers, booleans and three-component vectors. Object and object-array traversal is native; creation/removal remain available through the existing entity tools or source authoring. Native target fingerprints are short-lived optimistic conflict checks, not persistent IDs. Re-inspect after reload, navigation, deletion, undo, or world changes.

Nested World Editor writes pass the root container plus the engine's component/object/array property path. To edit a child entity, use its returned `entityId` as a new root. Generic module `save` refuses an unnamed world, including a prefab's unnamed scene, because native Save can open a blocking Save As dialog. Use prefab template operations for a prefab and the observed Save As control when creating a world.

World property edits use native edit actions. Other modules use `BaseContainer.Set` and return the observed result. Check `applied`, `verified`, `beforeValue` and `afterValue`. Save explicitly and verify the serialized file before calling it a disk change; some editor-owned containers do not expose a writable/serializable surface. Batches and multi-file installations are not transactions. A failed request may already have applied an edit; inspect before retrying. Requests from one MCP process are ordered, but multiple independent MCP processes must coordinate shared Workbench use.

## Work with source and unsaved buffers

Use `api_search`, then `wiki_search`/`wiki_read`, then shipped game source through `game_read` to verify engine symbols. Use `project search` for implementation references and `project read` for source and hashes. File lines are one-based; Script Editor lines are zero-based.

`wb_script_editor readRange` returns the actual editor buffer, including unsaved edits, along with `currentFile` and `linesCount`. `setLine`, `insertLine` and `removeLine` require `expectedFile`, `expectedLinesCount` and `expectedText`. They reject an unexpected tab or changed text and confirm the resulting buffer. These operations accept one line at a time and work without a loaded world. Workbench 1.8.0.13 exposes **Save All**, while its inherited single-file `Save()` returns false. Inspect other unsaved tabs before `saveAll`, pass `expectedFile` and `allowForeground: true`, then read and compare the intended disk source before compiling. A save dispatch alone is not disk verification. Never replace an unsaved buffer by assuming the disk copy is current.

For larger source changes use conflict-checked `project edit` with `expectedSha256`, or `write` for generated files. Do not silently overwrite a buffer open in an editor. A whole-file write is not a graph compiler; `.acp`, `.ptc`, `.pap`, `.siga`, animation, material, layout and world sources must also be loaded/validated by their owning editor.

## Debug with observed runtime evidence

`wb_debug diagnostics` reads bounded `script.log`, `console.log` or `error.log` from the newest Workbench log session. It returns raw context, parsed errors/warnings, a `nextCursor`, and explicit missing/partial/reset information. Pass the cursor to continue without rereading old errors. A rotated/truncated log resets the cursor. `snapshot` adds the current Script Editor file and line without requiring the custom state handler to succeed.

`controls` returns documented debugger menus and shortcuts. `togglePanel` toggles Callstack, Watch, Breakpoints, Output, Errors or the source-search panels through the native editor's checkable Window menu. Observe visibility first: toggling an open pane hides it, and a dispatch does not confirm visibility. `command` dispatches a documented debugger action through the existing bridge. A dispatch is not proof that a breakpoint was hit or a step finished.

Observe the intended Script Editor through the available computer-use tool before controlling the debugger. Match the current file/line, confirm the intended target is attached and paused, then inspect Callstack and Watch. For stepping, pass `runtimeObserved: true` and the exact `expectedFile`. The default `debugTarget: "workbench"` returns an explicit computer-use route: a paused Workbench cannot service its own script API. Use the observed F10/F11/Shift+F11/F5 controls, then inspect again. `debugTarget: "external"` dispatches native menu actions for an observed game/server attachment. Breakpoints require the observed zero-based `expectedLine` as well as the file; toggles are never blindly retried. The live menus are **Debug Game**, **Debug Workbench**, **Debug Custom**, and **Toggle Breakpoint**, under **Debug**; panes are under **Window**. Debug Custom attachment fields and watch entry use the observed UI. The public script API does not expose attachment state, stack values or watch evaluation.

Use native `wb_validate_scripts` for syntax/module diagnostics. Use `wb_reload` for compilation/reload with a fresh log marker and restored bridge ping. Compile/reload is forbidden inside a custom handler's menu reply because it can invalidate live serialization objects. Source parsing, a successful module compile, a runtime breakpoint session, and multiplayer/JIP testing are separate evidence.

Before script reload, save and close auxiliary editors that hold runtime resources. Live 1.8.0.13 checks found that Animation and Navmesh editors explicitly block reload; leaving the restored Audio Editor open retained audio configuration resources and produced an engine assertion. `wb_reload` now includes native errors without file locations and returns an error for unconfirmed completion. It never closes editors or retries a failed reload automatically. Ordinary Resource Manager and source operations do not need a World Editor scene.

## Documentation and repeatable acceptance

`wb_docs list/read` exposes these bundled guides through tools; they are also MCP resources under `enfusion://workbench/docs/`. `deploy` installs them into an explicit project's `Docs/EnfusionMCP` directory, preserving different existing files unless `overwrite: true`. Use `project write/edit` for task-specific design notes, observed API signatures and validation evidence. Deployment here means local files; Workshop publication is separate.

A complete task records its actual addon, changed files and hashes, inspected native targets, operation outcomes, compiler diagnostics and unresolved runtime evidence. For bridge acceptance, exercise native discovery, nested inspection, a rejected stale write, an actual reversible property edit/readback, live buffer editing, documentation deployment, an intentional isolated syntax error, its correction, a successful native compile and a recovered bridge. Remove test-only source after verification. Do not inject a deliberate error into the user's real implementation.

`node scripts/validate-native.mjs --project-path <absolute-loaded-addon-root> --allow-editor-mutations` runs that native acceptance against an empty World Editor with no source tabs open. It creates uniquely named fixtures and closes auxiliary editors before reload. It removes owned fixtures only after their editor targets close; a timeout or changed target preserves them and reports failure. Do not run it during release work. The 2026-09-05 run passed editing and compiler failure/recovery, but cleanup needed intervention after generic Save stalled; that Save call was removed. The revised close-only cleanup has not been rerun. Run serially and observe any native unsaved-change dialogs instead of retrying them blindly.

Engine references: [BaseContainer API](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceBaseContainer.html), [WorldEditorAPI](https://community.bistudio.com/wikidata/external-data/arma-reforger/EnfusionScriptAPIPublic/interfaceWorldEditorAPI.html), [Script Editor debugging](https://community.bistudio.com/wiki/Arma_Reforger:Script_Editor#Debugging). Material validation uses the shipped `ValidateMaterialPlugin.c` routines against the requested resource, not the built-in handler's unrelated UI selection index.
