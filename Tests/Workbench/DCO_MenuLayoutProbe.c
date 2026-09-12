class DCO_MenuLayoutProbe
{
 int passed;
 ref array<string> failures = {};
 void Expect(bool ok, string label)
 {
  if (ok) passed++;
  else failures.Insert(label);
 }
 void Run()
 {
  if (GetGame().InPlayMode() || DCO_GMUIController.IsActive()) { failures.Insert("Edit mode with no active GM required"); return; }
  WorkspaceWidget ws = GetGame().GetWorkspace();
  if (!ws) { failures.Insert("Workspace unavailable"); return; }
  Widget gm = ws.CreateWidgets("{13198E778653CACC}UI/layouts/DCO_GMPanel.layout");
  Expect(gm != null, "Native GM layout creates");
  if (gm)
  {
   DCO_GMScenarioPanel scenario = new DCO_GMScenarioPanel();
   scenario.DCO_TestLayout(gm, this);
   DCO_GMPlacementConfirm placement = new DCO_GMPlacementConfirm();
   placement.DCO_TestModalProtection(gm, this);
   DCO_GMUIController controller = new DCO_GMUIController(ResourceName.Empty);
   controller.DCO_TestInputOwnership(gm, this);
   DCO_PropertyScopeProbe properties = new DCO_PropertyScopeProbe();
   properties.Run(gm, this);
   gm.RemoveFromHierarchy();
  }
  Widget armory = ws.CreateWidgets("{AB205AD4E2000004}UI/layouts/Menus/ArmoryV2/GRSA_ScreenGunsmith.layout");
  Expect(armory != null, "Native Gunsmith layout creates");
  if (!armory) return;
  Expect(!armory.FindAnyWidget("PositionRow") && !armory.FindAnyWidget("PositionSlider"), "Position controls are absent");
  Widget scroll = armory.FindAnyWidget("HardpointRailScroll");
  Expect(ScrollLayoutWidget.Cast(scroll) != null, "Mount rail is native scroll layout");
  Expect(scroll && scroll.FindHandler(BIA_SmoothScrollComponent), "Mount rail has mouse wheel handler");
  BIA_CalloutLayer layer = new BIA_CalloutLayer(armory.FindAnyWidget("CalloutLayer"), null);
  layer.DCO_TestSeed();
  Expect(layer.FindEntryByAttachment("duplicate.et", 7) == 7, "Duplicate selection follows target mount");
  Expect(layer.FindEntryByAttachment("duplicate.et", 18) == -1, "Missing target cannot select another duplicate");
  BIA_HardpointRail rail = new BIA_HardpointRail(armory, layer);
  rail.DCO_TestMouseClothing(this);
  BIA_TileStrip strip = new BIA_TileStrip(armory);
  array<BIA_ItemEntry> candidateItems = {};
  BIA_ItemEntry candidate = new BIA_ItemEntry();
  candidate.m_Prefab = "{B74A4FF0DD8BB116}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et";
  candidateItems.Insert(candidate);
  strip.ShowTiles("Mount candidates", candidateItems, ResourceName.Empty, string.Empty, false);
  strip.DCO_TestCandidatePress(this);
  strip.ShowMessage("Mount layout test");
  strip.RefreshPanelHeight();
  Expect(strip.GetVisibleHeight() == 188, "Mount picker keeps its compact fixed height");
  rail.ReserveCandidateSpace(strip.GetVisibleHeight());
  float left, top, right, bottom;
  AlignableSlot.GetPadding(armory.FindAnyWidget("HardpointRail"), left, top, right, bottom);
  Expect(bottom == 268, "Mount list reserves candidate panel and gap");
  strip.Hide();
  Expect(strip.GetVisibleHeight() == 0, "Hidden picker reserves no height");
  rail.ReserveCandidateSpace(0);
  AlignableSlot.GetPadding(armory.FindAnyWidget("HardpointRail"), left, top, right, bottom);
  Expect(bottom == 96, "Mount list expands after picker closes");
  BIA_GunsmithScreen screen = BIA_GunsmithScreen.Cast(armory.FindHandler(BIA_GunsmithScreen));
  Expect(screen != null, "Native Gunsmith screen handler");
  if (screen) screen.DCO_TestContentsMode(armory, layer, this);
  DCO_PlacementCatalog catalog = new DCO_PlacementCatalog();
  catalog.DCO_TestSearch(this);
  rail.Disable();
  layer.Clear();
  armory.RemoveFromHierarchy();
 }
}
modded class DCO_GMScenarioPanel
{
 void DCO_TestLayout(Widget root, DCO_MenuLayoutProbe probe)
 {
  m_wPanel = root.FindAnyWidget("DCO_ScenarioPanel");
  m_wBackdrop = root.FindAnyWidget("DCO_ScenarioBackdrop");
  probe.Expect(ButtonWidget.Cast(m_wBackdrop) != null, "Property backdrop is a native button");
  ref DCO_GMContextMenu ownedMenu = new DCO_GMContextMenu();
  m_Menu = ownedMenu;
  m_Menu.Init(root);
  m_wPanel.SetVisible(true);
  m_wBackdrop.SetVisible(true);
  m_bOpen = true;
  m_aSessionAttributes = {};
  array<string> labels = {"Example"};
  array<int> ids = {1};
  m_Menu.Show(labels, ids, 20, 20, null, null);
  probe.Expect(m_Menu.IsOpen() && root.FindAnyWidget("DCO_MenuBackdrop").IsVisible(), "Dropdown and second backdrop open");
  GetGame().GetWorkspace().SetFocusedWidget(m_wBackdrop);
  DCO_ScenarioBackdropHandler handler = new DCO_ScenarioBackdropHandler(this);
  handler.OnMouseButtonDown(m_wBackdrop, 1, 1, 0);
  probe.Expect(m_bOpen, "Outside press keeps the gesture blocked");
  handler.OnMouseButtonUp(m_wPanel, 1, 1, 0);
  probe.Expect(m_bOpen, "Release on a property control preserves the session");
  handler.OnMouseButtonUp(m_wBackdrop, 1, 1, 1);
  probe.Expect(m_bOpen, "Right button release preserves the session");
  handler.OnMouseButtonUp(m_wBackdrop, 1, 1, 0);
  probe.Expect(!m_wPanel.IsVisible() && !m_wBackdrop.IsVisible(), "Outside handler hides real property widgets");
  probe.Expect(!m_Menu.IsOpen() && !root.FindAnyWidget("DCO_MenuBackdrop").IsVisible(), "Outside handler hides real dropdown widgets");
  probe.Expect(GetGame().GetWorkspace().GetFocusedWidget() != m_wBackdrop, "Outside handler releases backdrop focus");
  handler.OnClick(m_wBackdrop, 1, 1, 0);
  probe.Expect(!m_bOpen, "Repeated close remains safe with real widgets");
  m_bOpen = true;
  m_wPanel.SetVisible(true);
  m_wBackdrop.SetVisible(true);
  handler.OnMouseButtonUp(m_wBackdrop, 1, 1, 0);
  probe.Expect(!m_bOpen && !m_wBackdrop.IsVisible(), "Release-only gesture closes a reopened session");
  m_bOpen = true;
  m_wPanel.SetVisible(true);
  m_wBackdrop.SetVisible(true);
  handler.OnClick(m_wBackdrop, 1, 1, 0);
  probe.Expect(!m_bOpen, "Keyboard gamepad click still closes properties");
  m_wBackdrop.SetVisible(true);
  ReconcileVisibility();
  probe.Expect(!m_wBackdrop.IsVisible(), "Orphan Properties backdrop is reconciled");
  m_bOpen = true;
  m_bEditing = true;
  m_wPanel.SetVisible(false);
  m_wBackdrop.SetVisible(true);
  ReconcileVisibility();
  probe.Expect(!m_bOpen && !m_bEditing && !m_wBackdrop.IsVisible(), "Hidden Properties owner cancels its session and catcher");
  DCO_InputProbeAttribute visibleAttribute = new DCO_InputProbeAttribute();
  SCR_BaseEditorAttribute dataAttribute = new SCR_BaseEditorAttribute();
  m_aSessionAttributes = {dataAttribute, null, visibleAttribute};
  m_bOpen = true;
  m_bEditing = true;
  probe.Expect(CanOwnPropertySession(), "Mixed data-only and visible attributes agree on handoff");
  m_aSessionAttributes = {dataAttribute, null};
  probe.Expect(CanOwnPropertySession(), "Data-only session retains Bifrost ownership");
  EndEditing(false);
  probe.Expect(!m_bOpen && !m_wPanel.IsVisible() && !m_wBackdrop.IsVisible(), "Missing manager end event still closes every property surface");
  m_bOpen = true;
  m_bEditing = true;
  m_wPanel.SetVisible(true);
  m_wBackdrop.SetVisible(true);
  m_Menu.Show(labels, ids, 20, 20, null, null);
  m_bConditionalRefreshQueued = true;
  m_bCategoryRefreshQueued = true;
  OnAttributesStart({dataAttribute});
  probe.Expect(!m_Menu.IsOpen() && !root.FindAnyWidget("DCO_MenuBackdrop").IsVisible(), "New property session releases the previous dropdown and catcher");
  probe.Expect(!m_bConditionalRefreshQueued && !m_bCategoryRefreshQueued, "New property session discards previous refresh work");
  probe.Expect(m_bOpen && m_bEditing && m_wPanel.IsVisible() && m_wBackdrop.IsVisible() && CanOwnPropertySession(), "Data-only properties remain inside Bifrost");
  Shutdown();
 }
}
modded class BIA_CalloutLayer
{
 void DCO_TestSeed()
 {
  for (int i = 0; i < 19; i++)
  {
   BIA_CalloutEntry entry = new BIA_CalloutEntry();
   entry.m_sTypeLabel = "Mount " + i;
   entry.m_iStorageSlot = i;
   if (i == 2 || i == 7) entry.m_AttachedPrefab = "duplicate.et";
   m_aEntries.Insert(entry);
  }
 }
}
modded class BIA_HardpointRail
{
 int m_iTestClicks;
 void DCO_CountClick(int index) { m_iTestClicks++; }

 void DCO_TestMouseClothing(DCO_MenuLayoutProbe probe)
 {
  m_bEnabled = true;
  m_bPadMode = false;
  // Empty thumbnails keep this widget-only test independent of entity resources.
  m_Layer.GetEntry(2).m_AttachedPrefab = ResourceName.Empty;
  m_Layer.GetEntry(7).m_AttachedPrefab = ResourceName.Empty;
  Rebuild(null, true);
  probe.Expect(m_wRoot.IsVisible() && m_aRows.Count() == 19, "Mouse clothing rail includes all 19 mounts");
  m_OnRowClicked.Insert(DCO_CountClick);
  m_aRows[0].OnMouseButtonDown(m_aRows[0].GetRootWidget(), -100, -100, 0);
  m_aRows[0].OnMouseButtonUp(m_aRows[0].GetRootWidget(), -100, -100, 0);
  m_aRows[0].OnClick(m_aRows[0].GetRootWidget(), -100, -100, 0);
  probe.Expect(m_iTestClicks == 1, "Rail left click reaches mount selection exactly once");
  m_aRows[0].SetEnabled(false, false);
  m_aRows[0].OnMouseButtonDown(m_aRows[0].GetRootWidget(), -100, -100, 0);
  probe.Expect(m_iTestClicks == 1, "Disabled mount ignores mouse press");
  m_aRows[0].SetEnabled(true, false);
  m_aRows[0].DCO_TestMenuSelect();
  probe.Expect(m_iTestClicks == 2, "Mount retains native keyboard gamepad activation");
  m_OnRowClicked.Remove(DCO_CountClick);
  Rebuild(null, false);
  probe.Expect(!m_wRoot.IsVisible(), "Mouse weapon mode restores callouts");
  Rebuild(null, true);
 }
}

