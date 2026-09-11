class BIA_StandPlacementProbe
{
 int passed;
 ref array<string> failures = {};
 ref array<string> details = {};
 void Expect(bool value, string label)
 {
  if (value) passed++;
  else failures.Insert(label);
 }
 void Run()
 {
  BIA_StageCore core = new BIA_StageCore();
  if (!core.EnsureWorld("BIA_StandPlacementTest"))
  {
   failures.Insert("Private preview world unavailable");
   return;
  }
  IEntity stand = core.GetWorld().FindEntityByName("BIA_ArmorStand");
  Expect(stand != null, "Authored stand loads in Arsenal environment");
  if (stand)
  {
   details.Insert("Authored stand position: " + stand.GetOrigin().ToString());
   details.Insert("Authored stand angles: " + stand.GetYawPitchRoll().ToString());
   Expect(vector.Distance(stand.GetOrigin(), "0.342 0.388 -0.713") < 0.001, "User placement retained");
   Expect(Math.AbsFloat(stand.GetYawPitchRoll()[0] + 131.552) < 0.001, "Front-facing stand rotation retained");
   Expect((stand.GetFlags() & EntityFlags.VISIBLE) == 0, "Stand starts hidden outside supported-item presentation");
   vector authoredHome[4];
   stand.GetTransform(authoredHome);
   BIA_DraftService testService = new BIA_DraftService();
   testService.m_Draft = new BIA_Kit();
   testService.m_Draft.SetClothing(0, "{B74A4FF0DD8BB116}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et");
   BIA_DraftService previousService = BIA_DraftService.DCO_TestReplaceInstance(testService);
   BIA_WeaponStage stage = new BIA_WeaponStage(core);
   stage.TestStandPlacement(this, stand, "Prefabs/Characters/Vests/Vest_PASGT/Vest_PASGT.et", false);
   stage.TestStandPlacement(this, stand, "Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et", true);
   stand.SetTransform(authoredHome);
   stand.SetOrigin("0.51 0.42 -0.66");
   stand.SetYawPitchRoll("63 7 -4");
   stand.SetScale(1.25);
   stand.Update();
   stage.TestStandPlacement(this, stand, "Prefabs/Characters/Vests/Vest_PASGT/Vest_PASGT.et", false);
   stand.SetTransform(authoredHome);
   stand.Update();
   stage.Destroy();
   BIA_DraftService.DCO_TestReplaceInstance(previousService);
  }
  core.Release();
  Expect(!core.IsAlive(), "Private preview world released");
 }
}

