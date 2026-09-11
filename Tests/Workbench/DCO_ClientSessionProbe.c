class DCO_ClientSessionProbe
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
  WorkspaceWidget workspace = GetGame().GetWorkspace();
  if (!workspace) { failures.Insert("Native workspace unavailable"); return; }
  CheckList(workspace, "{6A4338ABF0020001}UI/layouts/Menus/ArmoryV2/GRSA_ItemListPanel.layout", "ItemList", "ItemScroll", "ItemSearchBox");
  CheckList(workspace, "{AB205AD4E2000003}UI/layouts/Menus/ArmoryV2/GRSA_ScreenSoldier.layout", "SoldierItemList", "SoldierItemScroll", "SoldierSearchBox");
  BIA_StageCore stage = new BIA_StageCore();
  if (!stage.EnsureWorld("BifrostNestedStorageTest")) { failures.Insert("Private test world unavailable"); return; }
  BaseWorld world = stage.GetWorld();
  IEntity outer = GetGame().SpawnEntityPrefabLocal(Resource.Load("{FD0890CE5C6D26CB}Prefabs/Characters/Vests/Vest_ALICE/Vest_ALICE_assembled_base.et"), world);
  IEntity inner = GetGame().SpawnEntityPrefabLocal(Resource.Load("{FD0890CE5C6D26CB}Prefabs/Characters/Vests/Vest_ALICE/Vest_ALICE_assembled_base.et"), world);
  IEntity pouch = GetGame().SpawnEntityPrefabLocal(Resource.Load("{9A21918AC35AC182}Prefabs/Characters/Vests/Vest_ALICE/Vest_ALICE_buttpack.et"), world);
  Expect(outer && inner && pouch, "Native nested-storage fixtures spawn");
  if (outer && inner && pouch)
  {
   BaseInventoryStorageComponent outerMounts = BIA_ItemIntel.AttachmentStorage(outer);
   BaseInventoryStorageComponent innerMounts = BIA_ItemIntel.AttachmentStorage(inner);
   BaseInventoryStorageComponent cargo = BaseInventoryStorageComponent.Cast(pouch.FindComponent(SCR_UniversalInventoryStorageComponent));
   if (outerMounts && innerMounts && cargo && outerMounts.GetSlotsCount() > 1 && innerMounts.GetSlotsCount() > 4)
   {
    outerMounts.GetSlot(1).AttachEntity(inner);
    innerMounts.GetSlot(4).AttachEntity(pouch);
    Expect(outerMounts.GetSlot(1).GetAttachedEntity() == inner && innerMounts.GetSlot(4).GetAttachedEntity() == pouch, "Two native attachment levels are connected");
    array<BaseInventoryStorageComponent> result = {};
    BIA_ItemIntel.CollectContainerStorages(outer, result);
    Expect(result.Find(cargo) >= 0, "Collector reaches cargo two attached items below vest");
    int occurrences;
    foreach (BaseInventoryStorageComponent store : result) if (store == cargo) occurrences++;
    Expect(occurrences == 1, "Nested cargo appears once");
    Expect(result.Find(outerMounts) < 0 && result.Find(innerMounts) < 0, "Mount routers are not cargo targets");
    Expect(cargo.CanStoreResource("{11B9CC1FB4AEE740}Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_Base.et", -1), "Deep pouch accepts a magazine");
    innerMounts.GetSlot(4).DetachEntity();
    BIA_ItemIntel.CollectContainerStorages(outer, result);
    Expect(result.Find(cargo) < 0, "Detached pouch is no longer a vest target");
   }
   else failures.Insert("Fixture native mounting/storage components missing");
  }
  if (pouch) BIA_PreviewDress.DeleteLocalHierarchy(pouch);
  if (inner) BIA_PreviewDress.DeleteLocalHierarchy(inner);
  if (outer) BIA_PreviewDress.DeleteLocalHierarchy(outer);
  stage.Release();
 }
 void CheckList(WorkspaceWidget workspace, ResourceName path, string listName, string scrollName, string searchName)
 {
  Widget root = workspace.CreateWidgets(path);
  Expect(root != null, "Native list layout creates: " + path);
  if (!root) return;
  BIA_ItemListPanel panel = new BIA_ItemListPanel(root, "ItemListPanel", "ItemListTitle", listName, scrollName, searchName, "ItemListBackControls", "ItemListFilters");
  panel.DCO_CheckModFilter(this);
  panel.Close();
  panel.Destroy();
  root.RemoveFromHierarchy();
 }
}
modded class BIA_ItemEntry
{
 void DCO_SetTestMods(string addon)
 {
  m_aSourceMods = {addon};
 }
}
modded class BIA_ItemListPanel
{
 void DCO_CheckModFilter(DCO_ClientSessionProbe probe)
 {
  probe.Expect(m_ModFilter != null, "Native mod selector is bound");
  if (!m_ModFilter) return;
  BIA_ItemEntry a = new BIA_ItemEntry();
  a.m_Prefab = "{B74A4FF0DD8BB116}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et";
  a.m_sDisplayName = "Fixture Helmet";
  a.DCO_SetTestMods("BifrostDev");
  BIA_ItemEntry b = new BIA_ItemEntry();
  b.m_Prefab = a.m_Prefab;
  b.m_sDisplayName = "Other Helmet";
  b.DCO_SetTestMods("ArmaReforger");
  array<ref BIA_ItemEntry> items = {a, b};
  Open("ITEMS", items, ResourceName.Empty, string.Empty, false, false);
  probe.Expect(m_ModFilter.GetNumItems() == 3, "All Mods plus distinct source mods");
  probe.Expect(m_aVisibleItems.Count() == 2, "All Mods includes both entries");
  int index = m_aModIds.Find("BifrostDev");
  m_ModFilter.m_OnChanged.Invoke(m_ModFilter, index);
  probe.Expect(m_aVisibleItems.Count() == 1 && m_aVisibleItems[0] == a, "Native selection event filters the owning mod");
  m_sSearchFilter = "other";
  RebuildRows();
  probe.Expect(m_aVisibleItems.IsEmpty(), "Search intersects selected mod");
  m_ModFilter.m_OnChanged.Invoke(m_ModFilter, 0);
  m_sSearchFilter = "bifrost-dev";
  RebuildRows();
  probe.Expect(m_aVisibleItems.Count() == 1 && m_aVisibleItems[0] == a, "Search recognizes displayed mod title");
 }
}
