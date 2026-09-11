modded class SCR_ResourcePlayerControllerInventoryComponent
{
	protected static const int BIA_MAX_KIT_JSON_LENGTH = 32768;
	protected static const int BIA_CHUNK_LENGTH = 900;
	protected static const float BIA_MAX_STATION_DISTANCE_SQ = 400;
	protected static const int BIA_APPLY_COOLDOWN_MS = 2000;

	protected ref array<string> m_aBIAChunks;
	protected int m_iBIAExpectedChunks;
	protected RplId m_BIAArsenalRplId;
	protected RplId m_BIATargetRplId;
	protected int m_iBIALastApplyTick;
	protected bool m_bBIAApplyInProgress;

	protected static ref ScriptInvoker s_BIAOnApplyResult;

	//------------------------------------------------------------------------------------------------
	//! (int status, int applied, int skipped, float suppliesCharged, string skippedSample)
	static ScriptInvoker BIA_GetOnApplyResult()
	{
		if (!s_BIAOnApplyResult)
			s_BIAOnApplyResult = new ScriptInvoker();
		return s_BIAOnApplyResult;
	}

	//------------------------------------------------------------------------------------------------
	//! Client entry point, streams the kit file JSON to the server in reliable ordered chunks.
	void BIA_RequestApplyKit(notnull BIA_KitFile kit, RplId arsenalRplId, RplId targetRplId)
	{
		string json = kit.GetRawJson();
		if (json.IsEmpty())
			json = kit.ExportToString();

		if (json.IsEmpty() || json.Length() > BIA_MAX_KIT_JSON_LENGTH)
		{
			BIA_GetOnApplyResult().Invoke(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		int length = json.Length();
		int total = (length + BIA_CHUNK_LENGTH - 1) / BIA_CHUNK_LENGTH;

		if (!targetRplId.IsValid())
		{
			BIA_GetOnApplyResult().Invoke(BIA_EApplyStatus.FAILED_NO_CHARACTER, 0, 0, 0, string.Empty);
			return;
		}

		Rpc(BIA_RpcAsk_KitBegin, total, arsenalRplId, targetRplId);
		for (int i = 0; i < total; ++i)
		{
			int start = i * BIA_CHUNK_LENGTH;
			int chunkLength = Math.Min(BIA_CHUNK_LENGTH, length - start);
			Rpc(BIA_RpcAsk_KitChunk, i, json.Substring(start, chunkLength));
		}
		DCO_TestDiagnostics.Event("kit.request.send", string.Format("target=%1 chunks=%2", targetRplId, total));
		Rpc(BIA_RpcAsk_KitApply);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void BIA_RpcAsk_KitBegin(int chunkCount, RplId arsenalRplId, RplId targetRplId)
	{
		if (chunkCount <= 0 || chunkCount > (BIA_MAX_KIT_JSON_LENGTH / BIA_CHUNK_LENGTH) + 1 || !targetRplId.IsValid())
		{
			BIA_ResetKitStream();
			return;
		}

		m_iBIAExpectedChunks = chunkCount;
		m_BIAArsenalRplId = arsenalRplId;
		m_BIATargetRplId = targetRplId;
		m_aBIAChunks = {};
		for (int i = 0; i < chunkCount; ++i)
		{
			m_aBIAChunks.Insert(string.Empty);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void BIA_RpcAsk_KitChunk(int index, string chunk)
	{
		if (!m_aBIAChunks || index < 0 || index >= m_aBIAChunks.Count())
			return;
		if (chunk.IsEmpty() || chunk.Length() > BIA_CHUNK_LENGTH)
		{
			BIA_ResetKitStream();
			return;
		}

		m_aBIAChunks[index] = chunk;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void BIA_RpcAsk_KitApply()
	{
		DCO_TestDiagnostics.Event("kit.request.receive", string.Format("target=%1 chunks=%2", m_BIATargetRplId, m_iBIAExpectedChunks));
		array<string> chunks = m_aBIAChunks;
		int expected = m_iBIAExpectedChunks;
		RplId arsenalRplId = m_BIAArsenalRplId;
		RplId targetRplId = m_BIATargetRplId;
		BIA_ResetKitStream();

		if (!chunks || chunks.Count() != expected || expected <= 0)
		{
			BIA_Log.Warn("Kit apply rejected: chunk stream incomplete");
			BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		if (m_bBIAApplyInProgress)
		{
			BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		if (!BIA_ArsenalScenarioSettings.Get().m_bAllowKitChanges)
		{
			BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		int nowTick = System.GetTickCount();
		if (m_iBIALastApplyTick != 0 && nowTick - m_iBIALastApplyTick < BIA_APPLY_COOLDOWN_MS)
		{
			BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		string json;
		foreach (string chunk : chunks)
		{
			if (chunk.IsEmpty())
			{
				BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
				return;
			}
			json += chunk;
		}

		if (json.Length() > BIA_MAX_KIT_JSON_LENGTH)
		{
			BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		BIA_KitFile kit = new BIA_KitFile();
		if (!kit.ImportFromString(json))
		{
			BIA_Log.Warn("Kit apply rejected: json failed to parse");
			BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		if (!controller)
		{
			BIA_SendResult(BIA_EApplyStatus.FAILED_NO_CHARACTER, 0, 0, 0, string.Empty);
			return;
		}

		RplComponent targetRpl = RplComponent.Cast(Replication.FindItem(targetRplId));
		GameEntity character;
		if (targetRpl)
			character = GameEntity.Cast(targetRpl.GetEntity());
		if (!character)
		{
			BIA_SendResult(BIA_EApplyStatus.FAILED_NO_CHARACTER, 0, 0, 0, string.Empty);
			return;
		}

		IEntity controlled = controller.GetControlledEntity();
		bool isGameMaster = DCO_GMRights.IsGameMaster(controller.GetPlayerId());
		if (!isGameMaster && (character != controlled || !DCO_ArsenalAccessComponent.CanUseNearby(controlled)))
		{
			BIA_Log.Warn(string.Format("Kit apply refused for player %1: no active Bifrost Arsenal Access or GM authority", controller.GetPlayerId()));
			BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		SCR_ArsenalComponent arsenal;
		if (arsenalRplId.IsValid())
		{
			arsenal = SCR_ArsenalComponent.Cast(Replication.FindItem(arsenalRplId));
			if (!arsenal)
			{
				BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
				return;
			}

			float distanceSq = vector.DistanceSq(arsenal.GetOwner().GetOrigin(), character.GetOrigin());
			if (distanceSq > BIA_MAX_STATION_DISTANCE_SQ)
			{
				BIA_Log.Warn(string.Format("Player %1 requested kit apply %2m from the station", controller.GetPlayerId(), Math.Sqrt(distanceSq)));
				BIA_SendResult(BIA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
				return;
			}
		}

		BIA_ApplyContext ctx = new BIA_ApplyContext();
		ctx.m_Arsenal = arsenal;
		ctx.m_iPlayerId = controller.GetPlayerId();
		ctx.m_Config = BIA_ResolveConfig(arsenal);

		m_bBIAApplyInProgress = true;
		m_iBIALastApplyTick = nowTick;
		DCO_ArsenalServer.PushUndo(character);
		BIA_ApplyReport report = BIA_ApplyService.ApplyKitFile(character, kit, ctx);
		m_bBIAApplyInProgress = false;

		BIA_SendResult(report.m_eStatus, report.m_iApplied, report.m_iSkipped, report.m_fSuppliesCharged, report.GetSkippedSample());
	}

	//------------------------------------------------------------------------------------------------
	protected void BIA_ResetKitStream()
	{
		m_aBIAChunks = null;
		m_iBIAExpectedChunks = 0;
		m_BIAArsenalRplId = RplId.Invalid();
		m_BIATargetRplId = RplId.Invalid();
	}

	//------------------------------------------------------------------------------------------------
	protected BIA_ArmoryConfig BIA_ResolveConfig(SCR_ArsenalComponent arsenal)
	{
		return BIA_ConfigHolder.GetDefault();
	}

	//------------------------------------------------------------------------------------------------
	protected void BIA_SendResult(BIA_EApplyStatus status, int applied, int skipped, float suppliesCharged, string skippedSample)
	{
		DCO_TestDiagnostics.Event("kit.result.server", string.Format("status=%1 applied=%2 skipped=%3", status, applied, skipped), status != BIA_EApplyStatus.SUCCESS);
		Rpc(BIA_RpcDo_KitApplyResult, status, applied, skipped, suppliesCharged, skippedSample);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void BIA_RpcDo_KitApplyResult(BIA_EApplyStatus status, int applied, int skipped, float suppliesCharged, string skippedSample)
	{
		DCO_TestDiagnostics.Event("kit.result.client", string.Format("status=%1 applied=%2 skipped=%3", status, applied, skipped), status != BIA_EApplyStatus.SUCCESS);
		BIA_GetOnApplyResult().Invoke(status, applied, skipped, suppliesCharged, skippedSample);
	}
}
