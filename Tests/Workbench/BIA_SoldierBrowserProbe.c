class BIA_SoldierBrowserProbe
{
 int passed;
 ref array<string> failures = {};
 void Expect(bool value, string label)
 {
  if (value) passed++;
  else failures.Insert(label);
 }
 void Run()
 {
  CheckWearReplies();
  WorkspaceWidget workspace = GetGame().GetWorkspace();
  if (!workspace) { failures.Insert("Workspace unavailable"); return; }
  Widget previousFocus = workspace.GetFocusedWidget();
  Widget root = workspace.CreateWidgets("{AB205AD4E2000003}UI/layouts/Menus/ArmoryV2/GRSA_ScreenSoldier.layout");
  Expect(root != null, "Real Soldier layout creates");
  if (!root) return;
  BIA_SoldierScreen screen = BIA_SoldierScreen.Cast(root.FindHandler(BIA_SoldierScreen));
  Expect(screen != null, "Real Soldier screen handler exists");
  BIA_DraftService service = new BIA_DraftService();
  service.m_Draft = new BIA_Kit();
  BIA_DraftService previous = BIA_DraftService.BIA_TestSwapBrowserService(service);
  if (screen) screen.BIA_TestBrowser(root, this);
  BIA_DraftService.BIA_TestSwapBrowserService(previous);
  root.RemoveFromHierarchy();
  if (previousFocus) workspace.SetFocusedWidget(previousFocus);
 }

 void CheckWearReplies()
 {
  BIA_DraftService first = new BIA_DraftService();
  first.m_bDraftDirty = true;
  int older = first.BeginApplyRequest();
  first.NotifyDraftChanged();
  Expect(!first.AcceptApplyResult(older, BIA_EApplyStatus.SUCCESS) && first.m_bDraftDirty, "Reply for an older draft cannot clear newer edits");
  int current = first.BeginApplyRequest();
  Expect(first.AcceptApplyResult(current, BIA_EApplyStatus.SUCCESS) && !first.m_bDraftDirty, "Matching success acknowledges only the sent revision");
  Expect(!first.AcceptApplyResult(current, BIA_EApplyStatus.SUCCESS), "Duplicate acknowledgement is ignored");
  first.m_bDraftDirty = true;
  older = first.BeginApplyRequest();
  current = first.BeginApplyRequest();
  Expect(!first.AcceptApplyResult(older, BIA_EApplyStatus.FAILED_INVALID) && first.m_bDraftDirty, "Older failure cannot consume the current request");
  Expect(first.AcceptApplyResult(current, BIA_EApplyStatus.FAILED_INVALID) && first.m_bDraftDirty, "Current failure preserves dirty state");
  older = first.BeginApplyRequest();
  BIA_DraftService reopened = new BIA_DraftService();
  reopened.m_bDraftDirty = true;
  current = reopened.BeginApplyRequest();
  Expect(current != older, "Request identity spans reopened sessions");
  Expect(!reopened.AcceptApplyResult(older, BIA_EApplyStatus.SUCCESS) && reopened.m_bDraftDirty, "Old session result cannot acknowledge a reopened menu");
  Expect(!reopened.AcceptApplyResult(0, BIA_EApplyStatus.SUCCESS), "Missing request identity is rejected");
  Expect(reopened.AcceptApplyResult(current, BIA_EApplyStatus.PARTIAL) && !reopened.m_bDraftDirty, "Matching partial result retains existing acknowledgement semantics");
  current = reopened.BeginApplyRequest();
  reopened.NotifyDraftChanged();
  reopened.m_bDraftDirty = true;
  Expect(!reopened.AcceptApplyResult(current, BIA_EApplyStatus.PARTIAL) && reopened.m_bDraftDirty, "Old partial result preserves newer edits");
 }
}

modded class BIA_DraftService
{
 static BIA_DraftService BIA_TestSwapBrowserService(BIA_DraftService service)
 {
  BIA_DraftService previous = s_Instance;
  s_Instance = service;
  return previous;
 }
}

