class DCO_GMMissionTool
{
	static const int HIDE = 1;
	static const int RESTORE = 2;
	static const int SCALE = 3;
	static const int INVINCIBLE = 4;
	static const int INTEL = 5;
	static const int HINT = 6;
	static const int CHATTER = 7;
	static const int TELEPORTER = 8;
	static const int NAMED = 9;
	static const int REMOVE = 10;
	static const int LZ = 11;
	static const int RP = 12;
	static const int TARGET = 13;

	static string Name(int kind)
	{
		switch (kind)
		{
			case HIDE: return "Hide Terrain Objects";
			case RESTORE: return "Restore Hidden Terrain";
			case SCALE: return "Scale Object";
			case INVINCIBLE: return "Make Invincible";
			case INTEL: return "Create/Edit Intel";
			case HINT: return "Global Hint";
			case CHATTER: return "Chatter";
			case TELEPORTER: return "Create Teleporter";
			case NAMED: return "Use Named Position";
			case REMOVE: return "Remove Intel / Teleporter";
			case LZ: return "Create LZ";
			case RP: return "Create RP";
			case TARGET: return "Create Target";
		}
		return "Mission Tool";
	}
}

class DCO_GMMissionServer
{
	static bool Apply(SCR_PlayerController controller, int tool, array<RplId> ids, vector position, vector options, string title, string body, out string result)
	{
		result = "Request rejected: unsupported target or settings.";
		if (!Replication.IsServer() || !controller || !DCO_GMRights.Allow(controller.GetPlayerId(), DCO_GMMissionTool.Name(tool)))
			return false;
		if (!ids || ids.Count() > 64 || title.Length() > 64 || body.Length() > 2048)
			return false;
		title.Replace("\n", " ");
		title.Replace("\r", " ");
		title.TrimInPlace();
		body.TrimInPlace();
		if (tool == DCO_GMMissionTool.RESTORE)
		{
			DCO_GMTerrainAreaComponent.RestoreAll();
			result = "Hidden terrain restored on the server and clients.";
			return true;
		}
		if (tool == DCO_GMMissionTool.HIDE)
		{
			if (!SCR_Global.IsPositionWithinTerrainBounds(position) || !(options[0] >= 5 && options[0] <= 100) || DCO_GMTerrainAreaComponent.Count() >= 16)
				return false;
			IEntity area = Spawn("{DCA6090410000000}Prefabs/E_DCO_TerrainArea.et", position);
			if (!area)
				return false;
			DCO_GMTerrainAreaComponent component = DCO_GMTerrainAreaComponent.Cast(area.FindComponent(DCO_GMTerrainAreaComponent));
			if (!component)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(area);
				return false;
			}
			component.Configure(options[0]);
			result = "Replicated hide area created for static terrain structures and trees.";
			return true;
		}
		if (tool >= DCO_GMMissionTool.LZ && tool <= DCO_GMMissionTool.TARGET)
		{
			if (title.IsEmpty())
				return false;
			bool created = DCO_GMMarkerServer.Apply(controller.GetPlayerId(), DCO_GMMarkerMutation.CREATE, 0, tool - 10, position, "10 10 0", title);
			if (created)
				result = "Named position saved to the server marker library.";
			return created;
		}
		if (tool == DCO_GMMissionTool.HINT || tool == DCO_GMMissionTool.CHATTER)
			return Announce(controller, tool, ids, options, title, body, result);
		if (tool == DCO_GMMissionTool.INTEL || tool == DCO_GMMissionTool.TELEPORTER)
			return ConfigureInteraction(tool, ids, options, title, body, result);
		if (tool != DCO_GMMissionTool.SCALE && tool != DCO_GMMissionTool.INVINCIBLE && tool != DCO_GMMissionTool.NAMED && tool != DCO_GMMissionTool.REMOVE)
			return false;
		int applied;
		string scaleIssue;
		if (tool == DCO_GMMissionTool.SCALE && !(options[0] >= 0.25 && options[0] <= 4.0))
		{
			result = "Enter a scale from 0.25 to 4.0. Use 1.0 for original size.";
			return false;
		}
		set<RplId> seen = new set<RplId>();
		foreach (RplId id : ids)
		{
			if (seen.Contains(id))
				continue;
			seen.Insert(id);
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(id));
			if (!editable || !editable.GetOwner())
			{
				if (scaleIssue.IsEmpty())
					scaleIssue = "The selected object no longer exists. Select it again.";
				continue;
			}
			if (tool == DCO_GMMissionTool.SCALE)
			{
				if (editable.DCO_SetMissionScale(options[0]))
					applied++;
				else if (scaleIssue.IsEmpty())
					scaleIssue = SCR_EditableEntityComponent.DCO_GetScaleIssue(editable.GetOwner());
			}
			if (tool == DCO_GMMissionTool.INVINCIBLE && (options[0] == 0 || options[0] == 1))
			{
				if (editable.DCO_SetMissionInvincible(options[0] == 1))
					applied++;
				if (options[1] == 1)
				{
					SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(editable.GetOwner().FindComponent(SCR_BaseCompartmentManagerComponent));
					array<IEntity> occupants = {};
					if (compartments)
						compartments.GetOccupants(occupants);
					foreach (IEntity occupant : occupants)
					{
						if (!occupant) continue;
						SCR_EditableEntityComponent crew = SCR_EditableEntityComponent.Cast(occupant.FindComponent(SCR_EditableEntityComponent));
						if (occupant && crew && crew.DCO_SetMissionInvincible(options[0] == 1))
							applied++;
					}
				}
			}
			if (tool == DCO_GMMissionTool.NAMED && options[0] >= 1 && options[0] <= 1000000 && UseNamed(editable, Math.Round(options[0])))
				applied++;
			if (tool == DCO_GMMissionTool.REMOVE)
			{
				DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.FindTarget(id);
				if (point)
				{
					SCR_EntityHelper.DeleteEntityAndChildren(point.GetOwner());
					applied++;
				}
			}
		}
		if (tool == DCO_GMMissionTool.SCALE)
		{
			if (ids.IsEmpty())
				result = "Select a static prop or barricade, then apply a scale.";
			else
				result = string.Format("Scale Object: resized %1; skipped %2. %3", applied, seen.Count() - applied, scaleIssue);
		}
		else
			result = string.Format("%1: applied to %2 supported targets (including crew when selected).", DCO_GMMissionTool.Name(tool), applied);
		return applied > 0;
	}

	protected static IEntity Spawn(ResourceName prefab, vector position)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;
		EntitySpawnParams parameters = new EntitySpawnParams();
		parameters.TransformMode = ETransformMode.WORLD;
		parameters.Transform[3] = position;
		return GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), parameters);
	}

	protected static bool ConfigureInteraction(int tool, array<RplId> ids, vector options, string title, string body, out string result)
	{
		result = "Select one prop; enter a name and content or link name. Units, vehicles and buildings are not supported.";
		if (ids.Count() != 1 || title.IsEmpty() || body.IsEmpty() || !(options[0] >= 0 && options[0] <= 2))
			return false;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(ids[0]));
		if (!editable || !DCO_GMMissionInteractionComponent.CanBind(editable.GetOwner()))
			return false;
		DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.FindTarget(ids[0]);
		int expectedKind = DCO_GMMissionInteractionComponent.INTEL;
		if (tool == DCO_GMMissionTool.TELEPORTER) expectedKind = DCO_GMMissionInteractionComponent.TELEPORTER;
		if (point && point.m_iKind != expectedKind)
		{
			result = "This prop already has a different interaction. Use Remove Intel / Teleporter before changing its purpose.";
			return false;
		}
		if (tool == DCO_GMMissionTool.TELEPORTER)
		{
			if (body.Length() > 64 || body.Contains("\n") || body.Contains("\r") || DCO_GMMissionInteractionComponent.PairCount(body, point) >= 2)
			{
				result = "Use a link name of 1-64 characters with no more than two endpoints.";
				return false;
			}
		}
		if (!point)
		{
			if (DCO_GMMissionInteractionComponent.Count() >= 64)
				return false;
			IEntity helper = Spawn("{DCA6090420000000}Prefabs/E_DCO_MissionInteraction.et", editable.GetOwner().GetOrigin());
			if (!helper)
				return false;
			point = DCO_GMMissionInteractionComponent.Cast(helper.FindComponent(DCO_GMMissionInteractionComponent));
			if (!point)
			{
				SCR_EntityHelper.DeleteEntityAndChildren(helper);
				return false;
			}
		}
		int kind = DCO_GMMissionInteractionComponent.INTEL;
		if (tool == DCO_GMMissionTool.TELEPORTER)
		{
			kind = DCO_GMMissionInteractionComponent.TELEPORTER;
			if (body.Length() > 64)
				body = body.Substring(0, 64);
		}
		point.Configure(kind, ids[0], title, body, Math.Round(options[0]), options[1] == 1);
		result = "Intel interaction saved. Players can collect it beside the prop.";
		if (kind == DCO_GMMissionInteractionComponent.TELEPORTER)
		{
			result = "Endpoint saved. Create a second endpoint with the same link name.";
			if (point.PairedEndpoint())
				result = "Two-way teleporter linked and ready for players.";
		}
		return true;
	}

	protected static bool Announce(SCR_PlayerController sender, int tool, array<RplId> ids, vector options, string title, string body, out string result)
	{
		result = "Enter message text and a valid faction audience.";
		if (body.IsEmpty() || !(options[0] >= 0 && options[0] <= 2))
			return false;
		string faction;
		vector origin;
		if (sender.GetControlledEntity())
			origin = sender.GetControlledEntity().GetOrigin();
		if (tool == DCO_GMMissionTool.CHATTER)
		{
			if (ids.Count() > 1 || (options[0] == 2 && ids.IsEmpty() && !sender.GetControlledEntity()))
			{
				result = "Select one AI speaker, or use HQ chatter with a living player for nearby delivery.";
				return false;
			}
			faction = DCO_GMMissionJournal.FactionOf(sender);
			if (ids.Count() == 1)
			{
				SCR_EditableEntityComponent actor = SCR_EditableEntityComponent.Cast(Replication.FindItem(ids[0]));
				if (!actor || !ChimeraCharacter.Cast(actor.GetOwner()) || DCO_PlayerUtil.IsPlayer(actor.GetOwner()))
					return false;
				ChimeraCharacter speaker = ChimeraCharacter.Cast(actor.GetOwner());
				if (!speaker.GetCharacterController() || speaker.GetCharacterController().IsDead() || speaker.GetCharacterController().IsUnconscious())
				{
					result = "Choose a living, conscious AI speaker.";
					return false;
				}
				FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(actor.GetOwner().FindComponent(FactionAffiliationComponent));
				if (affiliation && affiliation.GetAffiliatedFaction())
					faction = affiliation.GetAffiliatedFaction().GetFactionKey();
				origin = actor.GetOwner().GetOrigin();
				if (actor.GetInfo())
					title = WidgetManager.Translate(actor.GetInfo().GetName());
			}
			else
				title = faction + " HQ";
			if (options[0] == 1 && faction.IsEmpty())
				return false;
		}
		if (title.IsEmpty())
			title = "Game Master";
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		int delivered;
		foreach (int id : players)
		{
			SCR_PlayerController recipient = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(id));
			if (!recipient)
				continue;
			if (tool == DCO_GMMissionTool.CHATTER)
			{
				if (options[0] == 1 && DCO_GMMissionJournal.FactionOf(recipient) != faction)
					continue;
				if (options[0] == 2 && (!recipient.GetControlledEntity() || vector.DistanceSq(recipient.GetControlledEntity().GetOrigin(), origin) > 10000))
					continue;
			}
			recipient.DCO_MissionMessage(title, body, tool == DCO_GMMissionTool.CHATTER);
			delivered++;
		}
		result = string.Format("Message delivered to %1 current players.", delivered);
		return delivered > 0;
	}

	protected static bool UseNamed(SCR_EditableEntityComponent editable, int id)
	{
		vector position;
		int kind;
		if (!DCO_GMMarkerServer.ResolveNamedPosition(id, position, kind))
			return false;
		IEntity entity = editable.GetOwner();
		SCR_AIGroup group = SCR_AIGroup.Cast(entity);
		if (group)
		{
			if (!group.GetLeaderEntity() || DCO_PlayerUtil.IsPlayer(group.GetLeaderEntity()))
				return false;
			AIWaypoint waypoint = AIWaypoint.Cast(Spawn("PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Move.et", position));
			if (!waypoint)
				return false;
			group.AddWaypoint(waypoint);
			return true;
		}
		if (!entity.FindComponent(DCO_FxExplosionComponent) && !entity.FindComponent(DCO_FxMortarComponent) && !entity.FindComponent(DCO_TaskZoneComponent))
			return false;
		vector transform[4];
		entity.GetWorldTransform(transform);
		transform[3] = position;
		return editable.SetTransform(transform, true);
	}

	static bool Teleport(SCR_PlayerController controller, IEntity endpoint, out string result)
	{
		result = "No clear arrival position. Leave the endpoint surroundings clear.";
		ChimeraCharacter character = ChimeraCharacter.Cast(controller.GetControlledEntity());
		if (!Replication.IsServer() || !character || !endpoint)
			return false;
		CompartmentAccessComponent access = CompartmentAccessComponent.Cast(character.FindComponent(CompartmentAccessComponent));
		if (access && access.IsInCompartment())
		{
			result = "Exit the vehicle before using a teleporter.";
			return false;
		}
		vector basePosition = endpoint.GetOrigin();
		World world = GetGame().GetWorld();
		for (int i = 0; i < 16; i++)
		{
			float angle = i * Math.PI2 / 8;
			float radius = 2.0 + Math.Floor(i / 8.0) * 1.5;
			vector candidate = basePosition + Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius);
			candidate[1] = world.GetSurfaceY(candidate[0], candidate[2]) + 0.15;
			if (candidate[1] <= world.GetOceanHeight(candidate[0], candidate[2]) + 0.1) continue;
			if (Math.AbsFloat(world.GetSurfaceY(candidate[0] + 0.4, candidate[2]) - candidate[1]) > 0.45 || Math.AbsFloat(world.GetSurfaceY(candidate[0], candidate[2] + 0.4) - candidate[1]) > 0.45) continue;
			if (!SCR_Global.IsPositionWithinTerrainBounds(candidate) || Math.AbsFloat(candidate[1] - basePosition[1]) > 3)
				continue;
			TraceBox trace = new TraceBox();
			trace.Start = candidate + "0 0.15 0";
			trace.End = trace.Start + "0 0.05 0";
			trace.Mins = "-0.4 0 -0.4";
			trace.Maxs = "0.4 1.9 0.4";
			trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
			trace.TargetLayers = EPhysicsLayerDefs.FireGeometry;
			trace.Exclude = character;
			if (world.TraceMove(trace, null) < 1)
				continue;
			SCR_PlayersManagerEditorComponent manager = SCR_PlayersManagerEditorComponent.Cast(SCR_PlayersManagerEditorComponent.GetInstance(SCR_PlayersManagerEditorComponent));
			if (!manager)
				return false;
			manager.TeleportPlayerToPositionServer(character, controller.GetPlayerId(), candidate);
			result = "Arrived at the linked endpoint.";
			return true;
		}
		return false;
	}
}