modded class BIA_ItemRowComponent
{
 void DCO_TestMenuSelect() { OnMenuSelect(); }
 bool DCO_TestActivatesOnPress() { return m_bActivateOnPress; }
}
modded class BIA_TileStrip
{
 protected int m_iTestTileClicks;
 void DCO_TestCountTile(BIA_ItemRowComponent row) { m_iTestTileClicks++; }
 void DCO_TestCandidatePress(DCO_MenuLayoutProbe probe)
 {
  BIA_ItemRowComponent row;
  if (!m_aRows.IsEmpty()) row = m_aRows[0];
  probe.Expect(row && row.DCO_TestActivatesOnPress(), "Mount candidate activates on pointer press");
  if (!row) return;
  m_OnTileClicked.Insert(DCO_TestCountTile);
  Widget root = row.GetRootWidget();
  row.OnMouseButtonDown(root, -100, -100, 0);
  probe.Expect(m_iTestTileClicks == 1, "Mount candidate dispatches exactly once");
  m_OnTileClicked.Remove(DCO_TestCountTile);
 }
}
modded class BIA_GunsmithScreen
{
 void DCO_TestContentsMode(Widget root, BIA_CalloutLayer layer, DCO_MenuLayoutProbe probe)
 {
  m_wRoot = root;
  SizeLayoutWidget contentsAction = SizeLayoutWidget.Cast(root.FindAnyWidget("ContentsAction"));
  probe.Expect(SCR_ButtonTextComponent.GetButtonText("ContentsButton", root) != null && contentsAction && contentsAction.GetHeightOverride() == 58 && contentsAction.GetParent().GetName() == "HardpointRailColumn", "Contents action is fixed above the mount list");
  contentsAction.SetVisible(true);
  probe.Expect(contentsAction.IsVisibleInHierarchy(), "Contents action is visible with the clothing mount rail");
  probe.Expect(root.FindAnyWidget("ItemSearchBox") != null, "Contents search inherits existing layout");
  m_Contents = new BIA_ItemListPanel(root, "ItemListPanel", "ItemListTitle", "ItemList", "ItemScroll", "ItemSearchBox", "ItemListBackControls", "ItemListFilters");
  m_Strip = new BIA_TileStrip(root);
  m_CalloutLayer = layer;
  BIA_StageCore core = new BIA_StageCore();
  m_Stage = new BIA_WeaponStage(core);
  m_iStagedClothingSlot = 2;
  array<ref BIA_ItemEntry> items = {};
  m_Contents.Open("CONTENTS", items, ResourceName.Empty, string.Empty, false, true);
  probe.Expect(m_Contents.IsOpen(), "Contents opens in actual Gunsmith layout");
  probe.Expect(root.FindAnyWidget("ItemListFilters").IsVisible(), "Contents exposes item filters");
  OnSlotClicked(0);
  probe.Expect(!m_Contents.IsOpen(), "Mount selection hides contents");
  probe.Expect(m_iSelectedSlot == 0 && m_Strip.IsShown(), "Mount selection opens candidate panel");
  probe.Expect(TextWidget.Cast(root.FindAnyWidget("CandidatesTitle")).GetText().Contains("NO COMPATIBLE"), "Empty candidate pool explains missing parts");
  OnSlotClicked(0);
  probe.Expect(!m_Strip.IsShown() && m_iSelectedSlot == -1, "Reselecting mount closes picker");
  m_Contents.Destroy();
  m_Contents = null;
  m_CalloutLayer = null;
  m_Stage.Destroy();
  m_Stage = null;
  core.Release();
 }

}
modded class DCO_PlacementCatalog
{
 void DCO_AddFixture(string name, ResourceName path)
 {
  DCO_CatalogEntry entry = new DCO_CatalogEntry();
  entry.m_Name = name;
  entry.m_Prefab = path;
  entry.m_Category = CAT_OBJECT;
  entry.m_Type = EEditableEntityType.GENERIC;
  entry.m_SearchMetadata = SearchMetadata(entry);
  entry.m_SearchMetadata.ToLower();
  m_Entries.Insert(entry);
 }
 int DCO_ItemCount(string text)
 {
  array<ref DCO_CatalogRow> rows = Query(CAT_OBJECT, "", text);
  int count;
  foreach (DCO_CatalogRow row : rows) { if (!row.m_bHeader) count++; }
  return count;
 }
 void DCO_TestSearch(DCO_MenuLayoutProbe probe)
 {
  string section = "search:" + CategoryLabel(CAT_OBJECT);
  bool previous;
  bool existed = ExpansionState().Find(section, previous);
  ExpansionState().Set(section, true);
  DCO_AddFixture("Wooden Table", "PrefabsEditable/Props/Furniture/E_Table_01.et");
  DCO_AddFixture("Bunker", "PrefabsEditable/Compositions/E_Bunker.et");
  probe.Expect(!m_Entries[1].m_SearchMetadata.Contains("table"), "Editable directory cannot match table");
  probe.Expect(DCO_ItemCount("table") == 1, "Table search returns only furniture");
  probe.Expect(DCO_ItemCount("TABLE") == 1, "Search remains case insensitive");
  probe.Expect(DCO_ItemCount("wooden table") == 1, "Search requires all words");
  probe.Expect(DCO_ItemCount("no_such_item") == 0, "Unmatched search has no stale results");
  if (existed) ExpansionState().Set(section, previous);
  else ExpansionState().Remove(section);
 }
}

