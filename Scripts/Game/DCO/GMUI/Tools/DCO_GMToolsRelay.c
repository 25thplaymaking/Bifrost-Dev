// Routes GM entity changes through server authority.
class DCO_GMToolsServer
{
	static const int TOOL_INVULN   = 1;
	static const int TOOL_TELEPORT = 2;
	static const int TOOL_FLYBY    = 3;

	// Precise-mode SIM OPTIONS panel toggles.
	static const int TOOL_SIM       = 4;
	static const int TOOL_AI        = 5;
	static const int TOOL_COLLISION = 6;

	// Precise-mode stance controls.
	static const int TOOL_STANCE    = 7;	// pos[0] = 0 stand / 1 crouch / 2 prone.
	static const int TOOL_WEAPONRAISED = 9;
	static const int TOOL_TRACER_FIRE = 10;

	static const int TOOL_ZONE_RADIUS = 11;	// pos[0] = radius m.
	static const int TOOL_ZONE_PAIR   = 12;	// pos[0] = pair id 0-50.
	static const int TOOL_ZONE_RANGE  = 13;
	static const int TOOL_ZONE_SPRING = 14;	// no payload.
	static const int TOOL_ZONE_REARM  = 15;	// no payload.
	static const int TOOL_SENDGROUP   = 16;	// target = GROUP entity, pos = zone center.

