class DCO_PropertyScopeProbe
{
 SCR_EditorAttributeList LoadList(ResourceName path)
 {
  Resource resource = BaseContainerTools.LoadContainer(path);
  if (!resource || !resource.IsValid()) return null;
  return SCR_EditorAttributeList.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
 }

 void Run(Widget root, DCO_MenuLayoutProbe probe)
 {
  SCR_EditorAttributeList list = LoadList("{5DC0DC0A77E1B5E0}Configs/Editor/AttributeLists/DCO_Attributes.conf");
  probe.Expect(list && list.GetAttributesCount() == 110, "All 110 registered Bifrost attributes are present");
  if (!list) return;
  array<SCR_BaseEditorAttribute> attributes = {};
  list.InsertAllAttributes(attributes);
  BIA_StageCore stage = new BIA_StageCore();
  probe.Expect(stage.EnsureWorld("BifrostPropertyScopeTest"), "Property fixtures have a private preview world");
  if (!stage.IsAlive()) return;
  array<ResourceName> prefabs = {
   "{D7B8408B96F4DF79}PrefabsEditable/Auto/Compositions/Slotted/SlotRoadSmall/E_Barricade_S_US_01.et",
   "{3B5DEAE089B2E3A5}Prefabs/E_DCO_FxAudio.et",
   "{EEFC7B09110761DE}Prefabs/E_DCO_Trigger.et",
   "{DCA6090560000000}Prefabs/E_DCO_Teleporter.et"
  };
  array<IEntity> entities = {};
  array<SCR_EditableEntityComponent> editables = {};
  for (int fixture = 0; fixture < prefabs.Count(); fixture++)
  {
   IEntity entity = GetGame().SpawnEntityPrefabLocal(Resource.Load(prefabs[fixture]), stage.GetWorld());
   entities.Insert(entity);
   SCR_EditableEntityComponent editable;
   if (entity) editable = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
   editables.Insert(editable);
   probe.Expect(editable != null, "Property fixture has an editable component: " + prefabs[fixture]);
   if (!editable) continue;
   foreach (SCR_BaseEditorAttribute attribute : attributes)
   {
    bool expected = false;
    if (fixture == 1) expected = attribute.IsInherited(DCO_AudioSettingEditorAttribute);
    if (fixture == 2) expected = attribute.IsInherited(DCO_TriggerAttributeBase) || attribute.IsInherited(DCO_TriggerEnabledEditorAttribute);
    if (fixture == 3) expected = attribute.IsInherited(DCO_TeleporterActivationEditorAttribute);
    probe.Expect((attribute.ReadVariable(editable, null) != null) == expected,
     string.Format("Fixture %1 accepts only its own settings: %2", fixture, attribute.Type()));
   }
  }
  if (editables.Count() >= 2 && editables[0] && editables[1])
  {
   array<Managed> mixed = {editables[1], editables[0]};
   array<Managed> supported = {editables[1]};
   foreach (SCR_BaseEditorAttribute scoped : attributes)
   {
    probe.Expect(!DCO_EditorAttributeScope.AppliesToAll(scoped, mixed, null), "Mixed roadblock/audio selection excludes " + scoped.Type().ToString());
    probe.Expect(DCO_EditorAttributeScope.AppliesToAll(scoped, supported, null) == scoped.IsInherited(DCO_AudioSettingEditorAttribute), "Audio selection retains only " + scoped.Type().ToString());
   }
  }
  foreach (IEntity spawned : entities)
  {
   if (spawned) SCR_EntityHelper.DeleteEntityAndChildren(spawned);
  }
  entities.Clear();
  editables.Clear();
  stage.Release();
  probe.Expect(!stage.IsAlive(), "Property fixture world is released");

  probe.Expect(DCO_UIText.Plain("<color rgba='226,168,79,255'>RGB</color>") == "RGB", "Plain labels remove color formatting");
  probe.Expect(DCO_UIText.Plain("0 < 5 and 9 > 3") == "0 < 5 and 9 > 3", "Plain labels preserve comparison signs");
  probe.Expect(DCO_UIText.Plain("first<br/>second") == "first\nsecond", "Plain labels preserve line breaks");
  string blood = WidgetManager.Translate("#AR-Editor_Attribute_Blood_Description");
  probe.Expect(blood.Contains("<color"), "Actual localized Blood description contains native color markup");
  probe.Expect(!DCO_UIText.Plain("#AR-Editor_Attribute_Blood_Description").Contains("<color"), "Plain labels clean markup introduced by localization");
  WorkspaceWidget workspace = GetGame().GetWorkspace();
  Widget option = workspace.CreateWidgets("{2A2733736B074188}UI/layouts/DCO_GMScenarioOption.layout");
  probe.Expect(option != null, "Properties option layout creates");
  if (option)
  {
   RichTextWidget description = RichTextWidget.Cast(option.FindAnyWidget("DCO_OptionDescription"));
   probe.Expect(description != null, "Properties descriptions use native rich text");
   option.RemoveFromHierarchy();
  }
  for (int row = 0; row < 6; row++)
   probe.Expect(RichTextWidget.Cast(root.FindAnyWidget("DCO_Notif_Row" + row)) != null, "Notification row supports translated markup " + row);
  DCO_GMScenarioPanel panel = new DCO_GMScenarioPanel();
  panel.DCO_TestEmptyProperties(root, probe);
 }
}

modded class DCO_GMScenarioPanel
{
 void DCO_TestEmptyProperties(Widget root, DCO_MenuLayoutProbe probe)
 {
  Init(root, null);
  m_bCogSession = true;
  OnAttributesStart({});
  probe.Expect(CanOwnPropertySession() && m_bOpen && m_wPanel.IsVisible() && m_wBackdrop.IsVisible(), "Empty entity Properties retains Bifrost ownership");
  probe.Expect(!m_bCogSession && m_wTitle.GetText() == "PROPERTIES" && !m_wPresetBar.IsVisible(), "Empty entity selection cannot inherit Scenario Settings controls");
  probe.Expect(m_wPlaceholder && m_wPlaceholder.IsVisible() && m_Rows.IsEmpty(), "Empty Properties displays a clear message");
  OnAttributesEnded(null);
  probe.Expect(!m_bOpen && !m_wPanel.IsVisible() && !m_wBackdrop.IsVisible(), "Empty Properties closes without leaving a catcher");
  OnAttributesStart({});
  probe.Expect(m_wPlaceholder && m_wPlaceholder.IsVisible(), "Empty-state message survives reopening");
  OnAttributesEnded(null);
  Shutdown();
 }
}