modded class DCO_GMPlacementConfirm
{
 void DCO_TestModalProtection(Widget root, DCO_MenuLayoutProbe probe)
 {
  m_wRoot = root;
  for (Widget child = root.GetChildren(); child; child = child.GetSibling()) child.SetVisible(false);
  probe.Expect(!IsCursorOverPanels(), "Empty GM surface permits world placement");
  array<string> modals = {"DCO_ScenarioPanel", "DCO_ScenarioBackdrop", "DCO_MenuBackdrop"};
  foreach (string name : modals)
  {
   Widget modal = root.FindAnyWidget(name);
   modal.SetVisible(true);
   probe.Expect(IsCursorOverPanels(), name + " blocks world placement");
   modal.SetVisible(false);
  }
  root.SetEnabled(false);
  probe.Expect(IsCursorOverPanels(), "Native modal ownership blocks world placement");
  root.SetEnabled(true);
  m_wRoot = null;
 }
}

class DCO_InputProbeAttribute : SCR_BaseEditorAttribute
{
 override ResourceName GetLayout() { return "{13198E778653CACC}UI/layouts/DCO_GMPanel.layout"; }
}

modded class DCO_GMUIController
{
 void DCO_TestInputOwnership(Widget root, DCO_MenuLayoutProbe probe)
 {
  DCO_GMUIController previous = s_Instance;
  WorkspaceWidget workspace = GetGame().GetWorkspace();
  Widget previousFocus = workspace.GetFocusedWidget();
  s_Instance = this;
  m_bBuilt = true;
  m_wRoot = root;
  probe.Expect(!IsExternalMenuOpen(), "Isolated input test has no external menu");
  probe.Expect(!IsModalActive(), "Empty enabled shell releases modal ownership");
  array<string> modals = {"DCO_ScenarioPanel", "DCO_ScenarioBackdrop", "DCO_ContextMenu", "DCO_MenuBackdrop"};
  foreach (string name : modals)
  {
   Widget modal = root.FindAnyWidget(name);
   modal.SetVisible(true);
   probe.Expect(IsModalActive() && IsWorldInputBlocked(true), name + " blocks world and trigger selection");
   modal.SetVisible(false);
   probe.Expect(!IsModalActive(), name + " releases ownership when hidden");
  }
  array<string> panels = {"DCO_CreateBrowser", "DCO_EditTree", "DCO_TopBar", "DCO_OptionsPanel", "DCO_OrdersBox", "DCO_TacticsPanel", "DCO_GizmoPanel", "DCO_SimPanel", "DCO_OverlayBar", "DCO_NotifPanel", "DCO_ChatPanel", "DCO_LayoutChip"};
  foreach (string name : panels)
  {
   Widget panel = root.FindAnyWidget(name);
   panel.SetVisible(true);
   panel.Update();
   float x, y, width, height;
   panel.GetScreenPos(x, y);
   panel.GetScreenSize(width, height);
   int px = x + width * 0.5;
   int py = y + height * 0.5;
   probe.Expect(width > 0 && height > 0 && IsPointerOverPanel(root, px, py), name + " blocks clicks inside its rectangle");
   probe.Expect(!IsPointerOverPanel(root, -10000, -10000), name + " leaves the world outside its rectangle available");
   probe.Expect(IsPointerOverPanel(root, px, py, true) == (name != "DCO_EditTree"), name + " respects the tree selection exception");
   panel.SetVisible(false);
   probe.Expect(!IsPointerOverPanel(root, px, py), name + " stops intercepting input when hidden");
  }
  root.SetEnabled(false);
  probe.Expect(IsModalActive(), "Disabled shell blocks action listeners too");
  m_bNativePropertiesOpen = true;
  m_NativePropertiesDialog = null;
  ReconcileNativePropertiesFocus();
  probe.Expect(!m_bNativePropertiesOpen && root.IsEnabled(), "Destroyed native owner recovers without a heartbeat deadline");
  probe.Expect(IsModalActive(), "Recovery consumes the remainder of the close gesture");
  ClearWorldInputSuppression();
  probe.Expect(!IsModalActive(), "Next gesture is available after recovery");

  Widget frameModal = root.FindAnyWidget("DCO_ScenarioBackdrop");
  frameModal.SetVisible(true);
  SCR_SelectionEditorUIComponent selection = new SCR_SelectionEditorUIComponent();
  selection.DCO_TestFrameInterruption(probe);
  SCR_ContextMenuActionsEditorUIComponent actions = new SCR_ContextMenuActionsEditorUIComponent();
  actions.DCO_TestBlockedRelease(probe);
  DCO_TriggerSyncDrag syncDrag = new DCO_TriggerSyncDrag();
  syncDrag.DCO_TestModalInterruption(probe);
  frameModal.SetVisible(false);
  selection.DCO_TestFrameRecovery(probe);

  Widget outside = workspace.CreateWidgets("{13198E778653CACC}UI/layouts/DCO_GMPanel.layout");
  Widget outsideFocus = outside.FindAnyWidget("DCO_QuickEditBtn");
  workspace.SetFocusedWidget(outsideFocus);
  ReleaseMenuFocus();
  probe.Expect(workspace.GetFocusedWidget() == outsideFocus, "Closing Bifrost preserves focus owned by another root");
  SCR_BaseContextMenuEditorUIComponent nativeMenu = new SCR_BaseContextMenuEditorUIComponent();
  nativeMenu.DCO_TestCloseOwnership(root.FindAnyWidget("DCO_ContextMenu"), outsideFocus, probe);
  outside.RemoveFromHierarchy();
  ClearWorldInputSuppression();
  m_Menu = new DCO_GMContextMenu();
  m_Menu.Init(root);
  m_Menu.DCO_TestTransitions(probe);
  ClearWorldInputSuppression();

  Widget target = root.FindAnyWidget("DCO_OrdersBox");
  target.SetName("DCO_InputProbePanel");
  target.SetVisible(true);
  Widget grip = target.FindAnyWidget("DCO_OrdersDrag");
  DCO_GMDraggable drag = new DCO_GMDraggable(target);
  drag.DCO_TestInterruptions(grip, probe);
  DCO_GMResizable resize = new DCO_GMResizable(target);
  resize.DCO_TestInterruptions(grip, probe);
  target.SetName("DCO_OrdersBox");
  target.SetVisible(false);
  DCO_GMDraggable.ResetRaise();
  m_Menu.Hide();
  m_Menu = null;
  GetGame().GetCallqueue().Remove(ClearWorldInputSuppression);
  m_bWorldInputSuppressed = false;
  m_bBuilt = false;
  m_wRoot = null;
  s_Instance = previous;
  workspace.SetFocusedWidget(previousFocus);
 }
}