	// Single client-side entry point.
	static void Route(int toolId, IEntity target, vector pos)
	{
		if (!target)
			return;

		if (Replication.IsServer())
		{
			SCR_PlayerController localController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (!localController || !DCO_GMRights.Allow(localController.GetPlayerId(), "GM tool"))
				return;
			ApplyOn(target, toolId, pos);
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
		{
			Print("[DCO-GM] tool relay: no local player controller (cannot reach the server)", LogLevel.WARNING);
			return;
		}
		RplComponent rpl = RplComponent.Cast(target.FindComponent(RplComponent));
		if (!rpl || !rpl.Id().IsValid())
		{
			Print("[DCO-GM] tool relay: target has no valid replication id (local-only entity?) - tool skipped", LogLevel.WARNING);
			return;
		}
		pc.DCO_SendGMTool(toolId, rpl.Id(), pos);
	}

	static void RequestAuthorityState(IEntity target)
	{
		if (!target || Replication.IsServer())
			return;
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		RplComponent rpl = RplComponent.Cast(target.FindComponent(RplComponent));
		if (pc && rpl && rpl.Id().IsValid())
			pc.DCO_RequestGMToolState(rpl.Id());
	}

	// Server side: resolve the wire id back to the entity and apply.
	static bool Apply(int toolId, RplId targetId, vector pos, out bool confirmedState)
	{
		if (!Replication.IsServer())
			return false;
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (!rpl)
		{
			Print("[DCO-GM] tool relay: RplId did not resolve on the server - tool dropped", LogLevel.WARNING);
			return false;
		}
		IEntity target = rpl.GetEntity();
		ApplyOn(target, toolId, pos);
		return ReadState(target, toolId, confirmedState);
	}

	static bool ReadState(IEntity target, int toolId, out bool state)
	{
		if (!target)
			return false;
		DCO_GMTools tools = DCO_GMTools.Get();
		switch (toolId)
		{
			case TOOL_SIM:       { state = tools.IsSimOn(target); return true; }
			case TOOL_AI:        { state = tools.IsAIOn(target); return true; }
			case TOOL_COLLISION: { state = tools.IsCollisionOn(target); return true; }
			case TOOL_INVULN:    { state = tools.IsDamageOn(target); return true; }
			case TOOL_WEAPONRAISED: { state = tools.IsWeaponRaisedOn(target); return true; }
			case TOOL_TRACER_FIRE:  { state = tools.IsTracerFiring(target); return true; }
		}
		return false;
	}

	// Applies the requested change on authority.
	static void ApplyOn(IEntity target, int toolId, vector pos)
	{
		if (!Replication.IsServer() || !target)
			return;
		int ord = Math.Round(pos[0]);	// Stance and option controls carry their ordinal in pos[0].
		switch (toolId)
		{
			case TOOL_INVULN:    { DCO_GMTools.Get().ToggleInvulnEntity(target);     break; }
			case TOOL_TELEPORT:  { DCO_GMTools.Get().TeleportEntityTo(target, pos);  break; }
			case TOOL_FLYBY:     { DCO_GMTools.Get().FlybyEntity(target);            break; }
			case TOOL_SIM:       { DCO_GMTools.Get().ToggleSimEntity(target);        break; }
			case TOOL_AI:        { DCO_GMTools.Get().ToggleAIEntity(target);         break; }
			case TOOL_COLLISION: { DCO_GMTools.Get().ToggleCollisionEntity(target);  break; }
			case TOOL_STANCE:    { DCO_GMTools.Get().SetStanceEntity(target, ord);       break; }
			case TOOL_WEAPONRAISED: { DCO_GMTools.Get().ToggleWeaponRaisedEntity(target); break; }
			case TOOL_TRACER_FIRE: { DCO_GMTools.Get().ToggleTracerFireEntity(target);   break; }
			case TOOL_ZONE_RADIUS: { DCO_TaskZoneGMTools.SetRadius(target, pos[0]);    break; }
			case TOOL_ZONE_PAIR:   { DCO_TaskZoneGMTools.SetPairId(target, ord);       break; }
			case TOOL_ZONE_RANGE:  { DCO_TaskZoneGMTools.SetPushRange(target, pos[0]); break; }
			case TOOL_ZONE_SPRING: { DCO_TaskZoneGMTools.Spring(target);               break; }
			case TOOL_ZONE_REARM:  { DCO_TaskZoneGMTools.Rearm(target);                break; }
			case TOOL_SENDGROUP:   { DCO_TaskZoneGMTools.SendGroupToZone(target, pos); break; }
			default:            { Print(string.Format("[DCO-GM] tool relay: unknown tool id %1", toolId), LogLevel.WARNING); break; }
		}
	}


	// Client-side entry: route the editable entity's transform through server authority.
	static void RouteTransform(IEntity target, vector pos, vector anglesDeg, bool finalCommit = true)
	{
		if (!target)
			return;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(target.FindComponent(SCR_EditableEntityComponent));
		if (!editable)
			return;

		if (Replication.IsServer())
		{
			ApplyTransform(editable, pos, anglesDeg, finalCommit);
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return;
		RplId editableId;
		if (!editable.IsReplicated(editableId) || !editableId.IsValid())
			return;	// local-only entity has nothing to move on the server.
		pc.DCO_SendGMTransform(editableId, pos, anglesDeg, finalCommit);
	}

	// Server side: resolve the editable component and use its replicated transform path.
	static void ApplyTransform(RplId editableId, vector pos, vector anglesDeg, bool finalCommit)
	{
		if (!Replication.IsServer())
			return;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(Replication.FindItem(editableId));
		if (!editable)
			return;
		ApplyTransform(editable, pos, anglesDeg, finalCommit);
	}

	protected static void ApplyTransform(SCR_EditableEntityComponent editable, vector pos, vector anglesDeg, bool finalCommit)
	{
		if (!Replication.IsServer() || !editable || !editable.GetOwner())
			return;
		vector rot[3];
		Math3D.AnglesToMatrix(anglesDeg, rot);
		vector mat[4];
		mat[0] = rot[0];
		mat[1] = rot[1];
		mat[2] = rot[2];
		mat[3] = pos;
		if (editable.SetTransform(mat, finalCommit))
			DCO_GMTools.Get().ReanchorFrozen(editable.GetOwner());
	}
}

// Clears only entities the engine already tracks as garbage. Characters and
// vehicles receive stricter state checks so incapacitated AI, player corpses,
// usable vehicles, and occupied wrecks fail closed.
class DCO_GMGarbageServer
{
	protected static const int GARBAGE_SKIP = 0;
	protected static const int GARBAGE_AI_BODY = 1;
	protected static const int GARBAGE_WRECK = 2;
	protected static const int GARBAGE_DISCARDED_ITEM = 3;

	static int Clear(out int bodyCount, out int wreckCount, out int discardedCount, out int skippedCount)
	{
		bodyCount = 0;
		wreckCount = 0;
		discardedCount = 0;
		skippedCount = 0;
		if (!Replication.IsServer())
			return 0;

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		GarbageSystem garbageSystem;
		if (world)
			garbageSystem = world.GetGarbageSystem();
		if (!garbageSystem)
		{
			Print("[DCO-GM] clear garbage unavailable: world garbage system not found", LogLevel.WARNING);
			return 0;
		}

		array<IEntity> tracked = {};
		garbageSystem.FetchTrackedEntities(tracked);
		array<IEntity> bodies = {};
		array<IEntity> wrecks = {};
		array<IEntity> discarded = {};
		foreach (IEntity entity : tracked)
		{
			int garbageType = Classify(entity);
			switch (garbageType)
			{
				case GARBAGE_AI_BODY: { bodies.Insert(entity); break; }
				case GARBAGE_WRECK: { wrecks.Insert(entity); break; }
				case GARBAGE_DISCARDED_ITEM: { discarded.Insert(entity); break; }
				default: { skippedCount++; break; }
			}
		}

		bodyCount = DeleteAll(bodies);
		wreckCount = DeleteAll(wrecks);
		discardedCount = DeleteAll(discarded);
		return bodyCount + wreckCount + discardedCount;
	}

	protected static int Classify(IEntity entity)
	{
		if (!entity || entity.IsDeleted())
			return GARBAGE_SKIP;

		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.GetEditableEntity(entity);
		if (editable && editable.HasEntityFlag(EEditableEntityFlag.NON_DELETABLE))
			return GARBAGE_SKIP;

		if (ChimeraCharacter.Cast(entity))
		{
			if (IsPlayerEntity(entity))
				return GARBAGE_SKIP;
			DamageManagerComponent damageManager = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
			if (!damageManager || !damageManager.IsDestroyed())
				return GARBAGE_SKIP;
			return GARBAGE_AI_BODY;
		}

		if (BaseVehicle.Cast(entity))
		{
			if (HasPlayerOccupant(entity))
				return GARBAGE_SKIP;
			DamageManagerComponent damageManager = DamageManagerComponent.Cast(entity.FindComponent(DamageManagerComponent));
			if (!damageManager || !damageManager.IsDestroyed())
				return GARBAGE_SKIP;
			return GARBAGE_WRECK;
		}

		// The stock rules track dropped InventoryItemComponent entities. Refuse an
		// attached item in case collection and inventory attachment cross in one frame.
		IEntity root = entity.GetRootParent();
		if (root && root != entity)
			return GARBAGE_SKIP;
		return GARBAGE_DISCARDED_ITEM;
	}

	protected static bool IsPlayerEntity(IEntity entity)
	{
		if (!entity)
			return false;
		SCR_EditableCharacterComponent editableCharacter = SCR_EditableCharacterComponent.Cast(
			entity.FindComponent(SCR_EditableCharacterComponent));
		if (editableCharacter && editableCharacter.GetPlayerID() > 0)
			return true;
		return SCR_PossessingManagerComponent.GetPlayerIdFromMainEntity(entity) > 0;
	}

	protected static bool HasPlayerOccupant(IEntity vehicle)
	{
		SCR_BaseCompartmentManagerComponent compartmentManager = SCR_BaseCompartmentManagerComponent.Cast(
			vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartmentManager)
			return false;
		array<IEntity> occupants = {};
		compartmentManager.GetOccupants(occupants);
		foreach (IEntity occupant : occupants)
		{
			if (IsPlayerEntity(occupant))
				return true;
		}
		return false;
	}

	protected static int DeleteAll(notnull array<IEntity> entities)
	{
		int removed;
		foreach (IEntity entity : entities)
		{
			if (!entity || entity.IsDeleted())
				continue;
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.GetEditableEntity(entity);
			if (editable)
			{
				if (!editable.Delete(true, true))
					continue;
			}
			else
			{
				SCR_EntityHelper.DeleteEntityAndChildren(entity);
			}
			removed++;
		}
		return removed;
	}
}

modded class SCR_PlayerController
{
	[RplProp(onRplName: "DCO_OnPauseStateChanged")]
	protected bool m_bDCO_PauseState;
	protected int m_iDCO_AIOverlaySerial;

	override protected void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (Replication.IsServer())
		{
			m_bDCO_PauseState = DCO_GMPauseCore.Get().IsActive();
			GRSA_InitializeArsenalScenarioPolicy(GRSA_ArsenalScenarioSettings.Get().Pack());
		}
	}

	void DCO_SendGMBriefing(int entryId, string text)
	{
		if (Replication.IsServer())
		{
			DCO_GMBriefing.Apply(GetPlayerId(), entryId, text);
			return;
		}
		Rpc(DCO_RpcGMBriefing, entryId, text);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMBriefing(int entryId, string text)
	{
		DCO_GMBriefing.Apply(GetPlayerId(), entryId, text);
	}

	// Authority-owned replicated pause presentation.
	void DCO_SetPauseState(bool paused)
	{
		if (!Replication.IsServer() || m_bDCO_PauseState == paused)
			return;
		m_bDCO_PauseState = paused;
		Replication.BumpMe();
		DCO_OnPauseStateChanged();
	}

	protected void DCO_OnPauseStateChanged()
	{
		DCO_GMPausePresentationState.SetPaused(m_bDCO_PauseState);
	}

	// Sends the GM request to authority.
	void DCO_SendGMTool(int toolId, RplId targetId, vector pos)
	{
		Rpc(DCO_RpcGMTool, toolId, targetId, pos);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMTool(int toolId, RplId targetId, vector pos)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM tool"))
			return;
		bool confirmedState;
		if (DCO_GMToolsServer.Apply(toolId, targetId, pos, confirmedState))
			Rpc(DCO_RpcGMToolConfirmed, toolId, targetId, confirmedState);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMToolConfirmed(int toolId, RplId targetId, bool confirmedState)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (rpl)
			DCO_GMTools.Get().MirrorAuthorityState(rpl.GetEntity(), toolId, confirmedState);
	}

	void DCO_SendClearGarbage()
	{
		if (Replication.IsServer())
		{
			int bodies, wrecks, discarded, skipped;
			DCO_ClearGarbageOnAuthority(bodies, wrecks, discarded, skipped);
			return;
		}
		Rpc(DCO_RpcClearGarbage);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcClearGarbage()
	{
		int bodies, wrecks, discarded, skipped;
		DCO_ClearGarbageOnAuthority(bodies, wrecks, discarded, skipped);
	}

	protected bool DCO_ClearGarbageOnAuthority(out int bodies, out int wrecks, out int discarded, out int skipped)
	{
		bodies = 0;
		wrecks = 0;
		discarded = 0;
		skipped = 0;
		if (!DCO_GMRights.Allow(GetPlayerId(), "clear garbage"))
			return false;
		DCO_GMGarbageServer.Clear(bodies, wrecks, discarded, skipped);
		return true;
	}

	void DCO_RequestGMToolState(RplId targetId)
	{
		Rpc(DCO_RpcRequestGMToolState, targetId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMToolState(RplId targetId)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM tool state"))
			return;
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (!rpl)
			return;
		IEntity target = rpl.GetEntity();
		DCO_GMTools tools = DCO_GMTools.Get();
		Rpc(DCO_RpcGMToolState, targetId, tools.IsSimOn(target), tools.IsAIOn(target));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMToolState(RplId targetId, bool simOn, bool aiOn)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (!rpl)
			return;
		IEntity target = rpl.GetEntity();
		DCO_GMTools.Get().MirrorAuthorityState(target, DCO_GMToolsServer.TOOL_SIM, simOn);
		DCO_GMTools.Get().MirrorAuthorityState(target, DCO_GMToolsServer.TOOL_AI, aiOn);
	}

	// Relays gameplay pause and clock controls.
	void DCO_SendGMPause(int scope, int aspectMask, bool on, RplId selectedTargetId)
	{
		Rpc(DCO_RpcGMPause, scope, aspectMask, on, selectedTargetId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMPause(int scope, int aspectMask, bool on, RplId selectedTargetId)
	{
		int playerId = GetPlayerId();
		DCO_GMPauseCore pauseCore = DCO_GMPauseCore.Get();
		if (on)
		{
			if (!DCO_GMRights.Allow(playerId, "GM pause"))
				return;
		}
		else if (!DCO_GMRights.IsGameMaster(playerId) && !pauseCore.IsRequestOwner(playerId))
		{
			Print(string.Format("[DCO-GM] REFUSED GM resume from player %1 - not the active pause owner", playerId), LogLevel.WARNING);
			return;
		}
		int frozenCount = DCO_GMPauseServer.ApplyPause(scope, aspectMask, on, selectedTargetId);
		if (on && frozenCount > 0)
			pauseCore.NoteRequestOwner(playerId);
		Rpc(DCO_RpcGMPauseConfirmed, frozenCount > 0, frozenCount);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMPauseConfirmed(bool paused, int frozenCount)
	{
		DCO_GMPausePresentationState.SetConfirmed(paused, frozenCount);
	}

	void DCO_SendGMClock(float mult)
	{
		Rpc(DCO_RpcGMClock, mult);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMClock(float mult)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM clock"))
			return;
		DCO_GMPauseServer.ApplyClock(mult);
	}

	void DCO_SendGMBudgetLimits(RplId componentOwnerId, bool enabled)
	{
		Rpc(DCO_RpcGMBudgetLimits, componentOwnerId, enabled);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMBudgetLimits(RplId componentOwnerId, bool enabled)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM budget limits"))
			return;
		DCO_GMBudgetServer.Apply(componentOwnerId, enabled);
	}

	void DCO_SendGMAttach(RplId childId, RplId parentId, bool attach)
	{
		Rpc(DCO_RpcGMAttach, childId, parentId, attach);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMAttach(RplId childId, RplId parentId, bool attach)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM attach"))
			return;
		DCO_GMAttach.ApplyRelayed(childId, parentId, attach, GetPlayerId());
	}

	void DCO_SendGMDetachAll()
	{
		Rpc(DCO_RpcGMDetachAll);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMDetachAll()
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM detach all"))
			return;
		DCO_GMAttach.DetachAllOnAuthority(GetPlayerId());
	}

	void DCO_RequestGMAIOverlay(vector cameraPosition, int requestMask, string selectedIds, string pathIds, string groupPathIds)
	{
		Rpc(DCO_RpcRequestGMAIOverlay, cameraPosition, requestMask, selectedIds, pathIds, groupPathIds);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMAIOverlay(vector cameraPosition, int requestMask, string selectedIds, string pathIds, string groupPathIds)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM AI overlay"))
			return;
		m_iDCO_AIOverlaySerial++;
		array<string> chunks = {};
		DCO_GMAIOverlaySnapshot.BuildChunks(cameraPosition, requestMask, selectedIds, pathIds, groupPathIds, m_iDCO_AIOverlaySerial, chunks);
		foreach (string chunk : chunks)
			Rpc(DCO_RpcReceiveGMAIOverlay, chunk);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void DCO_RpcReceiveGMAIOverlay(string payload)
	{
		DCO_GMAIOverlaySnapshot.Receive(payload);
	}

	void DCO_SendGMMarkerMutation(int verb, int id, int kind, vector position, vector sizeRotation, string name)
	{
		Rpc(DCO_RpcGMMarkerMutation, verb, id, kind, position, sizeRotation, name);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMMarkerMutation(int verb, int id, int kind, vector position, vector sizeRotation, string name)
	{
		DCO_GMMarkerServer.Apply(GetPlayerId(), verb, id, kind, position, sizeRotation, name);
	}

	void DCO_RequestGMMarkerSnapshot()
	{
		Rpc(DCO_RpcRequestGMMarkerSnapshot);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMMarkerSnapshot()
	{
		DCO_GMMarkerServer.SendSnapshot(this);
	}

	void DCO_PushGMMarkerSnapshotBegin(int serial)
	{
		Rpc(DCO_RpcGMMarkerSnapshotBegin, serial);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMMarkerSnapshotBegin(int serial)
	{
		DCO_GMMarkerService.Get().OnSnapshotBegin(serial);
	}

	void DCO_PushGMMarkerSnapshotRecord(int serial, int id, int kind, int ownerPlayerId, vector position, vector sizeRotation, string name)
	{
		Rpc(DCO_RpcGMMarkerSnapshotRecord, serial, id, kind, ownerPlayerId, position, sizeRotation, name);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMMarkerSnapshotRecord(int serial, int id, int kind, int ownerPlayerId, vector position, vector sizeRotation, string name)
	{
		DCO_GMMarkerService.Get().OnSnapshotRecord(serial, id, kind, ownerPlayerId, position, sizeRotation, name);
	}

	void DCO_PushGMMarkerSnapshotEnd(int serial)
	{
		Rpc(DCO_RpcGMMarkerSnapshotEnd, serial);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMMarkerSnapshotEnd(int serial)
	{
		DCO_GMMarkerService.Get().OnSnapshotEnd(serial);
	}

	void DCO_RequestGMVisibilityCheck(vector point, float maxDistance, int sequence)
	{
		Rpc(DCO_RpcRequestGMVisibilityCheck, point, maxDistance, sequence);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMVisibilityCheck(vector point, float maxDistance, int sequence)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM visibility check"))
			return;
		int viewerCount;
		float nearestDistance;
		bool visible = DCO_GMVisibilityServer.Evaluate(GetPlayerId(), point, maxDistance, viewerCount, nearestDistance);
		Rpc(DCO_RpcGMVisibilityCheckResult, sequence, visible, viewerCount, nearestDistance);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMVisibilityCheckResult(int sequence, bool visible, int viewerCount, float nearestDistance)
	{
		DCO_GMVisibilityIndicator.Get().OnResult(sequence, visible, viewerCount, nearestDistance);
	}

	void DCO_BeginGMCompositionCapture(int token, string name, string category, string author, int expectedCount)
	{
		Rpc(DCO_RpcBeginGMCompositionCapture, token, name, category, author, expectedCount);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcBeginGMCompositionCapture(int token, string name, string category, string author, int expectedCount)
	{
		DCO_GMCompositionServer.BeginCapture(this, token, name, category, author, expectedCount);
	}

	void DCO_AddGMCompositionCaptureItem(int token, int index, RplId entityId)
	{
		Rpc(DCO_RpcAddGMCompositionCaptureItem, token, index, entityId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcAddGMCompositionCaptureItem(int token, int index, RplId entityId)
	{
		DCO_GMCompositionServer.AddCaptureItem(this, token, index, entityId);
	}

	void DCO_CommitGMCompositionCapture(int token)
	{
		Rpc(DCO_RpcCommitGMCompositionCapture, token);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcCommitGMCompositionCapture(int token)
	{
		DCO_GMCompositionServer.CommitCapture(this, token);
	}

	void DCO_RequestGMCompositionSnapshot()
	{
		Rpc(DCO_RpcRequestGMCompositionSnapshot);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMCompositionSnapshot()
	{
		DCO_GMCompositionServer.SendSnapshot(this);
	}

	void DCO_RequestGMCompositionPlace(int compositionId, vector position)
	{
		Rpc(DCO_RpcRequestGMCompositionPlace, compositionId, position);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMCompositionPlace(int compositionId, vector position)
	{
		DCO_GMCompositionServer.Place(this, compositionId, position);
	}

	void DCO_RequestGMCompositionDelete(int compositionId)
	{
		Rpc(DCO_RpcRequestGMCompositionDelete, compositionId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMCompositionDelete(int compositionId)
	{
		DCO_GMCompositionServer.DeleteLibraryEntry(this, compositionId);
	}

	void DCO_RequestGMCompositionUndo()
	{
		Rpc(DCO_RpcRequestGMCompositionUndo);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMCompositionUndo()
	{
		DCO_GMCompositionServer.UndoLastPlacement(this);
	}

	void DCO_PushGMCompositionSnapshotBegin(int serial)
	{
		Rpc(DCO_RpcGMCompositionSnapshotBegin, serial);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMCompositionSnapshotBegin(int serial)
	{
		DCO_GMCompositionService.Get().OnSnapshotBegin(serial);
	}

	void DCO_PushGMCompositionSnapshotRecord(int serial, int id, string name, string category, string author, int itemCount)
	{
		Rpc(DCO_RpcGMCompositionSnapshotRecord, serial, id, name, category, author, itemCount);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMCompositionSnapshotRecord(int serial, int id, string name, string category, string author, int itemCount)
	{
		DCO_GMCompositionService.Get().OnSnapshotRecord(serial, id, name, category, author, itemCount);
	}

	void DCO_PushGMCompositionSnapshotEnd(int serial)
	{
		Rpc(DCO_RpcGMCompositionSnapshotEnd, serial);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMCompositionSnapshotEnd(int serial)
	{
		DCO_GMCompositionService.Get().OnSnapshotEnd(serial);
	}

	void DCO_PushGMCompositionResult(bool success, string message)
	{
		Rpc(DCO_RpcGMCompositionResult, success, message);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMCompositionResult(bool success, string message)
	{
		DCO_GMCompositionService.Get().OnResult(success, message);
	}

	void DCO_SendGMOrderFor(RplId groupId, int actionId)
	{
		Rpc(DCO_RpcGMOrderFor, groupId, actionId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMOrderFor(RplId groupId, int actionId)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM order"))
			return;
		DCO_GMGroupOrders.ApplyRelayed(groupId, actionId);
	}

	void DCO_SendGMTransform(RplId editableId, vector pos, vector anglesDeg, bool finalCommit)
	{
		Rpc(DCO_RpcGMTransform, editableId, pos, anglesDeg, finalCommit);
	}

	void DCO_SendTriggerSync(RplId groupId, RplId triggerId)
	{
		Rpc(DCO_RpcTriggerSync, groupId, triggerId);
	}

	void DCO_SendTriggerPlacement(vector position)
	{
		Rpc(DCO_RpcTriggerPlacement, position);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcTriggerPlacement(vector position)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "trigger placement"))
		{
			Rpc(DCO_RpcTriggerPlacementConfirmed, false, "Trigger placement refused: Game Master rights required.");
			return;
		}
		string result;
		bool success = DCO_TriggerPlacementServer.Apply(position, result);
		Rpc(DCO_RpcTriggerPlacementConfirmed, success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcTriggerPlacementConfirmed(bool success, string result)
	{
		DCO_GMTools.Get().OnTriggerPlacementResult(success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcTriggerSync(RplId groupId, RplId triggerId)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "trigger sync"))
		{
			Rpc(DCO_RpcTriggerSyncConfirmed, false, "Sync refused: Game Master rights required.");
			return;
		}
		string result;
		bool success = DCO_TriggerSyncServer.Apply(groupId, triggerId, result);
		Rpc(DCO_RpcTriggerSyncConfirmed, success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcTriggerSyncConfirmed(bool success, string result)
	{
		DCO_TriggerSyncDrag.Get().OnAuthorityResult(success, result);
	}

	void DCO_SendAnimationFx(RplId targetId, int animation, bool leaveWhenThreatened)
	{
		Rpc(DCO_RpcAnimationFx, targetId, animation, leaveWhenThreatened);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcAnimationFx(RplId targetId, int animation, bool leaveWhenThreatened)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "animation FX"))
		{
			Rpc(DCO_RpcAnimationFxConfirmed, false, "Animation FX refused: Game Master rights required.");
			return;
		}
		string result;
		bool success = DCO_AIAnimationServer.Apply(targetId, animation, leaveWhenThreatened, result);
		Rpc(DCO_RpcAnimationFxConfirmed, success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcAnimationFxConfirmed(bool success, string result)
	{
		DCO_AIAnimationFxTool.Get().OnAuthorityResult(success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMTransform(RplId editableId, vector pos, vector anglesDeg, bool finalCommit)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM transform"))
			return;
		DCO_GMToolsServer.ApplyTransform(editableId, pos, anglesDeg, finalCommit);
	}


	void DCO_SendFpsSubscribe(bool on)
	{
		Rpc(DCO_RpcFpsSubscribe, on);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcFpsSubscribe(bool on)
	{
		if (on && !DCO_GMRights.Allow(GetPlayerId(), "FPS subscribe"))
			return;	// unsubscribe is always honoured: it only ever removes the caller's own viewer entry.
		DCO_FpsMonitorServer.Get().SubscribeGlobal(GetPlayerId(), on);
	}

	void DCO_SendFpsWatch(int targetPlayerId, bool on)
	{
		Rpc(DCO_RpcFpsWatch, targetPlayerId, on);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcFpsWatch(int targetPlayerId, bool on)
	{
		if (on && !DCO_GMRights.Allow(GetPlayerId(), "FPS watch"))
			return;	// unwatch is always honoured: it only ever removes the caller's own watch entry.
		DCO_FpsMonitorServer.Get().Watch(GetPlayerId(), targetPlayerId, on);
	}

	void DCO_SendFpsPoll()
	{
		Rpc(DCO_RpcFpsPoll);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcFpsPoll()
	{
		DCO_FpsMonitorClient.Get().BeginMeasure();
	}

	void DCO_SendFpsReport(int fps)
	{
		Rpc(DCO_RpcFpsReport, fps);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcFpsReport(int fps)
	{
		DCO_FpsMonitorServer.Get().Report(GetPlayerId(), fps);
	}

	void DCO_SendFpsSample(int playerId, int fps)
	{
		Rpc(DCO_RpcFpsSample, playerId, fps);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcFpsSample(int playerId, int fps)
	{
		DCO_FpsMonitorClient.Get().StoreSample(playerId, fps);
	}


	// Non-GM clients may use the same server-authoritative verbs only on their own body and only
	// while beside a replicated Arsenal Access binding. RELEASE remains available after deletion
	// or disconnect cleanup so a player can never be left movement-locked.
	protected bool DCO_CanUsePlayerArsenal(int verb, RplId targetId, bool allowRelease = false)
	{
		RplComponent replication = RplComponent.Cast(Replication.FindItem(targetId));
		IEntity controlled = GetControlledEntity();
		if (!replication || !controlled)
			return false;

		IEntity target = replication.GetEntity();
		if (verb == DCO_ArsenalServer.VERB_INSERT)
		{
			int guard;
			while (target && target.GetParent() && guard < 16)
			{
				target = target.GetParent();
				guard++;
			}
		}
		if (target != controlled)
			return false;
		return allowRelease || DCO_ArsenalAccessComponent.CanUseNearby(controlled);
	}

	// Fire an arsenal verb at the server. GM edits and player self-service share one mutation path.
	void DCO_SendGMArsenal(int verb, RplId targetId, string payload)
	{
		Rpc(DCO_RpcGMArsenal, verb, targetId, payload);
	}

	void DCO_SendGMArsenalAccessCreate(RplId targetId, vector interactionPosition, vector accentRgb, float panelOpacity)
	{
		Rpc(DCO_RpcGMArsenalAccessCreate, targetId, interactionPosition, accentRgb, panelOpacity);
	}

	void DCO_SendGMVehicleServiceAccessMove(RplId zoneId, vector interactionPosition)
	{
		Rpc(DCO_RpcGMVehicleServiceAccessMove, zoneId, interactionPosition);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMVehicleServiceAccessMove(RplId zoneId, vector interactionPosition)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "vehicle service access placement"))
		{
			Rpc(DCO_RpcGMVehicleServiceAccessConfirmed, false, "Vehicle Service access refused: Game Master rights required.");
			return;
		}
		string result;
		bool success = DCO_VehicleServiceAccessPlacement.ApplyRelayed(zoneId, interactionPosition, result);
		Rpc(DCO_RpcGMVehicleServiceAccessConfirmed, success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMVehicleServiceAccessConfirmed(bool success, string result)
	{
		DCO_VehicleServiceAccessPlacement.Get().OnAuthorityResult(success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMArsenalAccessCreate(RplId targetId, vector interactionPosition, vector accentRgb, float panelOpacity)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "arsenal access placement"))
		{
			Rpc(DCO_RpcGMArsenalAccessConfirmed, false, "Arsenal Access refused: Game Master rights required.");
			return;
		}
		string result;
		bool success = DCO_ArsenalAccessPlacement.ApplyRelayed(targetId, interactionPosition, accentRgb, panelOpacity, result);
		Rpc(DCO_RpcGMArsenalAccessConfirmed, success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMArsenalAccessConfirmed(bool success, string result)
	{
		DCO_ArsenalAccessPlacement.Get().OnAuthorityResult(success, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMArsenal(int verb, RplId targetId, string payload)
	{
		bool allowRelease = verb == DCO_ArsenalServer.VERB_RELEASE;
		if (!DCO_GMRights.IsGameMaster(GetPlayerId()) && !DCO_CanUsePlayerArsenal(verb, targetId, allowRelease))
		{
			Print(string.Format("[DCO-ARS] REFUSED player arsenal verb %1 from player %2", verb, GetPlayerId()), LogLevel.WARNING);
			return;
		}
		DCO_ArsenalServer.Apply(verb, targetId, payload);
	}


	void DCO_SendGMArsenalSnapshot(RplId targetId, int seq)
	{
		Rpc(DCO_RpcGMArsenalSnapshot, targetId, seq);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMArsenalSnapshot(RplId targetId, int seq)
	{
		if (!GRSA_ArsenalScenarioSettings.Get().m_bAllowKitChanges)
			return;
		if (!DCO_GMRights.IsGameMaster(GetPlayerId()) && !DCO_CanUsePlayerArsenal(0, targetId))
			return;	// Players can read only their own kit while actively using a placed arsenal.
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (!rpl)
			return;
		string json = DCO_ArsenalServer.SnapshotJson(rpl.GetEntity());
		Rpc(DCO_RpcGMArsenalSnapshotReply, seq, json);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMArsenalSnapshotReply(int seq, string json)
	{
		DCO_ArsenalLoadouts.Get().OnSnapshotReply(seq, json);
	}

	void DCO_SendGMArsenalApply(RplId targetId, string json)
	{
		Rpc(DCO_RpcGMArsenalApply, targetId, json);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMArsenalApply(RplId targetId, string json)
	{
		if (!GRSA_ArsenalScenarioSettings.Get().m_bAllowKitChanges)
			return;
		if (!DCO_GMRights.IsGameMaster(GetPlayerId()) && !DCO_CanUsePlayerArsenal(0, targetId))
			return;	// Players can write only their own kit while actively using a placed arsenal.
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (!rpl)
			return;
		DCO_ArsenalServer.ApplyLoadoutJson(rpl.GetEntity(), json);
	}

	// ARSENAL PLAYER LEG-LOCK leg.
	void DCO_SendArsenalLegLock(bool on)
	{
		Rpc(DCO_RpcArsenalLegLock, on);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcArsenalLegLock(bool on)
	{
		DCO_ArsenalLegLock.Apply(on);
	}
}
