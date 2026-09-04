// Runtime marker registry shared by the Bifrost marker panel, map, and GM-only world overlay.

class DCO_GMMarkerKind
{
	static const int POINT = 0;
	static const int LANDING_ZONE = 1;
	static const int RALLY_POINT = 2;
	static const int TARGET = 3;
	static const int INTEL = 4;
	static const int CIRCLE = 5;
	static const int RECTANGLE = 6;
	static const int COMMENT = 7;
	static const int COUNT = 8;

	static string Label(int kind)
	{
		switch (kind)
		{
			case LANDING_ZONE: return "LZ";
			case RALLY_POINT: return "RP";
			case TARGET: return "TARGET";
			case INTEL: return "INTEL";
			case CIRCLE: return "CIRCLE";
			case RECTANGLE: return "RECT";
			case COMMENT: return "NOTE";
		}
		return "POINT";
	}

	static string Category(int kind)
	{
		if (kind == CIRCLE || kind == RECTANGLE)
			return "AREAS";
		if (kind == COMMENT)
			return "COMMENTS";
		if (kind == LANDING_ZONE || kind == RALLY_POINT || kind == TARGET || kind == INTEL)
			return "INTEL & CONTROL";
		return "POINTS";
	}
}

class DCO_GMMarkerScope
{
	static const int LOCAL = 0;
	static const int SERVER = 1;
}

class DCO_GMMarkerMutation
{
	static const int CREATE = 0;
	static const int UPDATE = 1;
	static const int DELETE = 2;
}

class DCO_GMMarkerRecord
{
	int m_iId;
	int m_iKind;
	int m_iScope;
	int m_iOwnerPlayerId;
	vector m_vPosition;
	vector m_vSizeRotation;
	string m_sName;
	int m_iNativeMarkerId = -1;
	ref SCR_MapMarkerBase m_LocalMapMarker;

	DCO_GMMarkerRecord Copy()
	{
		DCO_GMMarkerRecord copy = new DCO_GMMarkerRecord();
		copy.m_iId = m_iId;
		copy.m_iKind = m_iKind;
		copy.m_iScope = m_iScope;
		copy.m_iOwnerPlayerId = m_iOwnerPlayerId;
		copy.m_vPosition = m_vPosition;
		copy.m_vSizeRotation = m_vSizeRotation;
		copy.m_sName = m_sName;
		copy.m_iNativeMarkerId = m_iNativeMarkerId;
		return copy;
	}
}

class DCO_GMMarkerService
{
	protected static ref DCO_GMMarkerService s_Instance;
	protected ref array<ref DCO_GMMarkerRecord> m_LocalRecords = {};
	protected ref array<ref DCO_GMMarkerRecord> m_ServerRecords = {};
	protected ref array<ref DCO_GMMarkerRecord> m_PendingServerRecords = {};
	protected ref ScriptInvoker m_OnChanged = new ScriptInvoker();
	protected BaseWorld m_World;
	protected int m_iNextLocalId = -1;
	protected int m_iSnapshotSerial = -1;

