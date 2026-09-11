# Reforger lag review — 8 September 2026

Reviewed live on grain.silo on 9 September UTC (8 September EDT). Scope: read-only server/process/configuration/log review after reports of rubber banding and lag spikes on Sunday and again Tuesday. No server restart, setting change, mod change, or publication was performed.

## Conclusion

The retained logs show two different failure patterns: severe simulation stalls on Sunday, and predominantly one-client packet loss on Tuesday. They do not identify Bifrost as the root cause. Bifrost cannot be cleared solely because its name is absent from exception stacks: an action can trigger native or another mod's code, and no controlled comparison or function profiler was available.

A separate, confirmed management problem exists: local RCON connections repeatedly exhaust the server's 16 admin-connection slots. It occurs in both reviewed sessions. Its contribution to gameplay lag is unproven.

## Authoritative targets and package

- Operations: `08a3268f-f6ce-4c0b-a6ed-3f8bbbea9c1a`, game port 2001.
- Training: `1a463a2a-3efb-4084-8b97-4b68bf8818aa`, game port 2011.
- Server data root: `/opt/25vid/backend/server-data/`.
- Both installed Bifrost packages identify version **1.0.31**, GUID `6A0C2D6CE9809C6E`, internal name `BifrostDev`.
- Both `data.pak` files are 63,812,900 bytes with SHA-256 `3d4853cbca90e8e9b07c9ec5cfc05f1764a26837eb0ba0e5554bf58e16485193`.
- Package metadata records an update at `2026-09-06T22:46:30Z`. The Sunday boot records a Bifrost download and subsequent mount. This establishes temporal overlap, not causation or an immutable Sunday package hash.
- The local authoritative addon.gproj was read and matches the identity. The working tree contains extensive existing changes and does not establish source parity with deployed 1.0.31. No existing changes were modified. Workbench was unreachable; no compilation or gameplay test was performed.
- Operations and then Training stopped during the read-only investigation, independently of these commands. Both subsequently reported Docker `Exited (0)`. Exit status alone does not identify who or what stopped them.

## Sunday: simulation stalls

Operations session: `profile/logs/logs_2026-09-06_22-47-01/`.
Times below are EDT; native log times are UTC.

| Time on Sunday | Native console evidence | Interpretation |
| --- | --- | --- |
| 7:51:04 PM | `console.log:24789`: 0.5 FPS, maximum frame 2141.9 ms, 11 players; all listed clients report PktLoss 0/100 | A roughly two-second simulation stall, without reported packet loss in this sample |
| 8:26:38 PM | `console.log:54841-54842`: maximum frame 1086.2 ms; Server Admin Tools reports 8 FPS with nine players | Another simulation stall |
| 8:27:45 PM | `console.log:56945`, `56952`: maximum frame 1065.6 ms; Server Admin Tools reports 4 FPS | Repeated stalls |
| 8:30:55 PM | `console.log:60103`: 0.5 FPS, maximum frame 2178.0 ms, nine players; all listed clients report PktLoss 0/100 | The largest observed active-session frame stall |

Across this session's 8,665 telemetry records with players connected, 98 report FPS below 30 and 46 below 10. These are records, not distinct incidents or exact durations.

`crash.log` contains **783 VM exception records**, from 7:55:02 PM through 8:20:06 PM, for a null `phy` variable in `SCR_AIGetAllowedLookRange.EOnTaskSimulate`, `Scripts/Game/AI/ScriptedNodes/Vehicles/SCR_AIGetAllowedLookRange.c:42`. They are exceptions, not 783 server process crashes. Stalls precede and outlast that interval, so this fault cannot alone explain every spike. The class name/stack does not establish which installed addon supplied or triggered the path.

Weapon and attachment creation appears beside the first two-second stalls under `rpl::Pip::ProcessNetToGame`. No retained caller stack identifies the initiating UI/mod. Bifrost tracer placement/deletion appears later, around 8:03–8:06 PM; the severe stalls began beforehand and continued afterward. Neither proximity nor absence of a named Bifrost error proves causation.

## Tuesday: client-specific packet loss and shorter stalls

Operations session: `profile/logs/logs_2026-09-07_08-57-39/`. Date rollover was tracked while parsing; the rows below are **8 September**, not the boot date.

For the 6–8 PM EDT window, client slots showed:

| Slot | Samples | Median reported PktLoss /100 | Maximum /100 | Samples with nonzero loss |
| --- | ---: | ---: | ---: | ---: |
| C0 | 2982 | 8 | 29 | 2868 |
| C1 | 1893 | 0 | 2 | 2 |
| C2 | 278 | 0 | 0 | 0 |

Slots are session identifiers, not verified player identities. These statistics summarize the game's reported counter; they are not a packet capture or a separately calculated network-wide loss percentage.

