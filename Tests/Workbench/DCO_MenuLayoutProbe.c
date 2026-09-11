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
  array<string> panels = {"DCO_CreateBrowser", "DCO_EditTree", "DCO_TopBar", "DCO_ContextMenu", "DCO_ScenarioPanel", "DCO_ScenarioBackdrop", "DCO_MenuBackdrop"};
  foreach (string name : panels) { Widget panel = root.FindAnyWidget(name); if (panel) panel.SetVisible(false); }
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