modded class SCR_SelectionEditorUIComponent
{
 void DCO_TestFrameInterruption(DCO_MenuLayoutProbe probe)
 {
  m_vCursorPosClick = Vector(1, 1, 0);
  DrawFrameDown(false);
  probe.Expect(m_bIsDrawingFrameCancelled && !m_bIsAnimatingFrame, "Modal blocks native box-selection press");
  m_bIsDrawingFrame = true;
  m_bIsDrawingFrameCancelled = false;
  DrawFramePressed(false);
  probe.Expect(!m_bIsDrawingFrame && m_bIsDrawingFrameCancelled, "Modal cancels an already active selection frame");
  m_bIsDrawingFrame = true;
  m_bIsDrawingFrameIsToggle = true;
  m_bIsDrawingFrameCancelled = false;
  DrawFrameUp(true);
  probe.Expect(!m_bIsDrawingFrame && !m_bIsDrawingFrameConfirmed, "Modal release cannot queue a selection commit");
  GetGame().GetCallqueue().Remove(ConfirmFrame);
  m_bIsDrawingFrameConfirmed = true;
  m_bIsDrawingFrameIsToggle = true;
  m_bIsDrawingFrameCancelled = false;
  ConfirmFrame(true);
  probe.Expect(!m_bIsDrawingFrameConfirmed && m_bIsDrawingFrameCancelled, "Delayed confirmation rechecks modal ownership");
  m_bIsDrawingFrame = true;
  m_bIsAnimatingFrame = true;
  m_bIsDrawingFrameCancelled = false;
  OnMenuUpdate(0);
  probe.Expect(!m_bIsDrawingFrame && !m_bIsAnimatingFrame && m_bIsDrawingFrameCancelled, "Menu update cancels a frame even when its input context stops");
  ResetFrame();
 }
 void DCO_TestFrameRecovery(DCO_MenuLayoutProbe probe)
 {
  m_vCursorPosClick = Vector(1, 1, 0);
  DrawFrameDown(false);
  probe.Expect(!m_bIsDrawingFrameCancelled && m_bIsAnimatingFrame && !m_bIsDrawingFrameIsToggle, "World box selection starts again after modal close");
  DrawFrameDown(true);
  probe.Expect(!m_bIsDrawingFrameCancelled && m_bIsAnimatingFrame && m_bIsDrawingFrameIsToggle, "Ctrl box selection retains native mode switching");
  ResetFrame();
  m_bIsAnimatingFrame = false;
 }
}

