# Startup movement and UI latency investigation

## Status

The reported regression is not yet attributed to a measured function. The user confirmed that it starts immediately when the game starts, before using Scale Object, Arsenal or FX. No gameplay or UI source was changed during this investigation. Removing a feature or changing its update rate without frame evidence would not establish a fix.

## Scope and decision

REQUIREMENTS
- Investigate delayed movement and UI responses after the recent changes.
- Trace work active at scenario startup, distinguish it from tool-triggered work, and preserve existing features.
- Use MCP, BI documentation and PAC1CLI; distinguish source inspection from runtime measurement.
- Leave hands-on testing to the user.

MINIMUM COMPONENTS NEEDED
- Existing source history, Workbench logs, native script definitions and a profiler capture from the affected session.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No speculative rollback, disabled features, graphics-setting changes, additional services or permanent instrumentation.
- Frame timing is required to identify whether the sustained delay is script, rendering or another engine cost.

PRIMARY RISKS
- Mistaking a potentially expensive code path for the demonstrated cause.
- Inferring frame performance from compiler success or from an idle Workbench process.

REQUEST INTERPRETATION
- Investigate the regression across movement and UI, including the recent 1.0.29 changes and subsequent local fixes.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Establish the cause with evidence before changing behavior.

## Evidence inspected

- Branch `bishop/runtime-scale-all-entities`, HEAD `8cff2a0`; initial working tree clean.
- Compared `dfb34df..HEAD` for the release changes, and `1ed7e47..HEAD` / `9de05e5..HEAD` for the later input, scale and tracer fixes.
- Workbench `logs_2026-09-04_22-58-07/script.log` and `console.log`: the current Game script CRC `d0d97080` compiled at 23:43:08. The GM opened at 23:44:56; the game was destroyed at 23:45:50. There are no recorded scale requests or spawned FX during this short session. Absence of those messages is not an inventory of entities already in the world.
- In the 23:44–23:45 console slice, 21 managed-texture errors accompany 18 explosive-icon loads and three map-icon loads. These are intermittent, not a continuous per-frame error flood. There are also serialized-resource warnings. Logs do not establish their frame cost.
- The newer `logs_2026-09-04_23-46-38` records startup and compilation, but no comparable GM play interval or frame profile at inspection time.
- Only one matching Workbench/game process was present. A two-second idle sample showed roughly 0.125 CPU seconds for Workbench and about 11.8 GiB free physical memory. This does not characterize the earlier affected play session or GPU load.
- MCP `wb_state` failed with “Handler scripts not loaded” and a stale connection. No launch, live reload or game-control operation was attempted.

Logs are under `C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/logs/`.

## Source findings

| Path | Activation and observed work | Interpretation |
|---|---|---|
| `DCO_GMMissionEntityState.c` scale | The replicated scale starts at zero. POSTFRAME is registered by an explicit non-unit scale application; the handler changes scale only when the actual value differs. | No demonstrated explanation for untouched startup. Moving scaled entities still need separate frame/physics measurement. |
| `DCO_TracerEmitter.c` | New trace, sound, visual and broadcast work occurs for a firing emitter. | Not an unconditional startup loop. Cost depends on active emitters and rate. |
| Recent button guards | Reject non-primary clicks; no timer or delay added. | No code-level mechanism for sustained startup latency found here. |
| `DCO_GMCreatePanelComponent.c` | Search poll is 400 ms and returns without repaint when unchanged. Scroll drag starts on mouse-down and stops on release, focus loss, hide, refresh and shutdown. Text fitting forces several measurements after a repaint. | Long names can add cost to scrolling/repainting. This is not an idle text-fitting loop. |
| `DCO_PlacementCatalog.c` | Source identity construction and comparison use linear membership searches, producing quadratic work for large catalogs. Called during build/change handling, not each search-poll tick. | Candidate for startup/catalog-transition hitches; sustained idle delay remains unproven. |
| `DCO_GMRenderManager.c` / `DCO_GMAwarenessCue.c` | Overlay update runs every 16 ms while GM is active. Recent release added replicated system cues; their work scales with placed triggers, zones and FX. Character and selection caches have separate refresh intervals. | A profiler priority if the loaded scenario contains many systems or units. The startup entity inventory is unavailable. |
| `DCO_GMEditTreeComponent.c` | Rebuild and force overview run every second; iterate editable entities and count ammunition periodically. | Cost scales with scenario population. Predates the latest two fixes. |
| Terrain hide areas and interaction helpers | Area scans require instantiated areas. Arsenal and mission interaction discovery return early when their registries are absent/empty. | Not world-wide scans added unconditionally to an empty scenario. |

## Documentation and native comparison

MCP scripting guidance recommends profiling hot paths, avoiding expensive repeated world queries and reducing allocation in frequently called code. The MCP copy of BI's [Script Profiling](https://community.bistudio.com/wiki/Arma_Reforger:Script_Profiling) documents **Diag Menu → Statistics → Script Profiler**, with frame or continuous modes. A direct web fetch returned HTTP 403; the documentation content was available through MCP.

PAC1CLI read the installed `data007.pak` native `scripts/Game/Editor/Components/EditableEntity/SCR_EditableEntityComponent.c`. Native transform handling routes moving characters/vehicles through their owner and preserves scale in the static broadcast path. The native source does not supply performance measurements for Bifrost's scale callback.

## Remaining measurement

Capture FPS and the busiest functions in the continuous Script Profiler while the immediate delay is present, using the same scenario and view. Keep tools unused for this baseline. This distinguishes initial catalog construction, repeated overlay/panel work, and a low-script-cost rendering/engine slowdown. The user has been asked for that capture.

No performance improvement, live runtime pass, dedicated-server pass, remote-client pass or JIP pass is claimed by this source/log review.
