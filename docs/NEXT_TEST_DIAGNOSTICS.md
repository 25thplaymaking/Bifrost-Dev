# Next-test diagnostics

REQUIREMENTS
- Observe silent failures in the existing candidate during Bryce's test, prioritizing GM outside-click lockup.
- Capture input phases, modal/focus state, property transaction closure, native handoff, Arsenal selection/placement and authority results.
- Preserve behavior and server authority; keep overhead and log volume bounded.
- Save and close Workbench after validation so the user can test.

MINIMUM COMPONENTS NEEDED
- One test logging helper and hooks in the existing entry/exit points.
- The existing 500 ms viewport poll for changed-state checks; no additional repeating timer.
- Existing client/server script logs and this test guide.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No global function wrapper, exception suppression, new telemetry service or external upload.
- No claim that this catches every script, native-engine or third-party failure.

PRIMARY RISKS
- Diagnostic output adds some overhead and can reach its cap.
- Client input state is unavailable from server logs; both sides are needed for multiplayer traces.
- Missing completion markers are evidence to investigate, not proof of a particular cause.

REQUEST INTERPRETATION
- Enable bounded diagnostic tracing in the workspace addon for the next acceptance test.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
Make silent state and return-path failures visible without changing gameplay behavior or the acceptance criteria.

## Enabled for the candidate

Tracing is enabled automatically by DCO_TestDiagnostics.ENABLED in this workspace addon. There is no console command, new file permission, external collector or Workshop publication. It writes [BF-DIAG] records to the engine's normal script log.

Each record includes a process-local sequence number, monotonic tick and client/authority role. Log timestamps supply wall-clock correlation; tick values cannot be compared directly across machines.

Coverage:
- gm.outside.down/up/click: actual property backdrop event methods, including widget visibility/enabled state.
- gm.dropdown.outside: dismissal of a dropdown's separate catcher.
- gm.properties.start/set-open/cancel/end/native-fallback: transaction boundaries.
- gm.native.open/handoff/close/native-ownership: native versus embedded ownership.
- gm.state: changed-state snapshots from the existing 500 ms poll, including root enablement, focus, panel and both backdrops. Hidden focused widgets, visible catchers without their panels, and a disabled root without native ownership are marked as suspect warnings.
- gm.native-heartbeat-expired: existing recovery was required.
- arsenal.inspect.open/stage.missing/mount.select/candidates/pick/move/rejected: preview binding, candidate counts, exact slots and refused placement.
- kit.request.send/receive, kit.apply.begin/end, kit.result.server/client: request flow, status and applied/skipped counts, including early apply exits.
- gm.tool.send/receive/denied/return/confirmed: existing generic tool RPC route. reportState=false is not automatically a failure because some tools do not return a toggle state.
- gm.scale.request/apply/result/broadcast/receive/jip and gm.visibility.apply: authority setters, reliable live-client delivery, late-join payload and local native readback. Matching broadcast and receive IDs are the dedicated-client handoff evidence; final rendering still requires observation on that machine.
- gm.shutdown: shell teardown.

No player names, chat, authentication values or kit JSON are recorded. Records may include numeric player/replication IDs and item resource paths.

## Bounds and limitations

Maximum 10,000 events per process/script session, with details limited to 800 characters. TRACE_LIMIT announces when the diagnostic cap is reached; normal engine logging continues. GM snapshots emit only when state changes. No additional polling timer or automatic corrective action was added. A script reload or fresh process resets the diagnostic cap.

This is targeted instrumentation, not a universal exception trap or profiler. It cannot see GPU/native failures, all third-party paths or every Bifrost function. A missing event can mean a path never ran, the wrong build was loaded, the cap was reached, or execution stopped. Treat it as a lead, not an automatic diagnosis.

## During the test

1. Load C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev/addon.gproj (GUID 6A0C2D6CE9809C6E). For multiplayer authority/replication diagnosis, the server and participating clients need the corresponding candidate build; the published build has none of these new hooks.
2. Test outside-click closure first, then vest customization and the other corrected tools.
3. If something sticks, note the local time and last action. If practical, pause briefly before using Y to recover so the existing 500 ms poll can record state.
4. Preserve the affected client's script.log and console.log from that run. Server logs alone cannot show client focus/hit testing. Include the server and observer logs for replication/apply failures.
5. Review [BF-DIAG] lines alongside ordinary errors and the action timestamp. The recorder does not suppress or replace native logging.

Workbench logs are under C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/logs/. Standalone game logs are under C:/Users/Bryce/Documents/My Games/ArmaReforger/logs/. Use the actual server profile's logs for a dedicated server.

## Validation

Native WORKBENCH compilation: zero errors, 14 base-game warnings after temporary probes were removed. The actual-layout regression still passed all 19 checks with tracing enabled. Its outside-click → cancellation → end sequence was confirmed in script.log under logs_2026-09-06_12-30-25. Source layout bindings: 370 checks, zero failures, two fault injections detected. An initial diagnostic-only invalid player-ID lookup was corrected before these passes.

Server and replicated callback traces are compile-checked and await the next multiplayer test. This instrumentation does not change the open user-acceptance status of the fixes.

## Workspace reconciliation — 6 September 2026

All implemented fixes, BIA names and enabled diagnostic hooks are now saved in the authoritative workspace addon above. Workbench loaded this exact path and passed a native reload, 19 menu checks and 34 lifecycle checks. Layout validation passed 370 bindings and two fault injections; field resource validation passed. This supersedes earlier instructions to open the external fix checkout. User acceptance and the GPU camera-crash investigation remain open.
