# Mission tools implementation and acceptance

Source shortlist: [shared interaction, final message](https://chatgpt.com/s/cx_6a9b1e6b6ee88191a763c85e02646b3d). Selection was delegated by the current request. The decision record is [MISSION_TOOLS_DECISION_RECORD_2026-09-04.md](MISSION_TOOLS_DECISION_RECORD_2026-09-04.md).

## Selected outcomes

Open **CREATE > Lightning > Bifrost** and choose a mission action, or use **GM context menu > Mission Tools**. Hide Terrain Objects is a native placeable system; its radius and enabled state are in Edit Properties. Other CREATE ground tools ask for a terrain click; object tools use the current selection or ask for an editable-object click. Context-menu position tools use the context-click position. Apply reports the server's result. Close or Back dismisses the editor; Escape cancels pending targeting.

| Shortlist | Feature | Implemented behavior and limits |
| --- | --- | --- |
| P0 | Hide Terrain Objects | Native editable SYSTEM with a 5–100 m radius, on/off property and persistent GM circle. Native placement uses the editor's SYSTEMS budget. Each machine hides static mesh scenery, including buildings/trees and mesh children, disables physics interaction layers, and tracks overlapping claims and newly streamed objects. Resize, movement, disabling and deletion release obsolete claims; Restore Hidden Terrain removes all areas. Editable props, characters and vehicles are excluded. Terrain height and baked navigation are unchanged. |
| A1 | Scale Object | Absolute uniform scale, 0.25–4, on selected standalone static editable props. Excludes characters, vehicles, buildings, dynamic/kinematic bodies and entity hierarchies. Explicit replicated scale state compensates for native editable transforms retaining local scale. Existing composition placement applies this state to eligible props. |
| A2 | Create/Edit Intel | Binds native Read Intel interaction to one supported prop. Title up to 64 characters; body up to 2048. Discovery awards a native map-journal entry to the finder, their faction, or everyone. Server stores audience-bound awards for the running mission and reconstructs authorized entries on reconnect, map opening and faction change. Optional clue deletion; duplicate collection prevented by player identity. Existing settings load through a GM-only reply. Secret body/scope/deletion settings are not replicated as public helper properties. |
| A3 | Global Hint | Authored title/body delivered to currently connected players through native hints. Respects native hint preferences. Transient; not replayed to later joiners. |
| A4 | Chatter | Authored text attributed to one selected living, conscious AI speaker, or the GM's faction HQ with no selection. Audience: everyone, speaker faction, or players within 100 m. Uses the native chat feed, marked [AI]; no synthesized speech or radio audio generation. |
| A5 | Make Invincible | Explicit enable/disable of native damage handling on supported selected entities, with replicated state. Optional current vehicle occupants. Does not repair previous damage or automatically protect later occupants. |
| A6 | Create LZ / Create RP / Create Target | Named positions in the existing authoritative server marker library. Use Named Position adds a native editable Move waypoint to selected AI groups, or moves supported strike/mortar/task-zone centers. Names are reusable coordinates; LZ does not itself launch an autonomous helicopter landing service. Existing pass snapshots remain unchanged until their next pass. |
| A8 | Create Teleporter | Bind two supported props using the same link name. Native interaction names the destination. Server validates the living conscious requester, proximity, sight line, on-foot status, paired endpoint and destination clearance/terrain/water. Five-second player cooldown and 1.5-second endpoint arrival spacing. Third endpoints are rejected. Remove Intel / Teleporter removes the helper while preserving the prop. |

There are at most 64 active intel/teleporter helpers combined and 256 distinct awarded intel records per running mission. Authored world helper state is session state; saving/reloading it across server restarts is not provided by this change. Existing composition storage remains separate.

## Deferred selection

**A7 Damage Buildings** was not approved for implementation. Installed `SCR_DestructibleBuildingComponent` exposes destruction/collapse replication and serializes its destroyed flag without a symmetric building restoration path. A health reset would not establish restored geometry, debris or navigation. Implementing a one-way damage button would omit the shortlist's restoration requirement.

The conditional B1–B10 list was not selected. No additional dependencies or arbitrary script execution were introduced.

## Implementation ledger

- Added four mission-tool script files: editable/terrain state, native player interactions/journal delivery, authoritative operations/owner replies, and one shared editor panel.
- Added two runtime-replicated helper prefabs and one Bifrost layout, each with resource metadata.
- Integrated Mission Tools into the existing context bridge, controller lifecycle, Back handling, hover/theme treatment and canvas cues.
- Replaced the old machine-local terrain-hide implementation and routed the existing invincibility action through replicated editable state.
- Reused marker ownership and identifiers for named-position lookup; used editable native waypoints for movement orders.
- Preserved all pre-existing uncommitted changes. No commit, push, release, Workshop publication or dedicated-server restart was performed.

## Initial implementation evidence

Evidence sequence: Enfusion MCP API/knowledge inspection, the full Enfusion Engine **Replication overview** through the MCP's indexed documentation, then PAC1CLI reads of the locally installed Reforger 1.8.0.13 `data007.pak`. Installed sources checked include `SCR_EditableEntityComponent`, `SCR_DestructibleBuildingComponent`, `SCR_JournalConfig`, `SCR_MapJournalUI`, player identity helpers and native editable Move waypoint resources.

- Native WORKBENCH script validation and final focus-free reload: **zero errors, 14 existing base-game warnings**, matching the pre-change baseline.
- Also checked the full BI Community Wiki [Multiplayer Scripting](https://community.bistudio.com/wiki/Arma_Reforger:Multiplayer_Scripting) page through MCP's local wiki index: runtime prefab proxies, owner RPC delivery, property synchronization and streaming/JIP callbacks match the selected architecture.
- Native resource inspection: both helper prefabs load with their intended components; the interaction prefab includes the native action configuration.
- Actual hidden engine layout instantiation: **16/16 required widgets found**. The disposable layout was removed immediately. Malformed hierarchy/unsupported widget properties found during development were corrected before this check.
- Final whitespace/structure checks passed; all three new assets have metadata and the repository's 395 metadata resource GUIDs are unique. No new mission script has a non-constant static field initializer. A source-wide scan also found three pre-existing scalar ID initializers in compositions, markers and vehicle service; they were left unchanged.
- Temporary Workbench handler files are absent, confirmed by the cleanup tool and filesystem check. Post-cleanup native validation still passes with the same 14 warnings. Disposable inspection commands exited; no additional Workbench process was launched.
- These initial checks establish compile/resource/layout validity. Subsequent local gameplay evidence is recorded below; dedicated-server, remote-client, JIP and controller-navigation acceptance remain unverified.

## Follow-up fixes and local evidence

The [workflow review](MISSION_TOOLS_WORKFLOW_REVIEW_2026-09-04.md) compares all 13 actions against the actual Arma 3 source workflows and records material gaps. LZ/RP/Target are still marker-library coordinates rather than selectable logic systems; LZ has no AI landing pad and Target has no laser designator. Teleporters connect two existing props rather than offering a destination network. These are not full Arma 3 module equivalents.

- Local Workbench playtest verified Hide placement, selectable properties, visible scenery removal, OFF restoration and radius reduction from 35 m to 15 m. QRF placement also succeeded on clear terrain. Overlap, movement/deletion, collision traversal and remote/JIP behavior require further runtime acceptance.
- Fixed the reported `RefreshPositions` null reference: the mission panel now owns copies of displayed marker records after the service replaces its snapshot. Selection follows the marker ID through rename/reorder; deletion requires an explicit new choice.
- Correlated mission result and private edit replies with the active request. Closed/reopened panels ignore old replies; duplicate replies cannot finish another operation. Server edit/save routes reject accidentally changing a prop between intel and teleporter purposes.
- Native reload compiled the production changes and isolated test probe with zero errors and 14 base-game warnings. The isolated engine regression returned **18 passed, zero failures**. Two consecutive named-position saves then succeeded in a fresh local GM playtest at 19:47:46 and 19:47:55, with no recurrence of the reported exception.
- The fresh CREATE view showed **Bifrost: 21** and **FX: 8**, with Hide Terrain Objects using the system icon and compositions absent from CREATE. The position editor displayed its bordered name field, persistent caption, example, limit and numbered steps; closing it returned to GM.

## Required runtime acceptance

Use a dedicated server, a remote GM and at least two ordinary players in different factions. Repeat with a listen host separately; do not substitute it for the dedicated test.

1. Open every Mission Tools action at full and compact viewport sizes. Check multiline entry, scope/include controls, marker paging, readable status, hover/focus and Back. Verify right-clicks while the panel is open do not open another world menu.
2. Hide two overlapping areas containing structures and trees. Check visibility and physical passage on server/remote clients. Stream away and back, join with a fresh client, then restore and verify original visibility/collision. Confirm terrain height and baked navigation are unchanged.
3. Scale eligible props to both limits. Move them afterward, reconnect and place a saved composition containing them. Compare transforms across machines. Confirm excluded objects are rejected without modification.
4. Toggle invincibility on/off for characters and vehicles, with and without current crew. Apply actual damage from remote clients; verify reconnect state and restoration of damage handling. Check subsequent occupants follow the documented behavior.
5. Create finder/faction/everyone intel. Read it as ordinary players and inspect their map journals, including a journal already open. Reconnect, change faction, edit the clue, reread and exercise optional clue removal. Check unauthorized audiences receive no body text; base briefing entries remain present and no duplicate buttons accumulate.
6. Send global hints and scoped chatter with selected AI and HQ. Verify exact audiences, hint preferences for hints, native chat-feed delivery for chatter, text limits, invalid speakers, no-body nearby HQ rejection and that later joiners receive no expired transient messages.
7. Create LZ/RP/Target markers, rename/delete them through the existing library, and verify the picker refreshes. Apply coordinates to supported AI groups and action centers. Verify waypoint visibility, movement and subsequent strike/task behavior on authority and remote GM after reconnect.
8. Link two teleport props. Test both directions, moved/deleted/unlinked endpoints, walls, water, slopes, occupied arrival space, unconscious/dead/seated users, simultaneous users and cooldowns. Verify all player transforms come from the server and nearby native actions still work.
9. Repeat requests after removing GM rights; operations and private edit replies must be denied. Disconnect/reconnect during interactions and inspect fresh logs for replication, UI and script errors. Delete helpers/targets and end the mission; verify scheduled callbacks and cached references are released.