modded class SCR_ContextMenuActionsEditorUIComponent
{
 void DCO_TestBlockedRelease(DCO_MenuLayoutProbe probe)
 {
  m_bDCO_ContextPressAllowed = true;
  m_bEditorIsSelectingState = false;
  OnOpenActionsMenuUp();
  probe.Expect(!m_bDCO_ContextPressAllowed && m_bEditorIsSelectingState, "Blocked right release resets native selection eligibility");
 }
}

modded class DCO_TriggerSyncDrag
{
 void DCO_TestModalInterruption(DCO_MenuLayoutProbe probe)
 {
  m_bDragging = true;
  OnRender(null);
  probe.Expect(!m_bDragging && !m_Group, "Render cancels an interrupted trigger gesture without a release event");
  Stop();
 }
}

modded class SCR_BaseContextMenuEditorUIComponent
{
 protected int m_iProbeCloses;
 protected void DCO_ProbeClose(bool open) { if (!open) m_iProbeCloses++; }
 void DCO_TestCloseOwnership(Widget nativePopup, Widget newFocus, DCO_MenuLayoutProbe probe)
 {
  m_ContextMenu = nativePopup;
  m_WorkSpace = GetGame().GetWorkspace();
  GetOnContextMenuToggle().Insert(DCO_ProbeClose);
  m_ContextMenu.SetVisible(true);
  CloseContextMenu();
  probe.Expect(!m_ContextMenu.IsVisible() && m_iProbeCloses == 1, "Native handoff closes its widget and emits its lifecycle event");
  probe.Expect(m_WorkSpace.GetFocusedWidget() == newFocus, "Native handoff preserves a newer window's focus");
  CloseContextMenu();
  probe.Expect(m_WorkSpace.GetFocusedWidget() == newFocus && m_iProbeCloses == 2, "Late hidden native close cannot steal focus");
  m_ContextMenu.SetVisible(true);
  m_WorkSpace.SetFocusedWidget(m_ContextMenu.FindAnyWidget("DCO_Menu_0"));
  CloseContextMenu();
  probe.Expect(!m_WorkSpace.GetFocusedWidget() && !m_ContextMenu.IsVisible(), "Native close still releases focus it owns");
  GetOnContextMenuToggle().Remove(DCO_ProbeClose);
  m_ContextMenu = null;
  m_WorkSpace = null;
 }
}

