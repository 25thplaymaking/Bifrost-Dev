class DCO_GMCompositionItem
{
	ResourceName m_Prefab;
	vector m_vOffset;
	vector m_vAngles;
	float m_fScale = 1.0;
	float m_fGroundOffset;
	int m_iParentIndex = -1;
	int m_iRootIndex = -1;
}

class DCO_GMCompositionRecord
{
	int m_iId;
	int m_iOwnerPlayerId;
	string m_sName;
	string m_sCategory;
	string m_sAuthor;
	ref array<ref DCO_GMCompositionItem> m_aItems = {};
}

class DCO_GMCompositionStore
{
	int m_iVersion = 1;
	ref array<ref DCO_GMCompositionRecord> m_aCompositions = {};
}

class DCO_GMCompositionCatalogEntry
{
	int m_iId;
	int m_iItemCount;
	string m_sName;
	string m_sCategory;
	string m_sAuthor;
}

class DCO_GMCompositionCaptureSlot
{
	int m_iIndex;
	RplId m_EntityId;
}

class DCO_GMCompositionCaptureStage
{
	int m_iPlayerId;
	int m_iToken;
	int m_iExpectedCount;
	bool m_bInvalid;
	string m_sName;
	string m_sCategory;
	string m_sAuthor;
	ref array<ref DCO_GMCompositionCaptureSlot> m_aSlots = {};
}

class DCO_GMCompositionLastPlacement
{
	int m_iPlayerId;
	ref array<IEntity> m_aEntities = {};
}

class DCO_GMCompositionService
{
	protected static ref DCO_GMCompositionService s_Instance;
	protected ref array<ref DCO_GMCompositionCatalogEntry> m_aEntries = {};
	protected ref array<ref DCO_GMCompositionCatalogEntry> m_aPendingEntries = {};
	protected ref ScriptInvoker m_OnChanged = new ScriptInvoker();
	protected ref ScriptInvoker m_OnResult = new ScriptInvoker();
	protected BaseWorld m_World;
	protected int m_iSnapshotSerial = -1;
	protected int m_iCaptureToken;

