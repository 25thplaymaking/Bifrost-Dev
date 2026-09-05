# Bifrost field-test corrections

REQUIREMENTS
- Scale characters with adjustable movement speed, proportional health, and usable third-person cameras.
- Place teleporters freely as editable objects; reopen settings, move and delete them through normal GM controls. Configure automatic/manual use and travel timing.
- Render object properties in Bifrost's settings interface without losing editable attributes.
- Deliver non-obstructive GM messages to chosen audiences with configurable duration.
- Supply usable crowd, speech, yelling, dog, gunfire and battle ambience with volume, fades and spatial obstruction/reverberation controls. Arma 3 provenance is optional per clarification.
- Keep gameplay server-authoritative and replicate configuration for remote clients and JIP.
- Validate scripts/resources and state the separate hands-on and multiplayer evidence boundaries.

MINIMUM COMPONENTS NEEDED
- Extend the existing mission panel, replicated entity state and native character hooks.
- An editable teleporter prefab using the existing interaction component and placement system.
- Extend the existing attribute renderer and owner-targeted message delivery; one passive hint layout.
- A bounded sound catalogue and positional playback component using native audio resources.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No external service, generic framework, or new dependency is needed.
- Do not distribute recordings without verified reuse terms.

PRIMARY RISKS
- Native movement/camera behavior at extreme scale, compatibility with damage and animation systems.
- Teleporter deletion/relinking while travel is pending, automatic return loops, collision-safe arrivals.
- Unsupported compound object attributes must stay fully editable.
- Audio resource availability and actual acoustic behavior require runtime listening.

REQUEST INTERPRETATION
- Implement all reported corrections locally. Do not publish a release as part of this request.
- Use linear health scaling, preserving existing injury percentage; provide independent speed adjustment.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Finish the reported GM tools as visible, amendable features and correct scaled-character behavior, while retaining dedicated-server authority.

Baseline: commit 8cff2a0; Workbench native validation passed with 14 existing warnings. Existing untracked startup investigation is unrelated and preserved.

## Implemented behavior

- **Scale:** the existing scale panel now has a movement multiplier. A value of 1 follows character size, while 2 requests twice that size-adjusted speed. Maximum health on character hit zones scales linearly from a cached baseline, preserving injury percentage. Restoring size 1 restores baseline health capacity. The third-person collision solver normalizes rotation before inverse/quaternion calculations and scales camera translation, boom and trace dimensions.
- **Teleporters:** a catalogue prefab supplies independent GM-editable endpoints. Double-click opens Bifrost's endpoint settings. Each point has a title, pair link, manual/automatic activation, 0–60 second delay and 1–20 metre use radius. Movement/deletion use ordinary GM controls. Travel is server-owned; leaving cancels a pending trip. Pair changes and disconnected/replaced characters are rechecked. Arrival guards prevent immediate return loops. Destination checks prefer the exact placed location, preserve floor/roof elevation and reject blocked arrivals. The old Remove Teleporter operation is no longer offered.
- **Object settings:** valid attribute layouts keep their session inside Bifrost's settings panel. Existing Bifrost controls handle common attributes; compound controls retain their native behavior inside that panel rather than forcing the entire session back to the base-game dialog.
- **Global hints:** an unfocused, disabled card displays a title and message beside the action, with 1–300 second duration. GM selection targets all players, selected players or factions of selected units. Messages use reliable owner-targeted delivery after server audience validation.
- **Audio:** six supplied recordings cover crowd, conversation, dog, shouting, battle and individual gunshot. Nine settings control selection, playback, volume, radius, fade-in/out, looping, wall muffling and native reverb. Configuration is replicated; listener-specific distance/obstruction and playback stay local. See `Sounds/Bifrost/README.md` and `sources.json` for source terms, conversion and acoustic limits.

## Verification record