modded class DCO_GMContextMenu
{
 protected int m_iProbeActions;
 protected void DCO_ProbeAction(int id, SCR_EditableEntityComponent entity)
 {
  m_iProbeActions++;
  array<string> labels = {"Next menu"};
  array<int> ids = {2};
  Show(labels, ids, 20, 20, null, null);
 }
 void DCO_TestTransitions(DCO_MenuLayoutProbe probe)
 {
  ScriptInvoker callback = new ScriptInvoker();
  callback.Insert(DCO_ProbeAction);
  array<string> labels = {"Example"};
  array<int> ids = {1};
  array<bool> disabled = {false};
  ShowWithAvailability(labels, ids, disabled, 20, 20, callback, null);
  probe.Expect(!OnMenuButton(m_Btns[0]) && m_iProbeActions == 0 && IsOpen(), "Disabled action cannot close or dispatch the menu");
  Show(labels, ids, 20, 20, callback, null);
  probe.Expect(OnMenuButton(m_Btns[0]) && m_iProbeActions == 1 && IsOpen() && m_ActionIds[0] == 2, "Action may synchronously reopen the shared menu");
  m_wMenu.SetVisible(false);
  ReconcileVisibility();
  probe.Expect(!m_wBackdrop.IsVisible() && !m_OnAction, "Orphan dropdown catcher and callback are released");
  Show(labels, ids, 20, 20, callback, null);
  m_wBackdrop.SetVisible(false);
  ReconcileVisibility();
  probe.Expect(!IsOpen(), "Menu without its catcher cannot remain an invisible modal owner");
  Show(labels, ids, 20, 20, callback, null);
  array<string> empty = {};
  Show(empty, ids, 20, 20, callback, null);
  probe.Expect(!IsOpen() && !m_wBackdrop.IsVisible(), "Empty menu request releases both surfaces");
  probe.Expect(!OnMenuButton(m_Btns[0]) && m_iProbeActions == 1, "Late click after close cannot dispatch a stale action");
  callback.Remove(DCO_ProbeAction);
 }
}