	static DCO_GMCompositionService Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMCompositionService();
		return s_Instance;
	}

	ScriptInvoker GetOnChanged()
	{
		return m_OnChanged;
	}

	ScriptInvoker GetOnResult()
	{
		return m_OnResult;
	}

	void Initialize()
	{
		BaseWorld world = GetGame().GetWorld();
		if (m_World != world)
		{
			m_World = world;
			m_aEntries.Clear();
			m_aPendingEntries.Clear();
			m_iSnapshotSerial = -1;
			m_iCaptureToken = 0;
		}
		RequestSnapshot();
	}

	void GetEntries(out notnull array<DCO_GMCompositionCatalogEntry> entries)
	{
		entries.Clear();
		foreach (DCO_GMCompositionCatalogEntry entry : m_aEntries)
		{
			if (entry)
				entries.Insert(entry);
		}
	}

	DCO_GMCompositionCatalogEntry Find(int id)
	{
		foreach (DCO_GMCompositionCatalogEntry entry : m_aEntries)
		{
			if (entry && entry.m_iId == id)
				return entry;
		}
		return null;
	}

	void CaptureSelected(string name, string category, string author)
	{
		name = NormalizeName(name);
		category = NormalizeMetadata(category, "Custom");
		author = NormalizeMetadata(author, "Game Master");
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		if (selected.IsEmpty())
		{
			OnResult(false, "Capture requires at least one selected editable entity.");
			return;
		}
		if (selected.Count() > DCO_GMCompositionServer.MAX_ITEMS)
		{
			OnResult(false, string.Format("Capture supports up to %1 selected entities.", DCO_GMCompositionServer.MAX_ITEMS));
			return;
		}

		array<RplId> entityIds = {};
		foreach (SCR_EditableEntityComponent editable : selected)
		{
			RplId entityId;
			if (!editable || !editable.GetOwner() || !editable.IsReplicated(entityId) || !entityId.IsValid())
			{
				OnResult(false, "Capture refused because the selection contains a non-replicated editable entity.");
				return;
			}
			entityIds.Insert(entityId);
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
		{
			OnResult(false, "Capture failed because the local player controller is unavailable.");
			return;
		}
		m_iCaptureToken++;
		if (m_iCaptureToken <= 0)
			m_iCaptureToken = 1;
		OnResult(true, string.Format("Capturing %1 selected entities on the server...", entityIds.Count()));
		controller.DCO_BeginGMCompositionCapture(m_iCaptureToken, name, category, author, entityIds.Count());
		for (int i = 0; i < entityIds.Count(); i++)
			controller.DCO_AddGMCompositionCaptureItem(m_iCaptureToken, i, entityIds[i]);
		controller.DCO_CommitGMCompositionCapture(m_iCaptureToken);
	}

	void Place(int compositionId, vector position)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
		{
			OnResult(false, "Placement failed because the local player controller is unavailable.");
			return;
		}
		OnResult(true, "Validating and placing the composition on the server...");
		controller.DCO_RequestGMCompositionPlace(compositionId, position);
	}

	void DeleteLibraryEntry(int compositionId)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
		{
			OnResult(false, "Delete failed because the local player controller is unavailable.");
			return;
		}
		controller.DCO_RequestGMCompositionDelete(compositionId);
	}

	void UndoLastPlacement()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!controller)
		{
			OnResult(false, "Undo failed because the local player controller is unavailable.");
			return;
		}
		controller.DCO_RequestGMCompositionUndo();
	}

	void RequestSnapshot()
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.DCO_RequestGMCompositionSnapshot();
	}

	void OnSnapshotBegin(int serial)
	{
		if (serial < m_iSnapshotSerial)
			return;
		m_iSnapshotSerial = serial;
		m_aPendingEntries.Clear();
	}

	void OnSnapshotRecord(int serial, int id, string name, string category, string author, int itemCount)
	{
		if (serial != m_iSnapshotSerial)
			return;
		DCO_GMCompositionCatalogEntry entry = new DCO_GMCompositionCatalogEntry();
		entry.m_iId = id;
		entry.m_sName = NormalizeName(name);
		entry.m_sCategory = NormalizeMetadata(category, "Custom");
		entry.m_sAuthor = NormalizeMetadata(author, "Game Master");
		entry.m_iItemCount = Math.Max(0, itemCount);
		m_aPendingEntries.Insert(entry);
	}

	void OnSnapshotEnd(int serial)
	{
		if (serial != m_iSnapshotSerial)
			return;
		m_aEntries.Clear();
		foreach (DCO_GMCompositionCatalogEntry entry : m_aPendingEntries)
			m_aEntries.Insert(entry);
		m_aPendingEntries.Clear();
		m_OnChanged.Invoke();
	}

	void OnResult(bool success, string message)
	{
		m_OnResult.Invoke(success, message);
	}

	static string NormalizeName(string name)
	{
		name.TrimInPlace();
		name.Replace("\n", " ");
		name.Replace("\r", " ");
		name.Replace("\t", " ");
		if (name.IsEmpty())
			name = "Untitled Composition";
		if (name.Length() > 48)
			name = name.Substring(0, 48);
		return name;
	}

	static string NormalizeMetadata(string value, string fallback)
	{
		value.TrimInPlace();
		value.Replace("\n", " ");
		value.Replace("\r", " ");
		value.Replace("\t", " ");
		if (value.IsEmpty())
			value = fallback;
		if (value.Length() > 32)
			value = value.Substring(0, 32);
		return value;
	}
}

class DCO_GMCompositionServer
{
	static const int MAX_ITEMS = 64;
	static const int MAX_LIBRARY = 64;
	protected static const int STORE_VERSION = 1;
	protected static const string STORE_FILE = "$profile:BifrostGM_compositions.json";
	protected static ref DCO_GMCompositionStore s_Store;
	protected static ref array<ref DCO_GMCompositionCaptureStage> s_aStages;
	protected static ref array<ref DCO_GMCompositionLastPlacement> s_aLastPlacements;
	protected static BaseWorld s_World;
	protected static bool s_bStoreLoaded;
	protected static int s_iNextId = 1;
	protected static int s_iSnapshotSerial;

