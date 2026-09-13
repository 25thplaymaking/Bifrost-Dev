class DCO_RackTransferFaultCase : DCO_GearRackTransfer
{
	string trace;
	int failAt = -1;
	bool failRollback;
	bool Finished() { return m_bFinished; }
	void Begin()
	{
		m_Moves.Insert(new DCO_GearRackMove());
		m_Moves.Insert(new DCO_GearRackMove());
		MoveNext();
	}
	override void MoveNext()
	{
		if (m_bFinished || m_iNext < 0 || m_iNext >= m_Moves.Count())
		{
			super.MoveNext();
			return;
		}
		if (m_bRollback) trace += "R";
		else trace += "M";
		trace += m_iNext.ToString();
		OnMoveDone((m_bRollback && !failRollback) || (!m_bRollback && m_iNext != failAt));
	}
}

[WorkbenchPluginAttribute(name: "Gear cross feature regression", wbModules: {"ResourceManager"})]
class DCO_GearRackFeaturePlugin : WorkbenchPlugin
{
	protected int m_Passed;
	protected int m_Failed;
	protected void Expect(bool value, string label)
	{
		if (value) m_Passed++;
		else { m_Failed++; Print("[RACK-TEST] FAIL " + label, LogLevel.ERROR); }
	}

	protected void CheckPersistence()
	{
		Resource resource = BaseContainerTools.LoadContainer("{3A03B52D11F7C36A}Configs/Systems/Persistence/Common.conf");
		Expect(resource && resource.IsValid(), "Native persistence configuration loads");
		if (!resource || !resource.IsValid()) return;
		BaseContainer source = resource.GetResource().ToBaseContainer();
		BaseContainerList collections = source.GetObjectArray("Collections");
		Expect(collections && collections.Count() >= 10, "Rack extension preserves native persistence collections");
		BaseContainerList groups = source.GetObjectArray("Configurations");
		BaseContainer rack;
		if (groups)
		{
			for (int i = 0; i < groups.Count(); i++)
			{
				BaseContainerList entries = groups.Get(i).GetObjectArray("Configurations");
				if (!entries) continue;
				for (int j = 0; j < entries.Count(); j++)
				{
					BaseContainer entry = entries.Get(j);
					if (entry.GetClassName() != "EntityPersistenceConfig") continue;
					BaseContainer rule = entry.GetObject("Rule");
					string component;
					if (rule && rule.Get("ComponentClass", component) && component == "DCO_GearRackComponent") rack = entry;
				}
			}
		}
		Expect(rack != null, "Rack has its own resolved native persistence rule");
		if (!rack) return;
		int priority;
		bool selfSpawn, selfDelete, storageRoot;
		Expect(rack.Get("Priority", priority) && priority > 33000, "Rack rule wins over generic storage and clothing rules");
		Expect(rack.Get("SelfSpawn", selfSpawn) && selfSpawn, "Enabled racks restore with the mission");
		Expect(rack.Get("SelfDelete", selfDelete) && selfDelete, "Deleted rack records are removed");
		Expect(rack.Get("StorageRoot", storageRoot) && storageRoot, "Rack owns its saved inventory hierarchy");
		BaseContainerList serializers = rack.GetObjectArray("ComponentSerializers");
		bool hasSettings, hasInventory, hasEditor;
		if (serializers)
		{
			for (int s = 0; s < serializers.Count(); s++)
			{
				string type = serializers.Get(s).GetClassName();
				if (type == "DCO_GearRackSerializer") hasSettings = true;
				if (type == "BaseInventoryStorageComponentSerializer") hasInventory = true;
				if (type == "SCR_EditableEntityComponentSerializer") hasEditor = true;
			}
		}
		Expect(hasSettings && hasInventory && hasEditor, "Settings, native inventory and GM editability are serialized together");
	}