modded class BIA_ItemRowComponent
{
 void BIA_TestBrowserPadSelect() { OnMenuSelect(); }
 bool BIA_TestBrowserPressEnabled() { return m_bActivateOnPress; }
 void BIA_TestBrowserPlus() { if (m_PlusHandler) m_PlusHandler.OnClick(m_wPlusButton, -100, -100, 0); }
}

modded class BIA_ItemListPanel
{
 BIA_ItemRowComponent BIA_TestBrowserFirstRow()
 {
  if (m_aRows.IsEmpty()) return null;
  return m_aRows[0];
 }
}

modded class BIA_SoldierScreen
{
 int m_iBrowserTestActivations;
 int m_iBrowserTestQuantity;
 void BIA_TestBrowserCount(BIA_ItemRowComponent row) { m_iBrowserTestActivations++; }
 void BIA_TestBrowserQuantity(BIA_ItemRowComponent row, int delta) { m_iBrowserTestQuantity += delta; }
 void BIA_TestBrowser(Widget root, BIA_SoldierBrowserProbe probe)
 {
  m_wRoot = root;
  m_wGearCardList = root.FindAnyWidget("GearCardList");
  m_wCustomizeRail = root.FindAnyWidget("LeftRail");
  m_wCustomizeList = root.FindAnyWidget("WeaponCardList");
  m_ItemList = new BIA_ItemListPanel(root, "ItemListPanel", "ItemListTitle", "SoldierItemList", "SoldierItemScroll", "SoldierSearchBox", "ItemListBackControls", "ItemListFilters");
  array<string> labels = {"Regression Headgear", "Regression Vests", "Regression Armored Vests"};
  array<ref BIA_ArmoryCategory> categories = {};
  foreach (int index, string label : labels)
  {
   BIA_SoldierCard card = new BIA_SoldierCard();
   card.m_sLabel = label;
   card.m_iClothingSlot = index;
   BIA_ArmoryCategory category = new BIA_ArmoryCategory();
   categories.Insert(category);
   card.m_Category = category;
   card.m_Category.m_sDisplayName = label;
   card.m_Category.m_eItemTypes = 0;
   SpawnCard(card, m_wGearCardList);
   probe.Expect(card.m_Row != null, label + " creates a production category row");
   if (!card.m_Row) continue;
   card.m_Row.m_OnEntryClicked.Insert(BIA_TestBrowserCount);
  }
  array<int> order = {0, 1, 2, 0, 2, 1};
  foreach (int selected : order)
  {
   if (selected >= m_aCards.Count()) continue;
   BIA_SoldierCard card = m_aCards[selected];
   BIA_ItemRowComponent row = card.m_Row;
   Widget rowRoot = row.GetRootWidget();
   Widget labelWidget = rowRoot.FindAnyWidget("RowName");
   int before = m_iBrowserTestActivations;
   row.OnFocus(rowRoot, -100, -100);
   bool consumed = row.OnMouseButtonDown(labelWidget, -100, -100, 0);
   probe.Expect(consumed && m_iBrowserTestActivations == before + 1, "First pointer press opens " + card.m_sLabel);
   probe.Expect(m_ItemList.IsOpen() && m_ActiveCard == card, "Correct item panel is visible immediately for " + card.m_sLabel);
   row.OnMouseButtonUp(labelWidget, -100, -100, 0);
   row.OnClick(labelWidget, -100, -100, 0);
   probe.Expect(m_iBrowserTestActivations == before + 1, "Release does not activate twice for " + card.m_sLabel);
   CloseItemList(false);
   probe.Expect(!m_ItemList.IsOpen(), "Back closes " + card.m_sLabel);
  }
  if (!m_aCards.IsEmpty())
  {
   BIA_ItemRowComponent row = m_aCards[0].m_Row;
   int before = m_iBrowserTestActivations;
   row.SetEnabled(false, false);
   row.OnMouseButtonDown(row.GetRootWidget(), -100, -100, 0);
   probe.Expect(m_iBrowserTestActivations == before && !m_ItemList.IsOpen(), "Disabled category does not open");
   row.SetEnabled(true, false);
   row.BIA_TestBrowserPadSelect();
   probe.Expect(m_iBrowserTestActivations == before + 1 && m_ItemList.IsOpen(), "Native controller activation still opens the panel");
  }
  CloseItemList(false);
  BIA_TestItemSelection(probe);
  ClearCards();
  ClearActions();
  m_ItemList.Destroy();
  m_ItemList = null;
 }

 void BIA_TestItemSelection(BIA_SoldierBrowserProbe probe)
 {
  if (m_aCards.Count() < 2) return;
  BIA_DraftService service = BIA_DraftService.Get();
  m_ActiveCard = m_aCards[1];
  m_FocusedCard = m_ActiveCard;
  m_ItemList.m_OnItemClicked.Insert(OnListRowClicked);
  m_ItemList.m_OnItemClicked.Insert(BIA_TestBrowserCount);
  m_ItemList.m_OnQtyDelta.Insert(BIA_TestBrowserQuantity);
  service.m_OnDraftChanged.Insert(OnDraftChanged);
  BIA_ItemEntry entry = new BIA_ItemEntry();
  entry.m_Prefab = "{4B57C11AA5161760}Prefabs/Characters/Vests/Vest_PASGT/Vest_PASGT.et";
  entry.m_sDisplayName = "PASGT Vest";
  array<ref BIA_ItemEntry> items = {entry};
  m_ItemList.Open("ARMORED VESTS", items, ResourceName.Empty, "EQUIPPED", false, false);
  BIA_ItemRowComponent row = m_ItemList.BIA_TestBrowserFirstRow();
  probe.Expect(row != null, "Populated selection list creates a real vest row");
  if (row)
  {
   Widget label = row.GetRootWidget().FindAnyWidget("RowName");
   int before = m_iBrowserTestActivations;
   probe.Expect(row.BIA_TestBrowserPressEnabled(), "Equipment selection uses first-press activation");
   row.OnFocus(row.GetRootWidget(), -100, -100);
   row.OnMouseButtonDown(label, -100, -100, 0);
   probe.Expect(CurrentPrefabFor(m_ActiveCard, service) == entry.m_Prefab, "First vest press equips the draft immediately");
   row.OnMouseButtonUp(label, -100, -100, 0);
   row.OnClick(label, -100, -100, 0);
   probe.Expect(m_iBrowserTestActivations == before + 1 && CurrentPrefabFor(m_ActiveCard, service) == entry.m_Prefab, "Release cannot remove the vest just equipped");
   row.OnMouseButtonDown(label, -100, -100, 0);
   row.OnMouseButtonUp(label, -100, -100, 0);
   row.OnClick(label, -100, -100, 0);
   probe.Expect(m_iBrowserTestActivations == before + 2 && CurrentPrefabFor(m_ActiveCard, service).IsEmpty(), "Second gesture removes the vest once");
   row.SetEnabled(false, false);
   row.OnMouseButtonDown(label, -100, -100, 0);
   probe.Expect(CurrentPrefabFor(m_ActiveCard, service).IsEmpty(), "Disabled item does not equip");
   row.SetEnabled(true, false);
   row.BIA_TestBrowserPadSelect();
   probe.Expect(CurrentPrefabFor(m_ActiveCard, service) == entry.m_Prefab, "Controller selection still equips the item");
  }
  service.m_OnDraftChanged.Remove(OnDraftChanged);
  m_ItemList.Open("CONTENTS", items, ResourceName.Empty, string.Empty, false, true);
  row = m_ItemList.BIA_TestBrowserFirstRow();
  probe.Expect(row != null, "Quantity list creates a row");
  if (row)
  {
   probe.Expect(!row.BIA_TestBrowserPressEnabled(), "Quantity rows retain native child control handling");
   row.OnFocus(row.GetRootWidget(), -100, -100);
   Widget plus = row.GetRootWidget().FindAnyWidget("QtyPlus");
   int before = m_iBrowserTestActivations;
   row.OnMouseButtonDown(plus, -100, -100, 0);
   bool consumed = row.OnClick(plus, -100, -100, 0);
   probe.Expect(!consumed && m_iBrowserTestActivations == before, "Stepper gesture cannot select its item row");
   row.BIA_TestBrowserPlus();
   probe.Expect(m_iBrowserTestQuantity == 1 && m_iBrowserTestActivations == before, "Plus delegates exactly one quantity change");
  }
  CloseItemList(false);
 }
}