	protected static void EnsureWorld()
	{
		BaseWorld world = GetGame().GetWorld();
		if (s_World == world)
			return;
		s_World = world;
		s_aStages = {};
		s_aLastPlacements = {};
		s_iSnapshotSerial = 0;
		EnsureStoreLoaded();
	}

	protected static array<ref DCO_GMCompositionRecord> Library()
	{
		EnsureWorld();
		EnsureStoreLoaded();
		return s_Store.m_aCompositions;
	}

	protected static void EnsureStoreLoaded()
	{
		if (s_bStoreLoaded)
			return;
		s_bStoreLoaded = true;
		s_Store = new DCO_GMCompositionStore();
		if (!FileIO.FileExists(STORE_FILE))
			return;

		JsonLoadContext context = new JsonLoadContext();
		DCO_GMCompositionStore loaded = new DCO_GMCompositionStore();
		if (!context.LoadFromFile(STORE_FILE) || !context.ReadValue("", loaded) || !ValidateStore(loaded))
		{
			Print("[DCO-GM] composition library is unreadable or invalid; the existing file was left unchanged", LogLevel.WARNING);
			return;
		}

		s_Store = loaded;
		s_iNextId = 1;
		foreach (DCO_GMCompositionRecord composition : s_Store.m_aCompositions)
		{
			if (composition.m_iId >= s_iNextId)
				s_iNextId = composition.m_iId + 1;
		}
	}

	protected static bool ValidateStore(DCO_GMCompositionStore store)
	{
		if (!store || store.m_iVersion != STORE_VERSION || !store.m_aCompositions || store.m_aCompositions.Count() > MAX_LIBRARY)
			return false;

		set<int> ids = new set<int>();
		foreach (DCO_GMCompositionRecord composition : store.m_aCompositions)
		{
			if (!composition || composition.m_iId <= 0 || ids.Contains(composition.m_iId))
				return false;
			ids.Insert(composition.m_iId);
			composition.m_sName = DCO_GMCompositionService.NormalizeName(composition.m_sName);
			composition.m_sCategory = DCO_GMCompositionService.NormalizeMetadata(composition.m_sCategory, "Custom");
			composition.m_sAuthor = DCO_GMCompositionService.NormalizeMetadata(composition.m_sAuthor, "Game Master");
			if (!ValidateComposition(composition))
				return false;
		}
		return true;
	}

	protected static bool ValidateComposition(DCO_GMCompositionRecord composition)
	{
		if (!composition.m_aItems || composition.m_aItems.IsEmpty() || composition.m_aItems.Count() > MAX_ITEMS)
			return false;

		int itemCount = composition.m_aItems.Count();
		for (int index = 0; index < itemCount; index++)
		{
			DCO_GMCompositionItem item = composition.m_aItems[index];
			if (!item || item.m_Prefab.IsEmpty() || item.m_iParentIndex < -1 || item.m_iParentIndex >= itemCount || item.m_iParentIndex == index)
				return false;
			if (item.m_iRootIndex < 0 || item.m_iRootIndex >= itemCount)
				return false;
			item.m_fScale = Math.Clamp(item.m_fScale, 0.01, 100.0);

			int ancestorIndex = index;
			int guard;
			while (composition.m_aItems[ancestorIndex].m_iParentIndex >= 0)
			{
				ancestorIndex = composition.m_aItems[ancestorIndex].m_iParentIndex;
				guard++;
				if (ancestorIndex < 0 || ancestorIndex >= itemCount || guard > itemCount)
					return false;
			}
			if (item.m_iRootIndex != ancestorIndex)
				return false;
		}
		return true;
	}

	protected static bool PersistLibrary()
	{
		if (!s_Store)
			return false;
		JsonSaveContext context = new JsonSaveContext();
		if (!context.WriteValue("", s_Store) || !context.SaveToFile(STORE_FILE))
		{
			Print("[DCO-GM] composition library SAVE FAILED", LogLevel.WARNING);
			return false;
		}
		return true;
	}