	static DCO_GMMarkerService Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_GMMarkerService();
		return s_Instance;
	}

	ScriptInvoker GetOnChanged()
	{
		return m_OnChanged;
	}

	void Initialize()
	{
		BaseWorld world = GetGame().GetWorld();
		if (m_World != world)
		{
			m_World = world;
			m_LocalRecords.Clear();
			m_ServerRecords.Clear();
			m_PendingServerRecords.Clear();
			m_iNextLocalId = -1;
			m_iSnapshotSerial = -1;
		}
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.DCO_RequestGMMarkerSnapshot();
	}

	void GetRecords(out notnull array<DCO_GMMarkerRecord> records)
	{
		records.Clear();
		foreach (DCO_GMMarkerRecord localRecord : m_LocalRecords)
		{
			if (localRecord)
				records.Insert(localRecord);
		}
		foreach (DCO_GMMarkerRecord serverRecord : m_ServerRecords)
		{
			if (serverRecord)
				records.Insert(serverRecord);
		}
	}

	DCO_GMMarkerRecord Find(int id, int scope)
	{
		array<ref DCO_GMMarkerRecord> source = m_LocalRecords;
		if (scope == DCO_GMMarkerScope.SERVER)
			source = m_ServerRecords;
		foreach (DCO_GMMarkerRecord record : source)
		{
			if (record && record.m_iId == id)
				return record;
		}
		return null;
	}

	void Create(int kind, int scope, vector position, vector sizeRotation, string name)
	{
		Normalize(kind, position, sizeRotation, name);
		if (scope == DCO_GMMarkerScope.LOCAL)
		{
			DCO_GMMarkerRecord record = new DCO_GMMarkerRecord();
			record.m_iId = m_iNextLocalId--;
			record.m_iKind = kind;
			record.m_iScope = scope;
			record.m_iOwnerPlayerId = SCR_PlayerController.GetLocalPlayerId();
			record.m_vPosition = position;
			record.m_vSizeRotation = sizeRotation;
			record.m_sName = name;
			m_LocalRecords.Insert(record);
			CreateLocalMapMarker(record);
			m_OnChanged.Invoke();
			return;
		}
		SendServerMutation(DCO_GMMarkerMutation.CREATE, 0, kind, position, sizeRotation, name);
	}

	void Update(int id, int oldScope, int kind, int newScope, vector position, vector sizeRotation, string name)
	{
		Normalize(kind, position, sizeRotation, name);
		if (oldScope != newScope)
		{
			Delete(id, oldScope);
			Create(kind, newScope, position, sizeRotation, name);
			return;
		}
		if (newScope == DCO_GMMarkerScope.LOCAL)
		{
			DCO_GMMarkerRecord record = Find(id, newScope);
			if (!record)
				return;
			RemoveLocalMapMarker(record);
			record.m_iKind = kind;
			record.m_vPosition = position;
			record.m_vSizeRotation = sizeRotation;
			record.m_sName = name;
			CreateLocalMapMarker(record);
			m_OnChanged.Invoke();
			return;
		}
		SendServerMutation(DCO_GMMarkerMutation.UPDATE, id, kind, position, sizeRotation, name);
	}

	void Delete(int id, int scope)
	{
		if (scope == DCO_GMMarkerScope.LOCAL)
		{
			for (int i = m_LocalRecords.Count() - 1; i >= 0; i--)
			{
				DCO_GMMarkerRecord record = m_LocalRecords[i];
				if (!record || record.m_iId != id)
					continue;
				RemoveLocalMapMarker(record);
				m_LocalRecords.Remove(i);
				m_OnChanged.Invoke();
				return;
			}
			return;
		}
		SendServerMutation(DCO_GMMarkerMutation.DELETE, id, DCO_GMMarkerKind.POINT, vector.Zero, vector.Zero, "");
	}

	protected void SendServerMutation(int verb, int id, int kind, vector position, vector sizeRotation, string name)
	{
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (controller)
			controller.DCO_SendGMMarkerMutation(verb, id, kind, position, sizeRotation, name);
	}

	protected void Normalize(inout int kind, inout vector position, inout vector sizeRotation, inout string name)
	{
		kind = Math.ClampInt(kind, 0, DCO_GMMarkerKind.COUNT - 1);
		name.TrimInPlace();
		name.Replace("\n", " ");
		name.Replace("\r", " ");
		if (name.IsEmpty())
			name = DCO_GMMarkerKind.Label(kind);
		if (name.Length() > 64)
			name = name.Substring(0, 64);
		sizeRotation[0] = Math.Clamp(sizeRotation[0], 1.0, 2000.0);
		sizeRotation[1] = Math.Clamp(sizeRotation[1], 1.0, 2000.0);
		sizeRotation[2] = Math.Clamp(sizeRotation[2], -3600.0, 3600.0);
		while (sizeRotation[2] < 0)
			sizeRotation[2] = sizeRotation[2] + 360;
		while (sizeRotation[2] >= 360)
			sizeRotation[2] = sizeRotation[2] - 360;
		BaseWorld world = GetGame().GetWorld();
		if (world)
			position[1] = world.GetSurfaceY(position[0], position[2]);
	}

	protected SCR_MapMarkerBase PrepareMapMarker(DCO_GMMarkerRecord record)
	{
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager || !record)
			return null;
		EMilitarySymbolIdentity identity = EMilitarySymbolIdentity.BLUFOR;
		if (record.m_iKind == DCO_GMMarkerKind.TARGET)
			identity = EMilitarySymbolIdentity.OPFOR;
		SCR_MapMarkerBase marker = manager.PrepareMilitaryMarker(identity, EMilitarySymbolDimension.LAND, EMilitarySymbolIcon.INFANTRY);
		if (!marker)
			return null;
		marker.SetWorldPos(Math.Round(record.m_vPosition[0]), Math.Round(record.m_vPosition[2]));
		marker.SetCustomText(DCO_GMMarkerKind.Label(record.m_iKind) + "  " + record.m_sName);
		marker.SetCanBeRemovedByOwner(false);
		return marker;
	}

	protected void CreateLocalMapMarker(DCO_GMMarkerRecord record)
	{
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		SCR_MapMarkerBase marker = PrepareMapMarker(record);
		if (!manager || !marker)
			return;
		record.m_LocalMapMarker = marker;
		manager.InsertStaticMarker(marker, true);
	}

	protected void RemoveLocalMapMarker(DCO_GMMarkerRecord record)
	{
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (manager && record && record.m_LocalMapMarker)
			manager.RemoveStaticMarker(record.m_LocalMapMarker);
		if (record)
			record.m_LocalMapMarker = null;
	}

	void OnSnapshotBegin(int serial)
	{
		if (serial < m_iSnapshotSerial)
			return;
		m_iSnapshotSerial = serial;
		m_PendingServerRecords.Clear();
	}

	void OnSnapshotRecord(int serial, int id, int kind, int ownerPlayerId, vector position, vector sizeRotation, string name)
	{
		if (serial != m_iSnapshotSerial)
			return;
		DCO_GMMarkerRecord record = new DCO_GMMarkerRecord();
		record.m_iId = id;
		record.m_iKind = kind;
		record.m_iScope = DCO_GMMarkerScope.SERVER;
		record.m_iOwnerPlayerId = ownerPlayerId;
		record.m_vPosition = position;
		record.m_vSizeRotation = sizeRotation;
		record.m_sName = name;
		m_PendingServerRecords.Insert(record);
	}

	void OnSnapshotEnd(int serial)
	{
		if (serial != m_iSnapshotSerial)
			return;
		m_ServerRecords.Clear();
		foreach (DCO_GMMarkerRecord record : m_PendingServerRecords)
			m_ServerRecords.Insert(record);
		m_PendingServerRecords.Clear();
		m_OnChanged.Invoke();
	}
}