At **6:58:40 PM**, `console.log:587122` reports C0 PktLoss **29/100**, RTT 33 ms, while C1 reports **0/100**, RTT 96 ms in the same record. At 7:05:56 PM and 7:12:25 PM the same contrast is 27/100 versus 0/100 and 26/100 versus 0/100. This points toward a client/path-specific problem or client-specific delivery/processing load; it does not identify the precise failing network hop or rule out per-client server behavior.

There are also shorter simulation hitches: **543.9 ms at 6:46:12 PM** (`console.log:583874`), **516.8 ms at 6:46:14 PM** (`583882`), and **501.6 ms at 6:47:14 PM** (`584141`). Average FPS remains high, so average CPU/FPS alone would miss these interruptions.

The same boot contains three map-marker cleanup exceptions with `AG0_TDLMapMarkerEntry.OnDeviceUnregistered`, `AG0_TDLSystem.UnregisterDevice`, and `AG0_TDLDeviceComponent.OnDelete` in the stack. These occurred on Monday, not at the Tuesday hitches, and do not prove Tuesday's cause.

Training's latest session was much less affected when sampled: active-session maximum frame 48.8 ms, median reported loss zero, with intermittent individual-client loss and RTT spikes. It was not a simultaneous controlled comparison with Operations.

## Confirmed local RCON connection churn

Operations logs repeatedly show `Client List is full!` for source **172.17.0.1**, the local Docker bridge:

- Sunday session: 6,405 authorized logins and **1,187 full-list rejections** across the entire retained session, including time with no players.
- Monday-to-Tuesday boot: 45,741 authorized logins and **2,697 full-list rejections**.
- Tuesday 6–8 PM EDT specifically: 2,180 authorized logins and 63 full-list rejections.

These are RCON/admin slots, not the gameplay player cap. The source is local to the host; it is not evidence of an external attack.

Current deployed website code offers a concrete source to investigate: `/opt/25vid/backend/services/rcon_bridge.py:187` opens a new UDP endpoint for every command, line 194-196 authenticates, and line 254 closes the local transport. The metrics collector (`services/server_metrics_collector.py:93`) and player reconciliation (`services/server_player_sessions.py:491`) each request `#players` through this path. A serialized queue is local to each worker process. The reviewed code and source address fit the repeated login pattern, but do not prove that every historical login came from these callers or that connection churn caused a gameplay stall.

## Network capacity and configuration

- Historical `sadf`/sysstat records for the physical interface `enp4s0` show no NIC error/drop samples on 6–9 September in the retained data.
- Sunday incident-period ten-minute averages were around 0.9–3.06% link utilization in the sampled rows. Tuesday's 6:30–7:30 PM EDT rows were 0.03–0.09%.
- These averages provide no evidence of sustained physical-link saturation during the matching periods. They cannot exclude brief bursts, upstream loss, per-client issues, or an attack outside the sampling evidence. No historical packet capture was available.
- Both Reforger processes run with **`-maxFPS 750`** and reached approximately 750 FPS when lightly loaded, accounting for substantial CPU use even without a large mission. This is a configuration review candidate, not a demonstrated cause of packet loss. BI's hosting example uses `-maxFPS 60`: https://community.bistudio.com/wiki/Arma_Reforger:Server_Hosting .
- Better View Distance logs explicitly report **12,000 m character/vehicle replication range** and a **12,000 m AI simulation ceiling**, with fortifications always replicated. Its effective view-distance ceiling is 25,000 m. This differs from simply reading the vanilla `networkViewDistance: 5000` field. The mod also warns that the launch config is not script-readable and points to its profile configuration. These effective settings are a possible load multiplier, not proof of the reported cause. BI documents the performance cost of wider network simulation ranges: https://community.bistudio.com/wiki/Arma_Reforger:Startup_Parameters .

## Recommended next steps

1. Identify the player represented by C0 during Tuesday's incident and compare that client's timestamp-matched logs/connection behavior with an unaffected player. Do not infer identity from slot number alone.
2. Repair and measure RCON session reuse/polling coordination as a separate website/server-management change. Preserve admin commands and player-session tracking; do not merely suppress the full-list errors.
3. Reproduce a reported hitch with server function profiling and matching client telemetry. Compare the same mission and player actions with Bifrost enabled/disabled in a controlled test, keeping other mods/configuration unchanged. Preserve the live mission rather than using production as an uncontrolled experiment.
4. Review the 750-FPS cap and effective BVD replication/AI ranges separately. Change one variable per authorized test and compare frame-time peaks plus per-client loss. Do not present speculative tuning as a verified fix.

No cause was conclusively attributed to Bifrost, and no fix or complete gameplay acceptance is claimed.