	protected static array<ref DCO_GMCompositionCaptureStage> Stages()
	{
		EnsureWorld();
		if (!s_aStages)
			s_aStages = {};
		return s_aStages;
	}

	protected static array<ref DCO_GMCompositionLastPlacement> LastPlacements()
	{
		EnsureWorld();
		if (!s_aLastPlacements)
			s_aLastPlacements = {};
		return s_aLastPlacements;
	}

	static void BeginCapture(SCR_PlayerController controller, int token, string name, string category, string author, int expectedCount)
	{
		EnsureWorld();
		if (!CanMutate(controller, "GM composition capture"))
			return;
		if (token <= 0 || expectedCount <= 0 || expectedCount > MAX_ITEMS)
		{
			SendResult(controller, false, string.Format("Capture requires 1-%1 selected entities.", MAX_ITEMS));
			return;
		}
		RemoveStagesFor(controller.GetPlayerId());
		DCO_GMCompositionCaptureStage stage = new DCO_GMCompositionCaptureStage();
		stage.m_iPlayerId = controller.GetPlayerId();
		stage.m_iToken = token;
		stage.m_iExpectedCount = expectedCount;
		stage.m_sName = DCO_GMCompositionService.NormalizeName(name);
		stage.m_sCategory = DCO_GMCompositionService.NormalizeMetadata(category, "Custom");
		stage.m_sAuthor = DCO_GMCompositionService.NormalizeMetadata(author, "Game Master");
		Stages().Insert(stage);
	}

	static void AddCaptureItem(SCR_PlayerController controller, int token, int index, RplId entityId)
	{
		if (!CanMutate(controller, "GM composition capture"))
			return;
		DCO_GMCompositionCaptureStage stage = FindStage(controller.GetPlayerId(), token);
		if (!stage)
			return;
		if (index < 0 || index >= stage.m_iExpectedCount || !entityId.IsValid())
		{
			stage.m_bInvalid = true;
			return;
		}
		foreach (DCO_GMCompositionCaptureSlot existing : stage.m_aSlots)
		{
			if (existing && (existing.m_iIndex == index || existing.m_EntityId.AsString() == entityId.AsString()))
			{
				stage.m_bInvalid = true;
				return;
			}
		}
		DCO_GMCompositionCaptureSlot slot = new DCO_GMCompositionCaptureSlot();
		slot.m_iIndex = index;
		slot.m_EntityId = entityId;
		stage.m_aSlots.Insert(slot);
	}

	static void CommitCapture(SCR_PlayerController controller, int token)
	{
		if (!CanMutate(controller, "GM composition capture"))
			return;
		DCO_GMCompositionCaptureStage stage = FindStage(controller.GetPlayerId(), token);
		if (!stage)
		{
			SendResult(controller, false, "Capture expired before the server received every selected entity.");
			return;
		}
		if (stage.m_bInvalid || stage.m_aSlots.Count() != stage.m_iExpectedCount)
		{
			Stages().RemoveItem(stage);
			SendResult(controller, false, "Capture rejected an incomplete or duplicate selection.");
			return;
		}
		if (Library().Count() >= MAX_LIBRARY)
		{
			Stages().RemoveItem(stage);
			SendResult(controller, false, string.Format("The persistent server library is full (%1 compositions).", MAX_LIBRARY));
			return;
		}

		array<SCR_EditableEntityComponent> editables = {};
		for (int index = 0; index < stage.m_iExpectedCount; index++)
		{
			DCO_GMCompositionCaptureSlot slot = FindSlot(stage, index);
			SCR_EditableEntityComponent editable = ResolveEditable(slot);
			RplId confirmedId;
			if (!editable || !editable.GetOwner() || editable.GetOwner().IsDeleted() || !editable.IsReplicated(confirmedId) || confirmedId.AsString() != slot.m_EntityId.AsString())
			{
				Stages().RemoveItem(stage);
				SendResult(controller, false, "Capture failed because a selected entity changed or disappeared on the server.");
				return;
			}
			ResourceName prefab = editable.GetPrefab();
			if (prefab.IsEmpty())
			{
				Stages().RemoveItem(stage);
				SendResult(controller, false, "Capture failed because a selected editable prefab is unavailable to the server.");
				return;
			}
			Resource resource = Resource.Load(prefab);
			if (!resource || !resource.IsValid())
			{
				Stages().RemoveItem(stage);
				SendResult(controller, false, "Capture failed because a selected editable prefab is unavailable to the server.");
				return;
			}
			editables.Insert(editable);
		}

		DCO_GMCompositionRecord composition = BuildComposition(stage, editables);
		Stages().RemoveItem(stage);
		if (!composition)
		{
			SendResult(controller, false, "Capture failed because the selection has an invalid hierarchy or terrain position.");
			return;
		}
		composition.m_iId = s_iNextId++;
		composition.m_iOwnerPlayerId = controller.GetPlayerId();
		Library().Insert(composition);
		if (!PersistLibrary())
		{
			Library().RemoveItem(composition);
			SendResult(controller, false, "Capture could not be written to the server library; nothing was saved.");
			return;
		}
		SendResult(controller, true, string.Format("Captured %1 entities as '%2'.", composition.m_aItems.Count(), composition.m_sName));
		BroadcastSnapshot();
	}

