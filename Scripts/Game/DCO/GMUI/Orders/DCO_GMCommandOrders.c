// Imports the native GM command-bar commands into our Orders box so we can peel the engine command bar.
class DCO_GMCommandOrders
{
	static const int CAT_WAYPOINTS  = 310;
	static const int CAT_OBJECTIVES = 311;
	static const int CAT_SPAWN      = 312;
	static const int CMD_ID_BASE    = 1000;	// Native command leaf id = CMD_ID_BASE + index into m_Commands.
	static const int EXTRA_ID_BASE  = 2000;	// Installed native waypoint prefabs omitted from the stock command list.

	protected ref array<SCR_BaseEditorAction> m_Commands = {};
	protected ref array<ResourceName> m_ExtraPrefabs = {};
	protected ref array<string> m_ExtraLabels = {};
	protected bool m_bBuilt;
	protected bool m_bExtrasBuilt;

	static bool IsCommandCategory(int id)
	{
		return id == CAT_WAYPOINTS || id == CAT_OBJECTIVES || id == CAT_SPAWN;
	}

	static bool IsCommandLeaf(int id)
	{
		return id >= CMD_ID_BASE && id < EXTRA_ID_BASE + 100;
	}

	protected void AddExtra(string label, ResourceName prefab)
	{
		Resource resource;
		if (!prefab.IsEmpty())
			resource = Resource.Load(prefab);
		if (!resource)
		{
			Print(string.Format("[DCO-GM] native waypoint unavailable; hidden from Orders: %1", prefab), LogLevel.WARNING);
			return;
		}
		m_ExtraLabels.Insert(label);
		m_ExtraPrefabs.Insert(prefab);
	}

	protected void EnsureExtrasBuilt()
	{
		if (m_bExtrasBuilt)
			return;
		m_bExtrasBuilt = true;
		m_ExtraLabels.Clear();
		m_ExtraPrefabs.Clear();
		AddExtra("Scout area", "PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Scout.et");
		AddExtra("Wait", "PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Wait.et");
		AddExtra("Load supplies", "PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_LoadSupplies.et");
		AddExtra("Unload supplies", "PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_UnloadSupplies.et");
	}

	protected void EnsureBuilt()
	{
		if (m_bBuilt)
			return;
		SCR_CommandActionsEditorComponent comp = SCR_CommandActionsEditorComponent.Cast(SCR_CommandActionsEditorComponent.GetInstance(SCR_CommandActionsEditorComponent, false, true));
		if (!comp)
			return;	// not in an editor yet; try again next open.
		m_Commands.Clear();
		array<SCR_BaseEditorAction> actions = {};
		comp.GetActions(actions);
		foreach (SCR_BaseEditorAction a : actions)
		{
			if (SCR_BaseCommandAction.Cast(a))
				m_Commands.Insert(a);
		}
		m_bBuilt = true;
	}

	protected int CategoryOf(SCR_BaseCommandAction cmd)
	{
		if (SCR_WaypointBaseCommandAction.Cast(cmd))
			return CAT_WAYPOINTS;
		if (SCR_TaskBaseCommandAction.Cast(cmd))
			return CAT_OBJECTIVES;
		return CAT_SPAWN;
	}

	protected string DisplayNameOf(SCR_BaseCommandAction cmd)
	{
		SCR_UIInfo info = cmd.GetInfo();
		LocalizedString authoredName;
		if (info)
			authoredName = info.GetName();

		string label = DCO_GMDisplayName.Resolve(authoredName, cmd.GetCommandPrefab(), "Command");
		string waypointPrefix = "E AIWaypoint ";
		if (label.IndexOf(waypointPrefix) == 0)
			label = label.Substring(waypointPrefix.Length(), label.Length() - waypointPrefix.Length());
		if (label == "Artillery Support")
			return "Artillery fire";
		return label;
	}

	void BuildCategoryOptions(int catId, out notnull array<string> labels, out notnull array<int> ids)
	{
		labels.Clear();
		ids.Clear();
		EnsureBuilt();
		EnsureExtrasBuilt();
		for (int i = 0; i < m_Commands.Count(); i++)
		{
			SCR_BaseCommandAction cmd = SCR_BaseCommandAction.Cast(m_Commands[i]);
			if (!cmd)
				continue;
			if (CategoryOf(cmd) != catId)
				continue;
			labels.Insert(DisplayNameOf(cmd));
			ids.Insert(CMD_ID_BASE + i);
		}
		if (catId == CAT_WAYPOINTS)
		{
			for (int extraIndex = 0; extraIndex < m_ExtraPrefabs.Count(); extraIndex++)
			{
				labels.Insert(m_ExtraLabels[extraIndex]);
				ids.Insert(EXTRA_ID_BASE + extraIndex);
			}
		}
	}

	void Trigger(int leafId, SCR_EditableEntityComponent group, array<SCR_EditableEntityComponent> allGroups = null)
	{
		EnsureBuilt();
		EnsureExtrasBuilt();
		if (leafId >= EXTRA_ID_BASE)
		{
			TriggerExtra(leafId - EXTRA_ID_BASE, group, allGroups);
			return;
		}
		int idx = leafId - CMD_ID_BASE;
		if (idx < 0 || idx >= m_Commands.Count())
			return;
		SCR_BaseCommandAction cmd = SCR_BaseCommandAction.Cast(m_Commands[idx]);
		if (!cmd)
			return;
		SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (!placing)
			return;
		set<SCR_EditableEntityComponent> recipients = new set<SCR_EditableEntityComponent>();
		if (allGroups)
		{
			foreach (SCR_EditableEntityComponent g : allGroups)
			{
				if (g)
					recipients.Insert(g);
			}
		}
		if (recipients.IsEmpty() && group)
			recipients.Insert(group);
		bool accepted = placing.SetSelectedPrefab(cmd.GetCommandPrefab(), false, true, recipients, cmd);
		if (!accepted || !placing.IsPlacing())
		{
			Print(string.Format("[DCO-GM] command refused: prefab is unavailable or unregistered: %1", cmd.GetCommandPrefab()), LogLevel.WARNING);
			return;
		}
	}

	protected void TriggerExtra(int index, SCR_EditableEntityComponent group, array<SCR_EditableEntityComponent> allGroups)
	{
		if (index < 0 || index >= m_ExtraPrefabs.Count())
			return;
		SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent, false, true));
		if (!placing)
			return;

		set<SCR_EditableEntityComponent> recipients = new set<SCR_EditableEntityComponent>();
		if (allGroups)
		{
			foreach (SCR_EditableEntityComponent selectedGroup : allGroups)
			{
				if (selectedGroup)
					recipients.Insert(selectedGroup);
			}
		}
		if (recipients.IsEmpty() && group)
			recipients.Insert(group);
		if (recipients.IsEmpty())
			return;

		ResourceName prefab = m_ExtraPrefabs[index];
		bool accepted = placing.SetSelectedPrefab(prefab, false, true, recipients, null);
		if (!accepted || !placing.IsPlacing())
		{
			Print(string.Format("[DCO-GM] specialist waypoint refused by native placement: %1", prefab), LogLevel.WARNING);
			return;
		}
	}
}