modded class DCO_GMDraggable
{
 void DCO_TestInterruptions(Widget grip, DCO_MenuLayoutProbe probe)
 {
  probe.Expect(!OnMouseButtonDown(grip, 0, 0, 0) && m_Dragging, "Drag leaves native button press routing intact");
  OnMouseButtonUp(grip, 0, 0, 1);
  probe.Expect(m_Dragging, "Other button release cannot end a left drag");
  probe.Expect(!OnMouseButtonUp(grip, 0, 0, 0) && !m_Dragging, "Drag leaves native button release routing intact");
  OnMouseButtonDown(grip, 0, 0, 0);
  OnFocusLost(grip, 0, 0);
  probe.Expect(!m_Dragging, "Focus loss cancels drag");
  OnMouseButtonDown(grip, 0, 0, 0);
  OnHide(grip);
  probe.Expect(!m_Dragging, "Hidden grip cancels drag");
  OnMouseButtonDown(grip, 0, 0, 0);
  OnDisable(grip);
  probe.Expect(!m_Dragging, "Disabled grip cancels drag");
  OnMouseButtonDown(grip, 0, 0, 0);
  m_Target.SetEnabled(false);
  OnDragTick();
  probe.Expect(!m_Dragging, "Disabled ancestor cancels drag on its next tick");
  m_Target.SetEnabled(true);
 }
}