- Native `WORKBENCH` script validation: passed with zero errors and 14 existing warnings, including the final teleporter feedback correction.
- Final native PC builds at 15:25:02–15:25:03: all 12 requested sound/hint/teleporter/settings resources built successfully. Earlier graph/prefab import diagnostics were corrected before this pass.
- `python Tests/verify_feature_layouts.py`: 363 binding checks across 26 layouts, zero failures, two fault-injection checks passed.
- `python Tests/verify_field_resources.py`: all six audio files, hashes, durations, formats, metadata, graph structure and nine distinct native attribute types passed.
- Final local Arland GM field probe at 15:24:36: **25 checks passed, no failures**, including visible hint dimensions/text/teardown, six registered sound events, six valid native playback handles beside the listener, three acoustic signals and the standalone teleporter's editable target/attribute. Playback handles were immediately terminated; this is not a listening-quality test.
- An earlier runtime failure identified four reverb-to-control connections encoded as constant inputs. They now use native signal connections (type 9); the file checker guards this distinction. Native audio node identifiers were also corrected and the emitter now contains its required signal manager. The successful final probe supersedes earlier import-only results.
- Early hint measurements ran before the game viewport finished opening. They passed once the native startup dialog was resolved and the game finished loading; no hint-sizing patch was needed.
- Workbench reloads hit a native `GameApp.cpp:1287` resource-leak assertion while reloading a loaded test world with editor resources open. Fresh startup reached the final passing probe. The cause of the editor reload assertion has not been established; it is not evidence of a successful reload or of multiplayer readiness.
- PAC1CLI was unavailable in this session; engine resource builds used Workbench's native PC build API after MCP and BI documentation/source checks.
- No release, Workshop upload, dedicated-server run, remote-client run or join-in-progress run has been performed.
- Temporary Game and WorkbenchGame test/bridge scripts were removed from the addon; post-cleanup native validation passed with zero errors and the same 14 warnings. Automatic approval review rejected removal of the external temporary audio-authoring folder (`C:/Users/Bryce/.codex/tmp/bifrost-audio-authoring`); its conversion dependencies remain outside the addon and are not shipped.

## Combined hands-on acceptance

Use a disposable GM scenario, a dedicated server and two clients; add a late joiner after the initial configuration. Keep listen-server evidence separate.

1. At character scales 0.25, 1, 2 and 10, measure forward/sprint movement at speed multipliers 0.5, 1 and 2. Check AI movement independently. Confirm the engine honors speed fractions above 1, injury percentage survives each resize, health capacity follows size and restoring scale 1 restores capacity. Exercise third-person rotation, freelook, crouch/prone, ADS transitions and walls; also enter/exit vehicles. Extreme 0.01/100 sizes need additional collision testing.
2. Place two free endpoints on ground and on separate building floors. Link them, rename, move, delete, recreate and relink through GM controls. Exercise interaction and automatic entry at zero and nonzero delay, leaving during countdown, deletion/relinking during travel, blocked arrivals and arrival-loop protection. Ordinary clients must not configure endpoints.
3. Double-click props, vehicles, characters, audio emitters and endpoints. Confirm Bifrost's shell opens once, every expected attribute appears, compound controls work and applying/closing releases input normally.
4. Send hints to each audience. Only selected recipients should see the passive card; verify keyboard/mouse input remains uninterrupted and the card expires at its configured time. Check long text and different display scales.
5. Listen to all six sound presets. Test start/stop, zero and long fades, volume, loop/one-shot, range, emitter movement/deletion, opposite sides of walls and indoor/outdoor reverberation. Verify late-joining clients receive ongoing sound configuration. Audio phase is not synchronised between clients.

The implementation is prepared for these tests; source and build evidence alone do not establish their results.

Observed outside this change: the existing terrain-hide prefab still reports the old `m_BudgetValue` field during catalogue load, and base/editor resources report legacy `Parent`/widget and notification diagnostics. These remain separate from the requested features and are not represented as fixed.