	static void Place(SCR_PlayerController controller, int compositionId, vector targetPosition)
	{
		if (!CanMutate(controller, "GM composition placement"))
			return;
		if (!SCR_Global.IsPositionWithinTerrainBounds(targetPosition))
		{
			SendResult(controller, false, "Placement refused because the target is outside the terrain bounds.");
			return;
		}
		DCO_GMCompositionRecord composition = FindComposition(compositionId);
		if (!composition || composition.m_aItems.IsEmpty())
		{
			SendResult(controller, false, "The selected persistent composition no longer exists.");
			return;
		}

		foreach (DCO_GMCompositionItem validateItem : composition.m_aItems)
		{
			if (!validateItem || validateItem.m_Prefab.IsEmpty())
			{
				SendResult(controller, false, "Placement refused because a composition prefab is no longer available.");
				return;
			}
			Resource validateResource = Resource.Load(validateItem.m_Prefab);
			if (!validateResource || !validateResource.IsValid())
			{
				SendResult(controller, false, "Placement refused because a composition prefab is no longer available.");
				return;
			}
		}

		BaseWorld world = GetGame().GetWorld();
		if (!world)
		{
			SendResult(controller, false, "Placement failed because the server world is unavailable.");
			return;
		}
		targetPosition[1] = world.GetSurfaceY(targetPosition[0], targetPosition[2]);
		array<float> rootDeltas = {};
		array<vector> plannedPositions = {};
		for (int fillIndex = 0; fillIndex < composition.m_aItems.Count(); fillIndex++)
		{
			rootDeltas.Insert(0.0);
			plannedPositions.Insert(vector.Zero);
		}

		for (int rootIndex = 0; rootIndex < composition.m_aItems.Count(); rootIndex++)
		{
			DCO_GMCompositionItem rootItem = composition.m_aItems[rootIndex];
			if (rootItem.m_iParentIndex >= 0)
				continue;
			vector rootPosition = targetPosition + rootItem.m_vOffset;
			if (!SCR_Global.IsPositionWithinTerrainBounds(rootPosition))
			{
				SendResult(controller, false, "Placement refused because part of the composition is outside the terrain bounds.");
				return;
			}
			rootDeltas[rootIndex] = world.GetSurfaceY(rootPosition[0], rootPosition[2]) + rootItem.m_fGroundOffset - rootPosition[1];
		}

		for (int planIndex = 0; planIndex < composition.m_aItems.Count(); planIndex++)
		{
			DCO_GMCompositionItem planItem = composition.m_aItems[planIndex];
			if (planItem.m_iRootIndex < 0 || planItem.m_iRootIndex >= composition.m_aItems.Count())
			{
				SendResult(controller, false, "Placement refused because the stored hierarchy is invalid.");
				return;
			}
			vector plannedPosition = targetPosition + planItem.m_vOffset;
			plannedPosition[1] = plannedPosition[1] + rootDeltas[planItem.m_iRootIndex];
			if (!SCR_Global.IsPositionWithinTerrainBounds(plannedPosition))
			{
				SendResult(controller, false, "Placement refused because part of the composition is outside the terrain bounds.");
				return;
			}
			plannedPositions[planIndex] = plannedPosition;
		}

		array<IEntity> spawnedEntities = {};
		array<SCR_EditableEntityComponent> spawnedEditables = {};
		array<bool> spawnedFlags = {};
		for (int reserveIndex = 0; reserveIndex < composition.m_aItems.Count(); reserveIndex++)
		{
			spawnedEntities.Insert(null);
			spawnedEditables.Insert(null);
			spawnedFlags.Insert(false);
		}

		int spawnedCount;
		while (spawnedCount < composition.m_aItems.Count())
		{
			bool progressed;
			for (int spawnIndex = 0; spawnIndex < composition.m_aItems.Count(); spawnIndex++)
			{
				if (spawnedFlags[spawnIndex])
					continue;
				DCO_GMCompositionItem item = composition.m_aItems[spawnIndex];
				if (item.m_iParentIndex >= 0 && !spawnedFlags[item.m_iParentIndex])
					continue;
				Resource resource = Resource.Load(item.m_Prefab);
				EntitySpawnParams spawnParams = new EntitySpawnParams();
				spawnParams.TransformMode = ETransformMode.WORLD;
				Math3D.AnglesToMatrix(item.m_vAngles, spawnParams.Transform);
				spawnParams.Transform[3] = plannedPositions[spawnIndex];
				spawnParams.Scale = item.m_fScale;
				IEntity entity = GetGame().SpawnEntityPrefab(resource, world, spawnParams);
				SCR_EditableEntityComponent editable;
				if (entity)
				{
					entity.SetScale(item.m_fScale);
					editable = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
				}
				if (!entity || !editable)
				{
					DeleteEntities(spawnedEntities);
					SendResult(controller, false, "Placement failed atomically; every partially spawned entity was rolled back.");
					return;
				}
				spawnedEntities[spawnIndex] = entity;
				spawnedEditables[spawnIndex] = editable;
				spawnedFlags[spawnIndex] = true;
				spawnedCount++;
				progressed = true;
			}
			if (!progressed)
			{
				DeleteEntities(spawnedEntities);
				SendResult(controller, false, "Placement failed atomically because the stored parent hierarchy is cyclic.");
				return;
			}
		}

		for (int linkIndex = 0; linkIndex < composition.m_aItems.Count(); linkIndex++)
		{
			DCO_GMCompositionItem linkItem = composition.m_aItems[linkIndex];
			SCR_EditableEntityComponent parentEditable;
			if (linkItem.m_iParentIndex >= 0)
				parentEditable = spawnedEditables[linkItem.m_iParentIndex];
			spawnedEditables[linkIndex].EOnEditorSessionLoad(parentEditable);
			if (parentEditable)
			{
				spawnedEditables[linkIndex].SetParentEntity(parentEditable);
				if (spawnedEditables[linkIndex].GetParentEntity() != parentEditable)
				{
					DeleteEntities(spawnedEntities);
					SendResult(controller, false, "Placement failed atomically because an editable parent relationship was rejected.");
					return;
				}
			}
		}

		RememberLastPlacement(controller.GetPlayerId(), spawnedEntities);
		SendResult(controller, true, string.Format("Placed '%1' as one %2-entity operation.", composition.m_sName, spawnedEntities.Count()));
	}