modded class DCO_GMResizable
{
 void DCO_TestInterruptions(Widget grip, DCO_MenuLayoutProbe probe)
 {
  probe.Expect(!OnMouseButtonDown(grip, 0, 0, 0) && m_Resizing, "Resize leaves native button press routing intact");
  OnMouseButtonUp(grip, 0, 0, 1);
  probe.Expect(m_Resizing, "Other button release cannot end a left resize");
  probe.Expect(!OnMouseButtonUp(grip, 0, 0, 0) && !m_Resizing, "Resize leaves native button release routing intact");
  OnMouseButtonDown(grip, 0, 0, 0);
  OnFocusLost(grip, 0, 0);
  probe.Expect(!m_Resizing, "Focus loss cancels resize");
  OnMouseButtonDown(grip, 0, 0, 0);
  OnHide(grip);
  probe.Expect(!m_Resizing, "Hidden grip cancels resize");
  OnMouseButtonDown(grip, 0, 0, 0);
  OnDisable(grip);
  probe.Expect(!m_Resizing, "Disabled grip cancels resize");
  OnMouseButtonDown(grip, 0, 0, 0);
  m_Target.SetVisible(false);
  OnResizeTick();
  probe.Expect(!m_Resizing, "Hidden ancestor cancels resize on its next tick");
  m_Target.SetVisible(true);
 }
}
