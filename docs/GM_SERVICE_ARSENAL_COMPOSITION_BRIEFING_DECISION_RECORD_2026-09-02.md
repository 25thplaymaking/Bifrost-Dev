REQUIREMENTS
- Place the Spawn vehicles crewed control directly above the World Capacity header.
- Remove the experimental Armaments tab and every server mutation that changes mounted weapon, ammunition-feed, ammunition-type, or authored capacity. Retain only Repair and Cargo tabs.
- Keep Repair-tab rearm limited to refilling the vehicle's existing authored magazines and rocket barrels so servicing never replaces a weapon prefab or mount.
- Restore firing for the armed loiter delivery.
- Restore the Arsenal gear-selection panel after leaving a container contents view, and make right-click item loading work for ALICE and Soviet vests.
- Make the Bifrost composition workflow match the practical Arma 3 Zeus workflow: a categorized composition library, explicit selection capture, clear placement, and retained composition metadata.
- Add editable GM mission information to Scenario Settings and make the information available through the map journal in every Game Master mission.
- Keep gameplay mutations server authoritative and preserve replicated and join-in-progress state.
- Validate source/layout references with PAC1CLI and validate scripts with Workbench.
- Repair the latest regressions: preserve vehicle usability after ordinary repair/refuel/rearm and after leaving the service area, isolate Mission Information to its own Scenario Settings category, restore the Arsenal gear browser immediately after Contents closes, restore ALICE/Soviet vest deposits, and restore the Gunsmith weapon carousel.
- Classify the supplied Workbench diagnostics so unrelated base-game warnings are not treated as evidence that an interaction was repaired.
- Rename the FX catalog entry and property labels from Ambient to Emitter without changing its resource identity or saved-layout compatibility.
- Fall back to prefab-derived catalog names for the three supplied base-game string IDs that are absent, preventing Bifrost catalog warmup from repeatedly requesting them.
- Keep the CREATE-row `TARGET` status inside its Bifrost row now that Arsenal Access has a visible icon.
- When Contents closes through Back or Escape, immediately repopulate the left panel with the originating gear category instead of leaving an empty intermediate state.
- Remove the unsupported checkbox-border layout property reported while loading `DCO_GMPanel.layout`.
- Make the checked `X` clearly distinguishable from the accent-colored crewed-vehicle checkbox border.
- Reserve a bounded trailing portion of every pooled CREATE row for status text so `TARGET` cannot be clipped by the Arsenal Access icon and label.
- Ensure a newly focused or clicked gear card cancels any stale post-Contents restore and immediately replaces an already-open normal category list.
- Restore the normal CREATE folder-row scale and let the visible row set use the full list region down to the Spawn vehicles crewed control.
- Returning from the Objects category to All must restore the complete category tree, including the Vehicles folder, after an asset was selected for placement.
- Keep canonical APP-6 faction symbols and third-party faction artwork in independent, non-compressing tab slots so neither source distorts the other.

MINIMUM COMPONENTS NEEDED
- Existing Bifrost Create panel layout and controller.
- Existing Arsenal Soldier screen, item-list panel, item rows, and item-storage discovery.
- Existing vehicle service client/server request path and native turret magazine APIs.
- Existing FX loiter pass controller.
- Existing composition record, catalog, persistence, RPC, and panel components.
- Existing GM Scenario Settings panel plus the native replicated respawn-briefing component and a Bifrost journal configuration.
- Existing WeaponSlotComponent inspection, cloth-node owned-storage hierarchy, and current Soldier/Gunsmith layouts; no new service or parallel UI framework is needed.
- Existing CREATE row text binding and the Soldier screen's normal gear-card selection path.
- Existing `LayoutSlot` sizing API and gear-row focus/click invokers; no new panel or input layer is needed.
- Existing CREATE row pool, custom scrollbar, category state, and faction-icon bindings; the redundant native scroll wrapper is not needed.

REJECTED/NEEDS CLARIFICATION BEFORE ACTION
- No requested behavior is rejected.
- Custom weapon/feed replacement and shared-capacity redistribution are removed rather than delayed behind a hidden UI or retained RPC.
- Arma 3 editor-only implementation details that have no Reforger equivalent will be adapted to Reforger while preserving the user-facing composition workflow.

