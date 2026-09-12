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
  details.Insert("Dynamic shoulder and third-party geometry");
  details.Insert("Authoritative workspace: C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev | BifrostDev | 6A0C2D6CE9809C6E");
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
   stage.TestStandPlacement(this, stand, "{4B57C11AA5161760}Prefabs/Characters/Vests/Vest_PASGT/Vest_PASGT.et", false);
   stage.TestStandPlacement(this, stand, "{2835A0EA3B79E63E}Prefabs/Characters/Vests/Vest_ALICE/Variants/Vest_ALICE_rifleman.et", false, true);
   stage.TestStandPlacement(this, stand, "{B74A4FF0DD8BB116}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et", true);
   stand.SetTransform(authoredHome);
   stand.SetOrigin("0.51 0.42 -0.66");
   stand.SetYawPitchRoll("63 7 -4");
   stand.SetScale(1.25);
   stand.Update();
   stage.TestStandPlacement(this, stand, "{4B57C11AA5161760}Prefabs/Characters/Vests/Vest_PASGT/Vest_PASGT.et", false);
   stand.SetTransform(authoredHome);
   stand.Update();
   stage.Destroy();
   BIA_DraftService.DCO_TestReplaceInstance(previousService);
  }
  TestWorldCross(core.GetWorld(), false);
  TestWorldCross(core.GetWorld(), true);
  TestThirdParty(core.GetWorld());
  core.Release();
  Expect(!core.IsAlive(), "Private preview world released");
 }
 void TestWorldCross(BaseWorld world, bool xl)
 {
  ResourceName prefab = "{D67E9B88BBDB4FE4}Prefabs/E_DCO_PlaceableArsenal.et";
  if (xl) prefab = "{762A1A6379E24BD4}Prefabs/E_DCO_XLGearCross.et";
  IEntity rack = GetGame().SpawnEntityPrefabLocal(Resource.Load(prefab), world);
  Expect(rack != null, "Native world cross spawns: " + prefab);
  if (!rack) return;
  rack.SetOrigin("25 3 -19");
  rack.SetYawPitchRoll("63 7 -4");
  rack.SetScale(1.25);
  rack.Update();
  DCO_GearRackComponent component = DCO_GearRackComponent.Cast(rack.FindComponent(DCO_GearRackComponent));
  Expect(component != null, "Native world cross has rack component");
  array<ResourceName> gear = {
   "{4B57C11AA5161760}Prefabs/Characters/Vests/Vest_PASGT/Vest_PASGT.et",
   "{2835A0EA3B79E63E}Prefabs/Characters/Vests/Vest_ALICE/Variants/Vest_ALICE_rifleman.et",
   "{B74A4FF0DD8BB116}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01.et"
  };
  foreach (int index, ResourceName itemPrefab : gear)
  {
   IEntity source = GetGame().SpawnEntityPrefabLocal(Resource.Load(itemPrefab), world);
   DCO_EGearRackSlot kind = DCO_EGearRackSlot.VEST;
   if (index == 2) kind = DCO_EGearRackSlot.HELMET;
   IEntity display;
   if (source && component) display = component.BIA_TestCreateGearDisplay(source, kind);
   Expect(display != null, "World display created: " + itemPrefab);
   if (display)
   {
    vector mins, maxs;
    display.GetBounds(mins, maxs);
    vector support = Vector((mins[0] + maxs[0]) * 0.5, maxs[1], (mins[2] + maxs[2]) * 0.5);
    float height = 0.524;
    if (xl) height = 1.058;
    if (index < 2)
    {
     support = BIA_WeaponStage.TestVestContact(display);
     if (index == 0) Expect(support[1] > 1.525 && support[1] < 1.548, "PASGT contact excludes collar");
     else Expect(support[1] > 1.51 && support[1] < 1.53, "ALICE contact seats below strap crown");
     height = 0.524;
     if (xl) height = 1.058;
    }
    if (index == 2)
    {
     height = 0.7643;
     if (xl) height = 1.3612;
    }
    Expect(display.GetParent() == rack, "World display parent retains rack");
    Expect(vector.Distance(display.CoordToParent(support), rack.CoordToParent(Vector(0, height, 0))) < 0.001, "World support on tilted scaled rack: " + itemPrefab);
    Expect((display.GetFlags() & EntityFlags.VISIBLE) != 0, "World display visible without Arsenal");
    details.Insert("World cross XL=" + xl.ToString() + " item=" + itemPrefab + " support=" + rack.CoordToLocal(display.CoordToParent(support)).ToString());
    float lowest = LowestOnStand(display, rack);
    details.Insert("Complete display clearance=" + lowest.ToString());
    Expect(lowest >= 0, "Complete display clears cross base: " + itemPrefab);
   }
   BIA_PreviewDress.DeleteLocalHierarchy(display);
   BIA_PreviewDress.DeleteLocalHierarchy(source);
  }
  BIA_PreviewDress.DeleteLocalHierarchy(rack);
 }
 void TestThirdParty(BaseWorld world)
 {
  array<string> paths = {
   "{3281A5FE11A4A7E3}Belts/MTAC_Belt/Prefab/Mtac_RPS_MC.et",
   "{6E5A5D2864C4CD2B}Belts/TYR_Gunfighter_MAB/Prefabs/TYR_MAB.et",
   "{94BF8E53A6E09DF7}Belts/Virtus_H_rig/Prefab/Virtus_H.et",
   "{02D1092AE48F1585}Plate Carriers/CorsarMP3MP6/Prefabs/Corsar_MP3MP6.et",
   "{286D63A18FE67849}Plate Carriers/Crye_AVS/Prefabs/Crye_AVS.et",
   "{A0A27FF148640F4E}Plate Carriers/FCPC/Prefabs/FCPC.et",
   "{C439B6676CB88773}Plate Carriers/M4/Prefabs/M4_Balistyka_MC.et",
   "{7786B651DC95C440}Plate Carriers/Perun-6/Prefab/Perun_MC.et",
   "{00443A3C077B3CF6}Plate Carriers/Uwin/Prefabs/UWIN_PRO_MM14.et",
   "{4749112633532294}Plate Carriers/Virtus_Vest/Prefabs/Virtus_Vest.et",
   "{1232B533E2C657E2}Plate Carriers/WAS_DCS/Prefabs/WAS_DCS_mc.et",
   "{612264A367EFDB63}Helmet/AIRFRAME/Prefabs/Crye_Airframe_MC.et",
   "{B9610C2B62C83734}Helmet/FAST XP/Prefabs/Opscore_Fast_XP_MC.et",
   "{0FF9C8EB8F4AA4C1}Prefabs/Characters/Vests/JPC/CDD_EagleMCPR_MC.et",
   "{81F849FA00635981}Prefabs/Characters/Vests/JPC/CDD_MayflowerPusher.et",
   "{7D81D9A63706B674}Prefabs/Characters/Vests/JPC/Vest_AirliteDbl_MC.et",
   "{C6738206F17D9991}Prefabs/Characters/Vests/JPC/Vest_AirliteJTAC_MC.et",
   "{8EAED25A5DFA901E}Prefabs/Characters/Vests/JPC/Vest_AVS_MC.et",
   "{5E3DE83A1F533C44}Prefabs/Characters/Vests/JPC/Vest_JPCR_MC.et",
   "{69080C18CF21E394}Prefabs/Characters/Vests/JPC/Vest_MFGen4_MC.et",
   "{839617349049EBBB}Prefabs/Characters/Vests/JPC/Vest_MFHybrid_MC.et",
   "{E0B25FB8EFE64131}Prefabs/Characters/Vests/JPC/Vest_NJPC_MC.et",
   "{BA01150793757EA1}Prefabs/Characters/Vests/JPC/Vest_SPCSingle_MC.et",
   "{5ADBAE866F7D9A10}Prefabs/Characters/Vests/JPC/Vest_SPC_MC.et",
   "{7BD6C7BFB27170E1}Assets/Characters/Belts/Belt_Ronin/BisonBelt.et",
   "{38CD60392EA68DD7}Assets/Characters/Belts/Belt_Ronin/ShutoBelt.et",
   "{6ABC5B77A87C44F5}Assets/Characters/Belts/Belt_Ronin/TyrBelt.et",
   "{62D1153BED0475C4}Prefabs/Characters/HeadGear/Helmet_Caiman/Helmet_Caiman_BK.et",
   "{9FCD729D6470BB86}Prefabs/Characters/HeadGear/Helmet_OPSCORE/Helmet_OPSCORE_SF_MC_AMP_CAG.et",
   "{0B4A1793BB5DDF80}Prefabs/Characters/Vests/JPC/Vest_EBCR_MC.et",
   "{05739081B097AF7E}Prefabs/Characters/Vests/SOHPC/Vest_LV119_MC.et",
   "{264FFBC0C74209BD}Prefabs/Characters/Vests/SOHPC/Vest_SOHPC_MC.et",
   "{367BEEE3BE61C597}Prefabs/Characters/HeadGear/Headgear_TSh4/Helmet_tsh4.et",
   "{5F71AB03597B3D25}Prefabs/Characters/HeadGear/Helmet_6B47/Helmet_6B47.et",
   "{58A12E4317DFAA23}Prefabs/Characters/HeadGear/Helmet_6B7/Helmet_6B7.et",
   "{4298D95C60998247}Prefabs/Characters/HeadGear/Helmet_Caiman/Helmet_Caiman_Coy.et",
   "{1608873B6690044C}Prefabs/Characters/HeadGear/Helmet_CVC_01/Helmet_CVC_01.et",
   "{33915036D0448C92}Prefabs/Characters/HeadGear/Helmet_ECH/Helmet_ECH_hc_v_r.et",
   "{54A7E4E38371553E}Prefabs/Characters/HeadGear/Helmet_Exfil/Helmet_Exfil_Blk.et",
   "{D6C2E291F59E8985}Prefabs/Characters/HeadGear/Helmet_Kiver_RSP/Helmet_Kiver_RSP.et",
   "{1E21D7A6F75BAFA6}Prefabs/Characters/HeadGear/Helmet_LShZ/Helmet_LShZ_atacs.et",
   "{FA2B69EE738B0D43}Prefabs/Characters/HeadGear/Helmet_LWH/Helmet_LWH_aor1.et",
   "{B769265F1F723E12}Prefabs/Characters/HeadGear/Helmet_MICH_ACH/Helmet_ACH_Cov_DCU.et",
   "{73ADF745BCECBF43}Prefabs/Characters/HeadGear/Helmet_OPSCORE/Helmet_OPSCORE_LBH_AOR1.et",
   "{FE5C49069C2499D9}Prefabs/Characters/HeadGear/Helmet_PASGT_01/Helmet_PASGT_01_cover.et",
   "{10E223B05E8874EA}Prefabs/Characters/HeadGear/Helmet_Spartan3/Helmet_Spartan3_FG.et",
   "{4C988BFF658AA819}Prefabs/Characters/HeadGear/Helmet_TBH3A/Helmet_TBH3A_HC.et",
   "{3F70EA42E26BA189}Prefabs/Characters/HeadGear/Helmet_TOR/Helmet_TOR_Large.et",
   "{364FF1A1CA2CA32E}Prefabs/Characters/HeadGear/Helmet_TOR2/Helmet_TOR2L_blk.et",
   "{034EC83B216F6F00}Prefabs/Characters/HeadGear/Helmet_Triada/Helmet_Triada_EmrM.et",
   "{BFE3D93AA5B13A89}Prefabs/Characters/HeadGear/Helmet_ZSh7/Helmet_ZSh7V.et",
   "{414F1A6A72B27A6E}Prefabs/Characters/Vests/Vest_6B23/Vest_6B23_6SH117.et",
   "{3E96F1007C62980B}Prefabs/Characters/Vests/Vest_6SH117/Vest_6SH117.et",
   "{275BE1D304BC2E2D}Prefabs/Characters/Vests/Vest_AACPC/Vest_AACPC_FG.et",
   "{AA28144C1153A86B}Prefabs/Characters/Vests/Vest_AA_A18/Vest_AA_A18_Coy.et",
   "{0AE05534501F694E}Prefabs/Characters/Vests/Vest_AVS/Vest_AVS_6x9_Blk.et",
   "{094B3C53D6EAA522}Prefabs/Characters/Vests/Vest_CPC_NCPC/Vest_NCPC_AOR1.et",
   "{C4FEAEB4AAA7B6B2}Prefabs/Characters/Vests/Vest_JPC/Vest_JPC_Coy.et",
   "{16876720DFA7021C}Prefabs/Characters/Vests/Vest_KBS_Strelok/Vest_KBS_Strelok.et",
   "{23715D37E03B24B5}Prefabs/Characters/Vests/Vest_LBT1961/Vest_LBT1961.et",
   "{1300A5F6CA51B206}Prefabs/Characters/Vests/Vest_PCGen_III/Vest_PCGen_III.et",
   "{099AE885BB314867}Prefabs/Characters/Vests/Vest_Ratnik3K_6B48/Vest_Ratnik_6B48.et",
   "{B9821AF2F5D104E4}Prefabs/Characters/Vests/Vest_Ratnik_6B45/Vest_Ratnik_6B45.et",
   "{69975371BB261B11}Prefabs/Characters/Vests/Vest_Ratnik_6B46/Vest_6B46_EMR.et",
   "{66177AF251E66DA5}Prefabs/Characters/Vests/Vest_Shaw_ARC/Vest_Shaw_ARC_Coy.et",
   "{A20DC15BF24DBE17}Prefabs/Characters/Vests/Vest_Taktika/Vest_Taktika.et",
   "{3A1F86DC026EF6AD}Prefabs/Characters/Vests/Vest_TT_MK2/Vest_TT_MK2.et",
   "{FAECE2A4CCD7FB96}Prefabs/Characters/Vests/Vest_TV102/Vest_TV102_EMR.et",
   "{1434EE7D082F04D4}Prefabs/Characters/Vests/Vest_TV110/Vest_TV110_FG.et",
   "{AFA62768BA709071}Prefabs/Characters/Vests/Vest_TV115/Vest_TV115_Coy.et",
   "{56F032CB6D743D49}Prefabs/Characters/Vests/Vest_TV119/Vest_TV119_Coy.et",
   "{35958C7FA91365F1}Prefabs/Characters/Vests/Vest_TV150/Vest_TV150_MC.et"
  };
  IEntity small = GetGame().SpawnEntityPrefabLocal(Resource.Load("{D67E9B88BBDB4FE4}Prefabs/E_DCO_PlaceableArsenal.et"), world);
  IEntity xl = GetGame().SpawnEntityPrefabLocal(Resource.Load("{762A1A6379E24BD4}Prefabs/E_DCO_XLGearCross.et"), world);
  small.SetOrigin("25 3 -19");
  small.SetYawPitchRoll("63 7 -4");
  small.SetScale(1.25);
  small.Update();
  xl.SetOrigin("28 4 -19");
  xl.SetYawPitchRoll("-37 5 8");
  xl.SetScale(1.25);
  xl.Update();
  foreach (string path : paths)
  {
   Resource resource = Resource.Load(path);
   if (!resource || !resource.IsValid())
   {
    details.Insert("Optional addon unavailable: " + path);
    continue;
   }
   IEntity source = GetGame().SpawnEntityPrefabLocal(resource, world);
   if (!source) { failures.Insert("Third-party source: " + path); continue; }
   DCO_EGearRackSlot kind = DCO_GearRackComponent.GearKind(source);
   if (kind == DCO_EGearRackSlot.NONE)
   {
    details.Insert("Not worn rack gear: " + resource.GetResource().GetResourceName());
    BIA_PreviewDress.DeleteLocalHierarchy(source);
    continue;
   }
   InventoryItemComponent inventory = InventoryItemComponent.Cast(source.FindComponent(InventoryItemComponent));
   IEntity display;
   if (inventory) display = inventory.CreatePreviewEntity(world, 0);
   Expect(display && display.GetVObject(), "Third-party worn display: " + path);
   if (display)
   {
    vector mins, maxs;
    display.GetBounds(mins, maxs);
    vector contact;
    if (kind == DCO_EGearRackSlot.VEST)
    {
     contact = BIA_WeaponStage.TestVestContact(display);
     Expect(contact[1] <= maxs[1] + 0.002, "Auxiliary mesh cannot lift a complete carrier's shoulder anchor: " + path);
    }
    details.Insert("Third-party=" + resource.GetResource().GetResourceName() + " kind=" + kind.ToString() + " mesh=" + display.GetVObject().GetResourceName() + " bounds=" + mins.ToString() + "/" + maxs.ToString() + " contact=" + contact.ToString());
    for (int size = 0; size < 2; size++)
    {
     if (size == 0 && kind == DCO_EGearRackSlot.BELT) continue;
     IEntity rack = small;
     if (size == 1) rack = xl;
     BIA_WeaponStage.PositionGearOnStand(display, rack, kind, rack.GetScale(), rack.GetYawPitchRoll(), size == 1);
     vector span = BIA_WeaponStage.TestShoulderSpan(display);
     if (span.LengthSq() > 0)
     {
      span = rack.VectorToLocal(display.VectorToParent(span));
      if (kind == DCO_EGearRackSlot.HELMET && size == 0) span = -span;
      Expect(span[0] > 0, "Worn-model orientation matches the rack: " + path);
     }
     float lowest = LowestOnStand(display, rack);
     Expect(lowest >= -0.002, "Third-party base clearance size=" + size.ToString() + ": " + path);
     if (kind == DCO_EGearRackSlot.VEST)
     {
      float height = 0.524;
      if (size == 1) height = 1.058;
      Expect(vector.Distance(rack.CoordToLocal(display.CoordToParent(contact)), Vector(0, height, 0)) < 0.002, "Third-party shoulder contact size=" + size.ToString() + ": " + path);
     }
    }
   }
   BIA_PreviewDress.DeleteLocalHierarchy(display);
   BIA_PreviewDress.DeleteLocalHierarchy(source);
  }
  BIA_PreviewDress.DeleteLocalHierarchy(small);
  BIA_PreviewDress.DeleteLocalHierarchy(xl);
 }
 float LowestOnStand(IEntity entity, IEntity stand)
 {
  vector mins, maxs;
  entity.GetBounds(mins, maxs);
  float lowest = 1e10;
  bool volume = maxs[0] - mins[0] > 0.0001 && maxs[1] - mins[1] > 0.0001 && maxs[2] - mins[2] > 0.0001;
  for (int corner = 0; volume && corner < 8; corner++)
  {
   vector point = mins;
   for (int axis = 0; axis < 3; axis++)
    if (corner & (1 << axis)) point[axis] = maxs[axis];
   point = stand.CoordToLocal(entity.CoordToParent(point));
   lowest = Math.Min(lowest, point[1]);
  }
  IEntity child = entity.GetChildren();
  while (child)
  {
   lowest = Math.Min(lowest, LowestOnStand(child, stand));
   child = child.GetSibling();
  }
  return lowest;
 }
}