class DCO_GMMarkerServer
{
	protected static ref array<ref DCO_GMMarkerRecord> s_Records;
	protected static int s_iNextId = 1;
	protected static int s_iSnapshotSerial;
	protected static BaseWorld s_World;

	protected static void EnsureWorld()
	{
		BaseWorld world = GetGame().GetWorld();
		if (s_World == world)
			return;
		s_World = world;
		s_Records = {};
		s_iNextId = 1;
		s_iSnapshotSerial = 0;
	}

	protected static array<ref DCO_GMMarkerRecord> Records()
	{
		EnsureWorld();
		if (!s_Records)
			s_Records = {};
		return s_Records;
	}

	static bool Apply(int playerId, int verb, int id, int kind, vector position, vector sizeRotation, string name)
	{
		EnsureWorld();
		if (!Replication.IsServer() || !DCO_GMRights.Allow(playerId, "GM marker mutation"))
			return false;
		if (verb != DCO_GMMarkerMutation.CREATE && verb != DCO_GMMarkerMutation.UPDATE && verb != DCO_GMMarkerMutation.DELETE)
			return false;
		if (verb != DCO_GMMarkerMutation.DELETE && !SCR_Global.IsPositionWithinTerrainBounds(position))
			return false;
		kind = Math.ClampInt(kind, 0, DCO_GMMarkerKind.COUNT - 1);
		name.TrimInPlace();
		name.Replace("\n", " ");
		name.Replace("\r", " ");
		if (name.IsEmpty())
			name = DCO_GMMarkerKind.Label(kind);
		if (name.Length() > 64)
			name = name.Substring(0, 64);
		sizeRotation[0] = Math.Clamp(sizeRotation[0], 1.0, 2000.0);
		sizeRotation[1] = Math.Clamp(sizeRotation[1], 1.0, 2000.0);
		sizeRotation[2] = Math.Clamp(sizeRotation[2], -3600.0, 3600.0);
		while (sizeRotation[2] < 0)
			sizeRotation[2] = sizeRotation[2] + 360;
		while (sizeRotation[2] >= 360)
			sizeRotation[2] = sizeRotation[2] - 360;
		BaseWorld world = GetGame().GetWorld();
		if (world && verb != DCO_GMMarkerMutation.DELETE)
			position[1] = world.GetSurfaceY(position[0], position[2]);

		DCO_GMMarkerRecord record = Find(id);
		if (verb == DCO_GMMarkerMutation.CREATE)
		{
			record = new DCO_GMMarkerRecord();
			record.m_iId = s_iNextId++;
			record.m_iOwnerPlayerId = playerId;
			record.m_iScope = DCO_GMMarkerScope.SERVER;
			Records().Insert(record);
		}
		else if (!record)
		{
			return false;
		}

		if (verb == DCO_GMMarkerMutation.DELETE)
		{
			RemoveNativeMarker(record);
			Records().RemoveItem(record);
			BroadcastSnapshot();
			return true;
		}

		RemoveNativeMarker(record);
		record.m_iKind = kind;
		record.m_vPosition = position;
		record.m_vSizeRotation = sizeRotation;
		record.m_sName = name;
		CreateNativeMarker(record);
		BroadcastSnapshot();
		return true;
	}

