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

	// Precise-mode POSING legs.
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

	// Server side: resolve the wire id back to the entity and apply.
	static bool Apply(int toolId, RplId targetId, vector pos, out bool confirmedState)
	{
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
		if (!target)
			return;
		int ord = Math.Round(pos[0]);	// posing legs carry their ordinal in pos[0] (see the TOOL_* block above).
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


	// Client-side entry: write the object's transform on the authority.
	static void RouteTransform(IEntity target, vector pos, vector anglesDeg)
	{
		if (!target)
			return;

		if (Replication.IsServer())
		{
			SetEntityTransform(target, pos, anglesDeg);	// listen/SP: proven direct path.
			return;
		}

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return;
		RplComponent rpl = RplComponent.Cast(target.FindComponent(RplComponent));
		if (!rpl || !rpl.Id().IsValid())
			return;	// local-only entity has nothing to move on the server.
		pc.DCO_SendGMTransform(rpl.Id(), pos, anglesDeg);
	}

	// Server side: resolve the wire id and apply the transform.
	static void ApplyTransform(RplId targetId, vector pos, vector anglesDeg)
	{
		RplComponent rpl = RplComponent.Cast(Replication.FindItem(targetId));
		if (!rpl)
			return;
		SetEntityTransform(rpl.GetEntity(), pos, anglesDeg);
	}

	static void SetEntityTransform(IEntity target, vector pos, vector anglesDeg)
	{
		if (!target)
			return;
		vector rot[3];
		Math3D.AnglesToMatrix(anglesDeg, rot);
		vector mat[4];
		mat[0] = rot[0];
		mat[1] = rot[1];
		mat[2] = rot[2];
		mat[3] = pos;
		float sc = target.GetScale();
		BaseGameEntity bge = BaseGameEntity.Cast(target);
		if (bge)
			bge.Teleport(mat);
		else
			target.SetWorldTransform(mat);
		target.SetScale(sc);
		Physics ph = target.GetPhysics();
		if (ph)
			ph.SetVelocity(vector.Zero);
		target.Update();

		DCO_GMTools.Get().ReanchorFrozen(target);
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
			m_bDCO_PauseState = DCO_GMPauseCore.Get().IsActive();
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
		Print(string.Format("[DCO-GM] tool authority ACK: tool=%1 target=%2 state=%3", toolId, targetId, confirmedState), LogLevel.NORMAL);
	}

	// Relays gameplay pause and clock controls.
	void DCO_SendGMPause(int scope, int aspectMask, bool on, RplId selectedTargetId)
	{
		Rpc(DCO_RpcGMPause, scope, aspectMask, on, selectedTargetId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMPause(int scope, int aspectMask, bool on, RplId selectedTargetId)
	{
		if (on && !DCO_GMRights.Allow(GetPlayerId(), "GM pause"))
			return;
		int frozenCount = DCO_GMPauseServer.ApplyPause(scope, aspectMask, on, selectedTargetId);
		Rpc(DCO_RpcGMPauseConfirmed, frozenCount > 0, frozenCount);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcGMPauseConfirmed(bool paused, int frozenCount)
	{
		DCO_GMPausePresentationState.SetConfirmed(paused, frozenCount);
		Print(string.Format("[DCO-GM] pause authority ACK: active=%1 frozen=%2", paused, frozenCount), LogLevel.NORMAL);
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
		DCO_GMAttach.DetachAllOnAuthority(GetPlayerId());
	}

	void DCO_RequestGMAIOverlay(vector cameraPosition, int requestMask, string selectedIds)
	{
		Rpc(DCO_RpcRequestGMAIOverlay, cameraPosition, requestMask, selectedIds);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void DCO_RpcRequestGMAIOverlay(vector cameraPosition, int requestMask, string selectedIds)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM AI overlay"))
			return;
		m_iDCO_AIOverlaySerial++;
		array<string> chunks = {};
		DCO_GMAIOverlaySnapshot.BuildChunks(cameraPosition, requestMask, selectedIds, m_iDCO_AIOverlaySerial, chunks);
		foreach (string chunk : chunks)
			Rpc(DCO_RpcReceiveGMAIOverlay, chunk);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Owner)]
	protected void DCO_RpcReceiveGMAIOverlay(string payload)
	{
		DCO_GMAIOverlaySnapshot.Receive(payload);
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

	void DCO_SendGMTransform(RplId targetId, vector pos, vector anglesDeg)
	{
		Rpc(DCO_RpcGMTransform, targetId, pos, anglesDeg);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMTransform(RplId targetId, vector pos, vector anglesDeg)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "GM transform"))
			return;
		DCO_GMToolsServer.ApplyTransform(targetId, pos, anglesDeg);
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


	// Fire a GM arsenal verb at the server.
	void DCO_SendGMArsenal(int verb, RplId targetId, string payload)
	{
		Rpc(DCO_RpcGMArsenal, verb, targetId, payload);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMArsenal(int verb, RplId targetId, string payload)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "arsenal verb"))
			return;
		DCO_ArsenalServer.Apply(verb, targetId, payload);
	}


	void DCO_SendGMArsenalSnapshot(RplId targetId, int seq)
	{
		Rpc(DCO_RpcGMArsenalSnapshot, targetId, seq);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcGMArsenalSnapshot(RplId targetId, int seq)
	{
		if (!DCO_GMRights.Allow(GetPlayerId(), "arsenal snapshot"))
			return;	// a loadout snapshot is another player's full kit: never readable by a non-GM.
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
		if (!DCO_GMRights.Allow(GetPlayerId(), "arsenal apply"))
			return;	// a whole-loadout write onto any character: GM-only, same gate as the snapshot leg.
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