	static void DeleteLibraryEntry(SCR_PlayerController controller, int compositionId)
	{
		if (!CanMutate(controller, "GM composition library delete"))
			return;
		DCO_GMCompositionRecord composition = FindComposition(compositionId);
		if (!composition)
		{
			SendResult(controller, false, "The selected persistent composition no longer exists.");
			return;
		}
		string name = composition.m_sName;
		int libraryIndex = Library().Find(composition);
		Library().RemoveItem(composition);
		if (!PersistLibrary())
		{
			Library().InsertAt(composition, libraryIndex);
			SendResult(controller, false, "Delete could not be written to the server library; the entry was restored.");
			return;
		}
		SendResult(controller, true, "Deleted '" + name + "' from the persistent server library.");
		BroadcastSnapshot();
	}

	static void UndoLastPlacement(SCR_PlayerController controller)
	{
		if (!CanMutate(controller, "GM composition undo"))
			return;
		DCO_GMCompositionLastPlacement placement = FindLastPlacement(controller.GetPlayerId());
		if (!placement || placement.m_aEntities.IsEmpty())
		{
			SendResult(controller, false, "There is no last composition placement to undo for this Game Master.");
			return;
		}
		int count = DeleteEntities(placement.m_aEntities);
		LastPlacements().RemoveItem(placement);
		SendResult(controller, true, string.Format("Undid the last composition placement (%1 entities removed).", count));
	}