PRIMARY RISKS
- ALICE and Soviet vest cargo may be exposed through nested or cloth-node storage components with different access rules than standard vests.
- Vehicle weapon prefabs can expose several muzzles and fixed-capacity magazines; rearm must refill only the already-mounted authored objects on the server.
- Loiter behavior spans deferred placement, server authority, projectile creation, and target acquisition; static validation alone cannot establish live firing.
- Composition and briefing state must remain correct for dedicated servers, remote clients, and join-in-progress clients.
- Scenario Settings and Create panel changes must preserve the current Bifrost layout at multiple UI scales.
- Any mounted-weapon prefab replacement can detach or invalidate the vehicle's authored mount hierarchy, so no service request may replace or delete mounted weapons.
- ClothNodeStorageComponent is a routing/aggregate store; validating or depositing against it directly can disagree with the child pouches that actually hold items.
- Closing a dynamically removed Contents panel can leave focus inside the click chain that removed it unless restoration is deferred to the next UI frame.
- The interaction handler's manual collection API replaces physically discovered actions. Enabling the primary override for a nearby Service helper suppresses vehicle door and compartment actions even though the vehicle remains otherwise valid.
- Mouse focus restored on a zero-delay callback can race the next pointer click and move focus back to the old Contents origin card before that click completes.
- A missing layout brace currently causes the Gunsmith receiver overlay button to be parsed as data on its parent widget.
- Rebuilding the gear browser synchronously from the Back button's click callback can remove the active panel during input dispatch; the canonical card-selection path must run on the next UI frame.
- The CREATE row's icon and status share a narrow horizontal layout, so the active status needs a bounded font size without changing normal budget presentation.
- Reusing the accent color for both the checkbox border and check mark reduces checked-state contrast; the mark should use the existing bright Bifrost text color.
- A zero-delay Vest restore can execute after a newer Footwear interaction unless that interaction invalidates the pending callback; focus is the earliest reliable card event.
- A direct status TextWidget remains content-sized in the horizontal row, so reducing its desired font cannot guarantee an icon-safe boundary.
- Native content-browser filter notifications are view-state changes, not catalog-source changes; rebuilding Bifrost's catalog from them can capture a transient Objects-only state.
- Seven faction buttons share a narrow row; their current image padding can force the horizontal layout to compress otherwise correctly proportioned APP-6 symbols.

REQUEST INTERPRETATION
- The top-level service modes are Repair and Cargo. Repair retains repair, refuel, rearm, and full-service operations; rearm only tops up existing authored magazines and rocket barrels.
- Leaving Arsenal contents mode immediately reopens the originating gear category in the left panel; one click on any other gear card then replaces it with that category.
- Vehicle Service remains an additional nearby action; it must not replace normal aimed vehicle interactions before opening, after closing, or after leaving the bay.
- Armed loiter must repeatedly fire during its orbit; observation-only loiter remains non-firing.
- Composition capture and placement should be discoverable from the Create workflow and use author/category metadata comparable to Arma 3's custom compositions.
- Mission information means faction-neutral Situation, Mission, Execution, Signal, and Intelligence text editable by an authorized GM from Scenario Settings.
- Mission Information is a synthetic Scenario Settings category, hidden in ordinary entity Properties sessions; it is not a field prepended to every attribute page.
- The supplied EditBoxFilterComponent warning is a base-script guard-condition defect reproduced in PAC1CLI source, and the missing string IDs are catalog label warnings. Neither is accepted as the cause of the reported vest or carousel failures.
- The CREATE list should distribute only its currently visible pooled rows across the existing region and retain Bifrost's own paging/scrollbar when the expanded tree exceeds that pool.
- Native filter callbacks may complete initial catalog warmup, but an already-built Bifrost catalog is refreshed only by the entity-catalog initialization path, not by category/search view changes.

UNDERSTANDING OF THE OVERALL TASK IN A BRIEF SUMMARY
- Repair the reported regressions and complete the requested GM workflows without adding a parallel framework: remove the unstable armament experiment, reuse Bifrost's existing UI, RPC, persistence, native storage hierarchies, and Reforger journal systems, then provide a precise hands-on test list for the remaining runtime evidence.