modded class BIA_WeaponStage
{
 void TestStandPlacement(BIA_StandPlacementProbe probe, IEntity stand, ResourceName prefab, bool helmet)
 {
  Resource resource = Resource.Load(prefab);
  if (!resource || !resource.IsValid())
  {
   probe.failures.Insert("Missing test item: " + prefab);
   return;
  }
  IEntity source = GetGame().SpawnEntityPrefabLocal(resource, m_Core.GetWorld());
  InventoryItemComponent item;
  if (source) item = InventoryItemComponent.Cast(source.FindComponent(InventoryItemComponent));
  if (item) m_Weapon = item.CreatePreviewEntity(m_Core.GetWorld(), BIA_StageCore.CAMERA);
  probe.details.Insert("Resource: " + resource.GetResource().GetResourceName());
  BIA_PreviewDress.DeleteLocalHierarchy(source);
  probe.Expect(m_Weapon && m_Weapon.GetVObject(), "Native item mesh: " + prefab);
  if (!m_Weapon) return;
  m_fItemHomeScale = m_Weapon.GetScale();
  m_ArmorStand = stand;
  stand.GetTransform(m_vStandHomeTransform);
  m_vStandHomeAngles = stand.GetYawPitchRoll();
  vector home[4];
  stand.GetTransform(home);
  m_bItemOnStand = true;
  m_bVestOnStand = !helmet;
  m_bHelmetOnStand = helmet;
  stand.SetFlags(EntityFlags.VISIBLE, true);
  RefreshCompanionHelmet();
  FrameStation(true);

  vector mins, maxs;
  m_Weapon.GetBounds(mins, maxs);
  vector top = Vector((mins[0] + maxs[0]) * 0.5, maxs[1], (mins[2] + maxs[2]) * 0.5);
  float height = 0.549;
  if (helmet) height = 0.7643;
  probe.Expect(vector.Distance(m_Weapon.CoordToParent(top), stand.CoordToParent(Vector(0, height, 0))) < 0.001, "Support aligns: " + prefab);
  float expectedScale = m_fItemHomeScale;
  if (!helmet)
   expectedScale = Math.Min(m_fItemHomeScale, Math.Max(0.549 * stand.GetScale() - 0.01, 0.001) / (maxs[1] - mins[1]));
  probe.Expect(Math.AbsFloat(m_Weapon.GetScale() - expectedScale) < 0.001, "Native item scale and world clearance retained: " + prefab);
  probe.Expect(vector.Distance(stand.GetOrigin(), home[3]) < 0.001, "Stand remains at authored position");
  probe.Expect(vector.Distance(m_Weapon.GetYawPitchRoll(), stand.GetYawPitchRoll()) < 0.001, "Item faces authored stand direction");
  probe.Expect(m_fBoundsDiag > 0.749, "Camera includes the complete stand");
  if (!helmet)
  {
   probe.Expect(m_CompanionHelmet && m_CompanionHelmet.GetVObject(), "Draft helmet accompanies vest on stand");
   if (m_CompanionHelmet)
   {
    vector helmetMins, helmetMaxs;
    m_CompanionHelmet.GetBounds(helmetMins, helmetMaxs);
    vector helmetTop = Vector((helmetMins[0] + helmetMaxs[0]) * 0.5, helmetMaxs[1], (helmetMins[2] + helmetMaxs[2]) * 0.5);
    probe.Expect(vector.Distance(m_CompanionHelmet.CoordToParent(helmetTop), stand.CoordToParent(Vector(0, 0.7643, 0))) < 0.001, "Companion helmet rests on crown");
    probe.Expect(vector.Distance(m_CompanionHelmet.GetYawPitchRoll(), stand.GetYawPitchRoll()) < 0.001, "Companion helmet faces with stand");
   }
  }
  vector bottom = top;
  bottom[1] = mins[1];
  probe.Expect(m_Weapon.CoordToParent(bottom)[1] >= stand.GetOrigin()[1], "Item clears the stand base and table");
  probe.details.Insert(prefab + " origin=" + m_Weapon.GetOrigin().ToString() + " bounds=" + mins.ToString() + " / " + maxs.ToString());

  m_fSpinYaw = 90;
  m_fFocusPitch = 12;
  ApplyRestPose(true);
  probe.Expect(vector.Distance(m_Weapon.CoordToParent(top), stand.CoordToParent(Vector(0, height, 0))) < 0.001, "Support survives rotation and focus");
  probe.Expect(Math.AbsFloat(m_Weapon.GetYawPitchRoll()[1] - m_vStandHomeAngles[1]) < 0.001, "Focus does not tip supported item");
  probe.Expect(vector.Distance(stand.GetOrigin(), home[3]) < 0.001, "Rotation does not recenter stand");

  FocusPoint("100 -100 100");
  vector focusMins, focusMaxs;
  m_Weapon.GetBounds(focusMins, focusMaxs);
  bool focusInside = true;
  for (int axis = 0; axis < 3; axis++)
   focusInside = focusInside && m_vFocusLocal[axis] >= focusMins[axis] && m_vFocusLocal[axis] <= focusMaxs[axis];
  probe.Expect(focusInside, "Stand hardpoint focus stays inside item bounds");
  probe.Expect(m_fFocusYawTarget == 0 && m_fFocusPitchTarget == 0, "Stand hardpoint focus preserves front presentation");
  probe.Expect(m_fFocusDist >= m_fBoundsDiag * 0.819, "Stand hardpoint focus keeps surface clearance");

  vector position = m_Weapon.GetOrigin();
  IEntity child = GetGame().SpawnEntityPrefabLocal(Resource.Load("Prefabs/Weapons/Rifles/AK74/Rifle_AK74.et"), m_Core.GetWorld());
  if (child)
  {
   child.SetOrigin(position + "0.3 -0.2 0");
   m_Weapon.AddChild(child, -1, EAddChildFlags.AUTO_TRANSFORM);
   ApplyRestPose(true);
   probe.Expect(vector.Distance(m_Weapon.GetOrigin(), position) < 0.001, "Mounted child cannot shift body anchor");
  }
  else probe.failures.Insert("Attachment bounds fixture unavailable");

  ClearStage();
  vector restored[4];
  stand.GetTransform(restored);
  bool same = true;
  for (int axis = 0; axis < 4; axis++)
   same = same && vector.Distance(home[axis], restored[axis]) < 0.001;
  probe.Expect(same, "Item release restores entire authored stand transform");
  probe.Expect((stand.GetFlags() & EntityFlags.VISIBLE) == 0, "Stand hides on item release");
  probe.Expect(!m_bItemOnStand && !m_bVestOnStand && !m_bHelmetOnStand && !m_Weapon && !m_CompanionHelmet, "Item release clears stand state");
 }
}

modded class BIA_DraftService
{
 static BIA_DraftService DCO_TestReplaceInstance(BIA_DraftService service)
 {
  BIA_DraftService previous = s_Instance;
  s_Instance = service;
  return previous;
 }
}