	static void SendSnapshot(SCR_PlayerController controller)
	{
		EnsureWorld();
		if (!Replication.IsServer() || !controller || !DCO_GMRights.IsGameMaster(controller.GetPlayerId()))
			return;
		int serial = ++s_iSnapshotSerial;
		controller.DCO_PushGMCompositionSnapshotBegin(serial);
		foreach (DCO_GMCompositionRecord composition : Library())
		{
			if (composition)
				controller.DCO_PushGMCompositionSnapshotRecord(serial, composition.m_iId, composition.m_sName, composition.m_sCategory, composition.m_sAuthor, composition.m_aItems.Count());
		}
		controller.DCO_PushGMCompositionSnapshotEnd(serial);
	}

	static void BroadcastSnapshot()
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;
		array<int> playerIds = {};
		players.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			if (!DCO_GMRights.IsGameMaster(playerId))
				continue;
			SCR_PlayerController controller = SCR_PlayerController.Cast(players.GetPlayerController(playerId));
			if (controller)
				SendSnapshot(controller);
		}
	}

	protected static bool CanMutate(SCR_PlayerController controller, string request)
	{
		if (!Replication.IsServer() || !controller)
			return false;
		if (DCO_GMRights.Allow(controller.GetPlayerId(), request))
			return true;
		SendResult(controller, false, "Game Master rights are required for composition changes.");
		return false;
	}

	protected static void SendResult(SCR_PlayerController controller, bool success, string message)
	{
		if (controller)
			controller.DCO_PushGMCompositionResult(success, message);
	}

	protected static DCO_GMCompositionCaptureStage FindStage(int playerId, int token)
	{
		foreach (DCO_GMCompositionCaptureStage stage : Stages())
		{
			if (stage && stage.m_iPlayerId == playerId && stage.m_iToken == token)
				return stage;
		}
		return null;
	}

	protected static void RemoveStagesFor(int playerId)
	{
		for (int i = Stages().Count() - 1; i >= 0; i--)
		{
			DCO_GMCompositionCaptureStage stage = Stages()[i];
			if (stage && stage.m_iPlayerId == playerId)
				Stages().Remove(i);
		}
	}

	protected static DCO_GMCompositionCaptureSlot FindSlot(DCO_GMCompositionCaptureStage stage, int index)
	{
		if (!stage)
			return null;
		foreach (DCO_GMCompositionCaptureSlot slot : stage.m_aSlots)
		{
			if (slot && slot.m_iIndex == index)
				return slot;
		}
		return null;
	}

	protected static SCR_EditableEntityComponent ResolveEditable(DCO_GMCompositionCaptureSlot slot)
	{
		if (!slot || !slot.m_EntityId.IsValid())
			return null;
		return SCR_EditableEntityComponent.Cast(Replication.FindItem(slot.m_EntityId));
	}

	protected static DCO_GMCompositionRecord BuildComposition(DCO_GMCompositionCaptureStage stage, notnull array<SCR_EditableEntityComponent> editables)
	{
		if (!stage || editables.IsEmpty())
			return null;
		DCO_GMCompositionRecord composition = new DCO_GMCompositionRecord();
		composition.m_sName = stage.m_sName;
		composition.m_sCategory = stage.m_sCategory;
		composition.m_sAuthor = stage.m_sAuthor;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		for (int i = 0; i < editables.Count(); i++)
		{
			SCR_EditableEntityComponent editable = editables[i];
			IEntity owner = editable.GetOwner();
			DCO_GMCompositionItem item = new DCO_GMCompositionItem();
			item.m_Prefab = editable.GetPrefab();
			item.m_fScale = Math.Clamp(owner.GetScale(), 0.01, 100.0);
			SCR_EditableEntityComponent parent = editable.GetParentEntity();
			for (int parentIndex = 0; parentIndex < editables.Count(); parentIndex++)
			{
				if (editables[parentIndex] == parent)
				{
					item.m_iParentIndex = parentIndex;
					break;
				}
			}
			vector transform[4];
			owner.GetWorldTransform(transform);
			if (!SCR_Global.IsPositionWithinTerrainBounds(transform[3]))
				return null;
			item.m_vOffset = transform[3];
			item.m_vAngles = Math3D.MatrixToAngles(transform);
			composition.m_aItems.Insert(item);
		}

		vector anchor;
		int rootCount;
		for (int rootCandidate = 0; rootCandidate < composition.m_aItems.Count(); rootCandidate++)
		{
			DCO_GMCompositionItem candidate = composition.m_aItems[rootCandidate];
			int rootIndex = rootCandidate;
			int guard;
			while (composition.m_aItems[rootIndex].m_iParentIndex >= 0)
			{
				rootIndex = composition.m_aItems[rootIndex].m_iParentIndex;
				guard++;
				if (rootIndex < 0 || rootIndex >= composition.m_aItems.Count() || guard > composition.m_aItems.Count())
					return null;
			}
			candidate.m_iRootIndex = rootIndex;
			if (candidate.m_iParentIndex < 0)
			{
				if (!SCR_Global.IsPositionWithinTerrainBounds(candidate.m_vOffset))
					return null;
				anchor = anchor + candidate.m_vOffset;
				rootCount++;
			}
		}
		if (rootCount <= 0)
			return null;
		anchor = anchor / rootCount;

		foreach (DCO_GMCompositionItem storedItem : composition.m_aItems)
		{
			vector worldPosition = storedItem.m_vOffset;
			if (storedItem.m_iParentIndex < 0)
				storedItem.m_fGroundOffset = worldPosition[1] - world.GetSurfaceY(worldPosition[0], worldPosition[2]);
			storedItem.m_vOffset = worldPosition - anchor;
		}
		return composition;
	}

	protected static DCO_GMCompositionRecord FindComposition(int id)
	{
		foreach (DCO_GMCompositionRecord composition : Library())
		{
			if (composition && composition.m_iId == id)
				return composition;
		}
		return null;
	}

	protected static DCO_GMCompositionLastPlacement FindLastPlacement(int playerId)
	{
		foreach (DCO_GMCompositionLastPlacement placement : LastPlacements())
		{
			if (placement && placement.m_iPlayerId == playerId)
				return placement;
		}
		return null;
	}

	protected static void RememberLastPlacement(int playerId, notnull array<IEntity> entities)
	{
		DCO_GMCompositionLastPlacement previous = FindLastPlacement(playerId);
		if (previous)
			LastPlacements().RemoveItem(previous);
		DCO_GMCompositionLastPlacement placement = new DCO_GMCompositionLastPlacement();
		placement.m_iPlayerId = playerId;
		foreach (IEntity entity : entities)
			placement.m_aEntities.Insert(entity);
		LastPlacements().Insert(placement);
	}

	protected static int DeleteEntities(notnull array<IEntity> entities)
	{
		int deleted;
		for (int i = entities.Count() - 1; i >= 0; i--)
		{
			IEntity entity = entities[i];
			if (!entity || entity.IsDeleted())
				continue;
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			deleted++;
		}
		return deleted;
	}
}