	protected void CheckGeometry()
	{
		BIA_StageCore core = new BIA_StageCore();
		Expect(core.EnsureWorld("BifrostRackFeatureTest"), "Isolated geometry world creates");
		if (!core.GetWorld()) return;
		IEntity stand = GetGame().SpawnEntityPrefabLocal(Resource.Load("{762A1A6379E24BD4}Prefabs/E_DCO_XLGearCross.et"), core.GetWorld());
		IEntity rifle = GetGame().SpawnEntityPrefabLocal(Resource.Load("{3E413771E1834D2F}Prefabs/Weapons/Rifles/M16/Rifle_M16A2.et"), core.GetWorld());
		Expect(stand && rifle, "XL cross and native primary rifle spawn");
		if (stand && rifle)
		{
			CheckSettings(stand);
			Expect(DCO_GearRackComponent.GearKind(rifle) == DCO_EGearRackSlot.PRIMARY, "Primary rifle uses the weapon slot classification");
			stand.SetOrigin("25 3 -19");
			stand.SetYawPitchRoll("63 7 -4");
			stand.SetScale(1.25);
			BIA_WeaponStage.PositionGearOnStand(rifle, stand, DCO_EGearRackSlot.PRIMARY, stand.GetScale(), stand.GetYawPitchRoll(), true);
			vector minimum, maximum, standMin, standMax;
			rifle.GetBounds(minimum, maximum);
			stand.GetBounds(standMin, standMax);
			float lowest = 1e10;
			for (int corner = 0; corner < 8; corner++)
			{
				vector point = minimum;
				if (corner & 1) point[0] = maximum[0];
				if (corner & 2) point[1] = maximum[1];
				if (corner & 4) point[2] = maximum[2];
				vector localPoint = stand.CoordToLocal(rifle.CoordToParent(point));
				lowest = Math.Min(lowest, localPoint[1]);
			}
			Expect(Math.AbsFloat(lowest - standMin[1] - 0.02) < 0.002, "Rifle butt rests on the transformed cross base");
			vector origin = stand.CoordToLocal(rifle.GetOrigin());
			vector barrel = stand.CoordToLocal(rifle.CoordToParent("0 0 1")) - origin;
			Expect(barrel[1] > 0.9 && barrel[0] < -0.1, "Rifle barrel points upward and leans toward the upright");
		}
		BIA_PreviewDress.DeleteLocalHierarchy(rifle);
		BIA_PreviewDress.DeleteLocalHierarchy(stand);
		core.Release();
		Expect(!core.IsAlive(), "Geometry world releases");
	}

	protected void CheckSettings(IEntity stand)
	{
		DCO_GearRackComponent rack = DCO_GearRackComponent.Cast(stand.FindComponent(DCO_GearRackComponent));
		Expect(rack != null, "Spawned rack exposes settings component");
		if (!rack) return;
		string result;
		Expect(rack.ConfigureRack(" Alpha Cross ", " Warlord ", false, result), "Rack accepts valid settings without requiring mission saving");
		Expect(rack.GetRackLabel() == "Alpha Cross (Warlord)", "World action includes cross and operator names");
		Expect(!rack.ConfigureRack("Changed", "This operator name deliberately exceeds the maximum allowed sixty four characters", false, result)
			&& rack.GetRackName() == "Alpha Cross", "Invalid operator name does not partly overwrite rack settings");
		Expect(!rack.ConfigureRack("Changed", "Other", true, result) && !rack.IsPersistent()
			&& rack.GetRackName() == "Alpha Cross", "Unavailable persistence fails without changing settings");
		rack.RestoreRackSettings("", "", false);
	}

	protected void CheckTransfers()
	{
		DCO_RackTransferFaultCase success = new DCO_RackTransferFaultCase();
		success.Begin();
		Expect(success.Finished() && success.trace == "M0M1", "Paired transfer completes both native steps once");
		DCO_RackTransferFaultCase firstFailure = new DCO_RackTransferFaultCase();
		firstFailure.failAt = 0;
		firstFailure.Begin();
		Expect(firstFailure.Finished() && firstFailure.trace == "M0", "Rejected first move leaves second item untouched");
		DCO_RackTransferFaultCase secondFailure = new DCO_RackTransferFaultCase();
		secondFailure.failAt = 1;
		secondFailure.Begin();
		Expect(secondFailure.Finished() && secondFailure.trace == "M0M1R0", "Rejected second move restores the completed first move");
		secondFailure.OnMoveDone(true);
		Expect(secondFailure.trace == "M0M1R0", "Late completion cannot restart a finished transfer");
		DCO_RackTransferFaultCase rollbackFailure = new DCO_RackTransferFaultCase();
		rollbackFailure.failAt = 1;
		rollbackFailure.failRollback = true;
		rollbackFailure.Begin();
		Expect(rollbackFailure.Finished() && rollbackFailure.trace == "M0M1R0", "Rejected rollback terminates without retrying or creating replacement items");
	}

	override void RunCommandline()
	{
		string project;
		Expect(Workbench.GetAbsolutePath("$BifrostDev:addon.gproj", project)
			&& project == "C:/Users/Bryce/Documents/My Games/ArmaReforgerWorkbench/addons/Bifrost-Dev/addon.gproj", "Authoritative Bifrost workspace is loaded");
		Print("[RACK-TEST] PROJECT " + project);
		DCO_GearRackReplicationRegression replication = new DCO_GearRackReplicationRegression();
		DCO_GearRackReplicationResponse baseline = DCO_GearRackReplicationResponse.Cast(replication.GetResponse(null));
		m_Passed += baseline.passed;
		foreach (string failure : baseline.failures) Expect(false, failure);
		CheckPersistence();
		CheckTransfers();
		CheckGeometry();
		Print(string.Format("[RACK-TEST] RESULT passed=%1 failed=%2", m_Passed, m_Failed));
		int code;
		if (m_Failed) code = 1;
		Workbench.Exit(code);
	}
}