	protected static DCO_GMMarkerRecord Find(int id)
	{
		foreach (DCO_GMMarkerRecord record : Records())
		{
			if (record && record.m_iId == id)
				return record;
		}
		return null;
	}

	protected static void CreateNativeMarker(DCO_GMMarkerRecord record)
	{
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager || !record || record.m_iKind == DCO_GMMarkerKind.COMMENT)
			return;
		EMilitarySymbolIdentity identity = EMilitarySymbolIdentity.BLUFOR;
		if (record.m_iKind == DCO_GMMarkerKind.TARGET)
			identity = EMilitarySymbolIdentity.OPFOR;
		SCR_MapMarkerBase marker = manager.PrepareMilitaryMarker(identity, EMilitarySymbolDimension.LAND, EMilitarySymbolIcon.INFANTRY);
		if (!marker)
			return;
		marker.SetWorldPos(Math.Round(record.m_vPosition[0]), Math.Round(record.m_vPosition[2]));
		marker.SetCustomText(DCO_GMMarkerKind.Label(record.m_iKind) + "  " + record.m_sName);
		marker.SetCanBeRemovedByOwner(false);
		manager.InsertStaticMarker(marker, false, true);
		record.m_iNativeMarkerId = marker.GetMarkerID();
	}

	protected static void RemoveNativeMarker(DCO_GMMarkerRecord record)
	{
		if (!record || record.m_iNativeMarkerId < 0)
			return;
		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (manager)
		{
			SCR_MapMarkerBase marker = manager.GetStaticMarkerByID(record.m_iNativeMarkerId);
			if (marker)
				manager.RemoveStaticMarker(marker);
		}
		record.m_iNativeMarkerId = -1;
	}

	static void SendSnapshot(SCR_PlayerController controller)
	{
		EnsureWorld();
		if (!Replication.IsServer() || !controller || !DCO_GMRights.IsGameMaster(controller.GetPlayerId()))
			return;
		int serial = ++s_iSnapshotSerial;
		controller.DCO_PushGMMarkerSnapshotBegin(serial);
		foreach (DCO_GMMarkerRecord record : Records())
		{
			if (record)
				controller.DCO_PushGMMarkerSnapshotRecord(serial, record.m_iId, record.m_iKind, record.m_iOwnerPlayerId, record.m_vPosition, record.m_vSizeRotation, record.m_sName);
		}
		controller.DCO_PushGMMarkerSnapshotEnd(serial);
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
}
