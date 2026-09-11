# GM camera crash review — 2026-09-06

Status: crash confirmed; new BF-DIAG tracing excluded from this run; underlying graphics fault unresolved.

## Evidence

- Latest crash session: `C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/logs/logs_2026-09-06_12-43-21`.
- `console.log:23` and `:33` establish the loaded project as `C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev/addon.gproj`, not the isolated candidate in `Documents/Bifrost-Fixes/session-2026-09-05`.
- A complete current source search of that original checkout (188 files, no skipped files) contains no DCO_TestDiagnostics reference. The crashed session logs contain no BF-DIAG events.
- GM_Arland loaded; the player opened GM at 12:50:30.730 local time.
- Windows System log contains four nvlddmkm Event 153 errors at 12:50:41.385, 12:50:45.681, 12:50:46.466 and 12:50:51.870. Each reports Error occurred on GPUID: 100.
- `console.log:366-370`: at 12:50:51.871 the renderer reports 0x887A0006 / DXGI_ERROR_DEVICE_HUNG from RendererImpl::EndFrame, followed by an engine crash.
- Post-failure renderer statistics show Wait4GPU 5076.28 ms and VRAM 1660 MB of 11342 MB budget. This does not indicate VRAM exhaustion at the reported snapshot.
- crash.log records a native illegal write to address 0. The stack has unresolved native symbols and provides no actionable script source frame; those symbol labels are insufficient to name a texture or compression function as the cause.
- The native minidump remains in the original log folder. No logs were removed and no application was relaunched during this investigation.

## Conclusion and next verification

The newly added BF-DIAG code was absent from the loaded checkout, so it did not cause this particular crash. The observed failure is a graphics-device hang, corroborated by Windows NVIDIA driver events. This alone does not distinguish an engine/rendering bug, an addon rendering trigger, driver behavior or hardware instability; it does not exonerate all Bifrost rendering code.

This run also cannot accept or reject the candidate fixes or BIA rename because it loaded the other checkout. Open the candidate addon.gproj for the next acceptance run and verify BF-DIAG output. If camera movement reproduces a device hang, isolate the same world/camera movement with a base-game control and then the candidate before changing drivers, graphics settings or rendering code. No speculative source or system setting changes were made.