modded class DCO_GearRackComponent
{
 IEntity BIA_TestCreateGearDisplay(IEntity source, DCO_EGearRackSlot kind)
 {
  return CreateDisplay(source, kind);
 }
}

modded class BIA_WeaponStage
{
 static vector TestVestContact(IEntity item) { return VestSupportPoint(item); }
 static vector TestShoulderSpan(IEntity item)
 {
  vector left, right;
  if (!ShoulderFrame(item, left, right)) return vector.Zero;
  return right - left;
 }

 bool Facing(IEntity item, IEntity stand, bool reverse)
 {
  vector i[4], s[4];
  item.GetTransform(i); stand.GetTransform(s);
  i[1] = i[1].Normalized(); s[1] = s[1].Normalized(); i[2] = i[2].Normalized(); s[2] = s[2].Normalized();
  if (reverse) s[2] = -s[2];
  return vector.Distance(i[1], s[1]) < 0.001 && vector.Distance(i[2], s[2]) < 0.001;
 }

 void TestStandPlacement(BIA_StandPlacementProbe probe, IEntity stand, ResourceName prefab, bool helmet, bool alice = false)
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
  if (alice)
  {
   // Native ALICE suspenders and waist extents, measured independently of the pose helper.
   mins = "-0.16113 1.08351 -0.221214";
   maxs = "0.164532 1.54934 0.107938";
  }
  vector top = Vector((mins[0] + maxs[0]) * 0.5, maxs[1], (mins[2] + maxs[2]) * 0.5);
  float height = 0.524;
  if (!helmet) top = VestSupportPoint(m_Weapon);
  if (helmet) height = 0.7643;
  probe.Expect(vector.Distance(m_Weapon.CoordToParent(top), stand.CoordToParent(Vector(0, height, 0))) < 0.001, "Support aligns: " + prefab);
  float expectedScale = m_fItemHomeScale;
  float bottomY = mins[1];
  if (alice) bottomY = 0.812281;
  if (!helmet)
   expectedScale = Math.Min(m_fItemHomeScale, Math.Max(height * stand.GetScale() - 0.01, 0.001) / (top[1] - bottomY));
  probe.Expect(Math.AbsFloat(m_Weapon.GetScale() - expectedScale) < 0.001, "Native item scale and world clearance retained: " + prefab);
  probe.Expect(vector.Distance(stand.GetOrigin(), home[3]) < 0.001, "Stand remains at authored position");
  probe.Expect(Facing(m_Weapon, stand, helmet), "Item follows the requested stand-facing direction");
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
    probe.Expect(Facing(m_CompanionHelmet, stand, true), "Companion helmet has corrected front direction");
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
  probe.Expect(Facing(m_Weapon, stand, helmet), "Focus preserves the supported item basis");
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
   probe.Expect(vector.Distance(m_Weapon.CoordToParent(top), stand.CoordToParent(Vector(0, height, 0))) < 0.001, "Mounted child cannot shift shoulder support");
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