## Teleporter catalogue follow-up

On September 5, the reported missing entry was checked in the current local Bifrost Workbench project. The native placeable registry contains the standalone teleporter; its editable info resolves as Teleporter. A local Arland GM catalogue build returned 1,557 entries and found exactly one Teleporter result in Effects. The rendered CREATE panel also showed Teleporter as the second item in Effects > Bifrost, below Hide Terrain Objects. No catalogue code correction was needed. The previous Create/Edit Teleporter setup action has been replaced by this placeable prefab row. The reason it was not visible in the user's earlier view was not reproduced. Temporary diagnostic scripts were removed after the check; no scene entities were placed or saved.

## Replication review corrections

REQUIREMENTS
- Remove the dedicated-server dependency on a local GM editor when teleporting.
- Make one-shot recordings finish without restarting during fade-out.
- Implement a bounded movement-speed workaround above the native limiter.
MINIMUM COMPONENTS NEEDED
- Existing player-controller RPC path and native character teleport.
- Existing audio component and six finite playback banks.
- Existing character controller with swept, owner-simulated extra ground movement using server-replicated settings.
REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No replacement locomotion framework, per-frame movement RPCs, or publication.
PRIMARY RISKS
- Native prediction and collision behavior of scripted displacement require runtime evidence; dedicated/remote/JIP remain separate gates.
REQUEST INTERPRETATION
- Fix the reviewed defects locally and validate the available runtime. Movement assistance must stop for vehicles, airborne and special movement states.
UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Correct the release-blocking replication findings and make increased walking speed effective without taking over the game's locomotion system.

### Interrupted validation — not a release approval

- Teleport now uses the server's actual player controller and native SCR_Global.TeleportPlayer, followed by reliable broadcast with character replication identity checked before applying an arrival. It no longer depends on a locally open GM editor; null controllers are rejected.
- Audio banks are finite, with one native recording per event. The component tracks whether a one-shot has already started and releases its handle at the recording duration; only enabled looping can start it again.
- A candidate movement assist runs on the character simulation owner using server-replicated scale/speed, a swept collider, special-state exclusions and a 35 m/s ceiling. Its real movement/prediction behavior is NOT verified and must not be represented as publication-ready.
- Native validation before the final audio lifetime addition passed with zero errors and 14 existing warnings. All 12 native resource builds succeeded at 16:21:25. The final audio lifetime addition has not completed validation.
- The first local regression run failed movement, immediate teleport-position and audio-lifetime assertions. The test spawned at the GM camera's horizontal position over water, making movement evidence invalid; the immediate teleport assertion was also moved to a later simulation tick. The corrected fixture uses dry ground and audio beside the listener. These corrections have not produced a passing run.
- Workbench crashed after the second script reload at 16:25:00. The old process exited; a fresh startup encountered WorkbenchGame compile diagnostics. Computer Use was then stopped by the user with Escape. No further editor interaction was performed. Temporary injected test/bridge scripts were removed from the addon; persistent test sources remain under Tests/Workbench.
- Dedicated-server, remote-client, join-in-progress, movement collision/rotation and final local regression evidence remain outstanding. Do not publish this candidate on the strength of compilation or source review alone.

### User testing handoff

The user elected to perform runtime testing. Temporary Game and WorkbenchGame hooks are confirmed absent. Post-cleanup native WORKBENCH validation passed with zero errors and 14 existing warnings, including the final audio lifetime implementation; the fresh-start WorkbenchGame errors came from the injected EMCP_WB_Inspector helper and disappeared from validation after cleanup. Resource file/hash checks and git diff whitespace checks passed. Movement collision clearance is subtracted only when the sweep hits an obstacle, so a clear path does not lose a fixed distance each frame. The movement assist is bounded to 35 m/s and does not accelerate airborne, linked, vehicle, ragdoll or stance-transition motion. No passing final local functional run or dedicated/remote/JIP result is claimed.