modded class SCR_PlayerController
{
	protected float m_fDCO_LastMissionRequest = -10000;
	protected float m_fDCO_LastMissionUse = -10000;
	protected float m_fDCO_LastIntelSnapshot = -10000;

	void DCO_SendMissionTool(int tool, array<RplId> ids, vector position, vector options, string title, string body, int requestSequence)
	{
		if (Replication.IsServer())
			DCO_RpcMissionTool(tool, ids, position, options, title, body, requestSequence);
		else
			Rpc(DCO_RpcMissionTool, tool, ids, position, options, title, body, requestSequence);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcMissionTool(int tool, array<RplId> ids, vector position, vector options, string title, string body, int requestSequence)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCO_LastMissionRequest < 300)
		{
			DCO_SendMissionResult(requestSequence, false, "Please wait a moment before applying another operation.");
			return;
		}
		m_fDCO_LastMissionRequest = now;
		string result;
		bool success = DCO_GMMissionServer.Apply(this, tool, ids, position, options, title, body, result);
		DCO_SendMissionResult(requestSequence, success, result);
	}
	protected void DCO_SendMissionResult(int requestSequence, bool success, string result)
	{
		if (GetGame().GetPlayerController() == this)
			DCO_RpcMissionResult(requestSequence, success, result);
		else
			Rpc(DCO_RpcMissionResult, requestSequence, success, result);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcMissionResult(int requestSequence, bool success, string result)
	{
		DCO_GMMissionPanel.Get().OnRequestResult(requestSequence, success, result);
	}
	void DCO_MissionMessage(string title, string body, bool chatter = false)
	{
		if (GetGame().GetPlayerController() == this)
			DCO_RpcMissionMessage(title, body, chatter);
		else
			Rpc(DCO_RpcMissionMessage, title, body, chatter);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcMissionMessage(string title, string body, bool chatter)
	{
		if (chatter)
		{
			SCR_ChatComponent.RadioProtocolMessage(title + " [AI]: " + body);
			return;
		}
		SCR_HintManagerComponent.ShowCustomHint(body, title, 15);
	}
	void DCO_UseMissionPoint(RplId id)
	{
		if (Replication.IsServer())
			DCO_RpcUseMissionPoint(id);
		else
			Rpc(DCO_RpcUseMissionPoint, id);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcUseMissionPoint(RplId id)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCO_LastMissionUse < 5000)
		{
			DCO_MissionMessage("Interaction", "Wait five seconds between uses.");
			return;
		}
		m_fDCO_LastMissionUse = now;
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(id));
		if (!rpl || !rpl.GetEntity())
			return;
		DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.Cast(rpl.GetEntity().FindComponent(DCO_GMMissionInteractionComponent));
		string result;
		if (point)
		{
			point.Use(this, result);
			DCO_MissionMessage("Interaction", result);
		}
	}
	void DCO_ClearIntel()
	{
		if (GetGame().GetPlayerController() == this)
			DCO_RpcClearIntel();
		else
			Rpc(DCO_RpcClearIntel);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcClearIntel() { DCO_GMMissionJournal.ClearLocal(); }
	void DCO_FinishIntel()
	{
		if (GetGame().GetPlayerController() == this)
			DCO_RpcFinishIntel();
		else
			Rpc(DCO_RpcFinishIntel);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcFinishIntel() { DCO_GMMissionJournal.NotifyChanged(); }
	void DCO_DeliverIntel(int id, string title, string body, bool notify)
	{
		if (GetGame().GetPlayerController() == this)
			DCO_RpcIntel(id, title, body, notify);
		else
			Rpc(DCO_RpcIntel, id, title, body, notify);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcIntel(int id, string title, string body, bool notify) { DCO_GMMissionJournal.Receive(id, title, body, notify); }

	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		if (GetGame().GetPlayerController() == this)
			GetGame().GetCallqueue().CallLater(DCO_RequestIntel, 1000, false);
	}
	void DCO_RequestIntel()
	{
		if (Replication.IsServer())
			DCO_RpcRequestIntel();
		else
			Rpc(DCO_RpcRequestIntel);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcRequestIntel()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCO_LastIntelSnapshot < 1000)
			return;
		m_fDCO_LastIntelSnapshot = now;
		DCO_GMMissionJournal.Snapshot(this);
	}
	void DCO_RequestMissionEdit(RplId targetId, int tool, int requestSequence)
	{
		if (Replication.IsServer())
			DCO_RpcMissionEdit(targetId, tool, requestSequence);
		else
			Rpc(DCO_RpcMissionEdit, targetId, tool, requestSequence);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcMissionEdit(RplId targetId, int tool, int requestSequence)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "Read mission interaction"))
		{
			DCO_SendMissionResult(requestSequence, false, "Game Master permission is required to read these settings.");
			return;
		}
		DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.FindTarget(targetId);
		if (!point)
		{
			DCO_SendMissionResult(requestSequence, false, "That interaction no longer exists. Close and reopen this editor.");
			return;
		}
		int expectedKind = DCO_GMMissionInteractionComponent.INTEL;
		if (tool == DCO_GMMissionTool.TELEPORTER) expectedKind = DCO_GMMissionInteractionComponent.TELEPORTER;
		if ((tool != DCO_GMMissionTool.INTEL && tool != DCO_GMMissionTool.TELEPORTER) || point.m_iKind != expectedKind)
		{
			DCO_SendMissionResult(requestSequence, false, "This prop already has a different interaction. Use Remove Intel / Teleporter before changing its purpose.");
			return;
		}
		if (GetGame().GetPlayerController() == this)
			DCO_RpcMissionEditReply(requestSequence, targetId, point.m_sTitle, point.m_sBody, point.m_iScope, point.m_bRemoveClue);
		else
			Rpc(DCO_RpcMissionEditReply, requestSequence, targetId, point.m_sTitle, point.m_sBody, point.m_iScope, point.m_bRemoveClue);
	}
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcMissionEditReply(int requestSequence, RplId targetId, string title, string body, int scope, bool removeClue)
	{
		DCO_GMMissionPanel.Get().OnEdit(requestSequence, targetId, title, body, scope, removeClue);
	}
}
