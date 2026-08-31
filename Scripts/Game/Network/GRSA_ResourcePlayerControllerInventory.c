modded class SCR_ResourcePlayerControllerInventoryComponent
{
	protected static const int GRSA_MAX_KIT_JSON_LENGTH = 32768;
	protected static const int GRSA_CHUNK_LENGTH = 900;
	protected static const float GRSA_MAX_STATION_DISTANCE_SQ = 400;
	protected static const int GRSA_APPLY_COOLDOWN_MS = 2000;

	protected ref array<string> m_aGRSAChunks;
	protected int m_iGRSAExpectedChunks;
	protected RplId m_GRSAArsenalRplId;
	protected RplId m_GRSATargetRplId;
	protected int m_iGRSALastApplyTick;
	protected bool m_bGRSAApplyInProgress;

	protected static ref ScriptInvoker s_GRSAOnApplyResult;

	//------------------------------------------------------------------------------------------------
	//! (int status, int applied, int skipped, float suppliesCharged, string skippedSample)
	static ScriptInvoker GRSA_GetOnApplyResult()
	{
		if (!s_GRSAOnApplyResult)
			s_GRSAOnApplyResult = new ScriptInvoker();
		return s_GRSAOnApplyResult;
	}

	//------------------------------------------------------------------------------------------------
	//! Client entry point, streams the kit file JSON to the server in reliable ordered chunks.
	void GRSA_RequestApplyKit(notnull GRSA_KitFile kit, RplId arsenalRplId, RplId targetRplId)
	{
		string json = kit.GetRawJson();
		if (json.IsEmpty())
			json = kit.ExportToString();

		if (json.IsEmpty() || json.Length() > GRSA_MAX_KIT_JSON_LENGTH)
		{
			GRSA_GetOnApplyResult().Invoke(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		int length = json.Length();
		int total = (length + GRSA_CHUNK_LENGTH - 1) / GRSA_CHUNK_LENGTH;

		if (!targetRplId.IsValid())
		{
			GRSA_GetOnApplyResult().Invoke(GRSA_EApplyStatus.FAILED_NO_CHARACTER, 0, 0, 0, string.Empty);
			return;
		}

		Rpc(GRSA_RpcAsk_KitBegin, total, arsenalRplId, targetRplId);
		for (int i = 0; i < total; ++i)
		{
			int start = i * GRSA_CHUNK_LENGTH;
			int chunkLength = Math.Min(GRSA_CHUNK_LENGTH, length - start);
			Rpc(GRSA_RpcAsk_KitChunk, i, json.Substring(start, chunkLength));
		}
		Rpc(GRSA_RpcAsk_KitApply);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GRSA_RpcAsk_KitBegin(int chunkCount, RplId arsenalRplId, RplId targetRplId)
	{
		if (chunkCount <= 0 || chunkCount > (GRSA_MAX_KIT_JSON_LENGTH / GRSA_CHUNK_LENGTH) + 1 || !targetRplId.IsValid())
		{
			GRSA_ResetKitStream();
			return;
		}

		m_iGRSAExpectedChunks = chunkCount;
		m_GRSAArsenalRplId = arsenalRplId;
		m_GRSATargetRplId = targetRplId;
		m_aGRSAChunks = {};
		for (int i = 0; i < chunkCount; ++i)
		{
			m_aGRSAChunks.Insert(string.Empty);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GRSA_RpcAsk_KitChunk(int index, string chunk)
	{
		if (!m_aGRSAChunks || index < 0 || index >= m_aGRSAChunks.Count())
			return;
		if (chunk.IsEmpty() || chunk.Length() > GRSA_CHUNK_LENGTH)
		{
			GRSA_ResetKitStream();
			return;
		}

		m_aGRSAChunks[index] = chunk;
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void GRSA_RpcAsk_KitApply()
	{
		array<string> chunks = m_aGRSAChunks;
		int expected = m_iGRSAExpectedChunks;
		RplId arsenalRplId = m_GRSAArsenalRplId;
		RplId targetRplId = m_GRSATargetRplId;
		GRSA_ResetKitStream();

		if (!chunks || chunks.Count() != expected || expected <= 0)
		{
			GRSA_Log.Warn("Kit apply rejected: chunk stream incomplete");
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		if (m_bGRSAApplyInProgress)
		{
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		if (!GRSA_ArsenalScenarioSettings.Get().m_bAllowKitChanges)
		{
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		int nowTick = System.GetTickCount();
		if (m_iGRSALastApplyTick != 0 && nowTick - m_iGRSALastApplyTick < GRSA_APPLY_COOLDOWN_MS)
		{
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		string json;
		foreach (string chunk : chunks)
		{
			if (chunk.IsEmpty())
			{
				GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
				return;
			}
			json += chunk;
		}

		if (json.Length() > GRSA_MAX_KIT_JSON_LENGTH)
		{
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		GRSA_KitFile kit = new GRSA_KitFile();
		if (!kit.ImportFromString(json))
		{
			GRSA_Log.Warn("Kit apply rejected: json failed to parse");
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		if (!controller)
		{
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_NO_CHARACTER, 0, 0, 0, string.Empty);
			return;
		}

		RplComponent targetRpl = RplComponent.Cast(Replication.FindItem(targetRplId));
		GameEntity character;
		if (targetRpl)
			character = GameEntity.Cast(targetRpl.GetEntity());
		if (!character)
		{
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_NO_CHARACTER, 0, 0, 0, string.Empty);
			return;
		}

		IEntity controlled = controller.GetControlledEntity();
		bool isGameMaster = DCO_GMRights.IsGameMaster(controller.GetPlayerId());
		if (!isGameMaster && (character != controlled || !DCO_ArsenalAccessComponent.CanUseNearby(controlled)))
		{
			GRSA_Log.Warn(string.Format("Kit apply refused for player %1: no active Bifrost Arsenal Access or GM authority", controller.GetPlayerId()));
			GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
			return;
		}

		SCR_ArsenalComponent arsenal;
		if (arsenalRplId.IsValid())
		{
			arsenal = SCR_ArsenalComponent.Cast(Replication.FindItem(arsenalRplId));
			if (!arsenal)
			{
				GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
				return;
			}

			float distanceSq = vector.DistanceSq(arsenal.GetOwner().GetOrigin(), character.GetOrigin());
			if (distanceSq > GRSA_MAX_STATION_DISTANCE_SQ)
			{
				GRSA_Log.Warn(string.Format("Player %1 requested kit apply %2m from the station", controller.GetPlayerId(), Math.Sqrt(distanceSq)));
				GRSA_SendResult(GRSA_EApplyStatus.FAILED_INVALID, 0, 0, 0, string.Empty);
				return;
			}
		}

		GRSA_ApplyContext ctx = new GRSA_ApplyContext();
		ctx.m_Arsenal = arsenal;
		ctx.m_iPlayerId = controller.GetPlayerId();
		ctx.m_Config = GRSA_ResolveConfig(arsenal);

		m_bGRSAApplyInProgress = true;
		m_iGRSALastApplyTick = nowTick;
		DCO_ArsenalServer.PushUndo(character);
		GRSA_ApplyReport report = GRSA_ApplyService.ApplyKitFile(character, kit, ctx);
		m_bGRSAApplyInProgress = false;

		GRSA_SendResult(report.m_eStatus, report.m_iApplied, report.m_iSkipped, report.m_fSuppliesCharged, report.GetSkippedSample());
	}

	//------------------------------------------------------------------------------------------------
	protected void GRSA_ResetKitStream()
	{
		m_aGRSAChunks = null;
		m_iGRSAExpectedChunks = 0;
		m_GRSAArsenalRplId = RplId.Invalid();
		m_GRSATargetRplId = RplId.Invalid();
	}

	//------------------------------------------------------------------------------------------------
	protected GRSA_ArmoryConfig GRSA_ResolveConfig(SCR_ArsenalComponent arsenal)
	{
		return GRSA_ConfigHolder.GetDefault();
	}

	//------------------------------------------------------------------------------------------------
	protected void GRSA_SendResult(GRSA_EApplyStatus status, int applied, int skipped, float suppliesCharged, string skippedSample)
	{
		Rpc(GRSA_RpcDo_KitApplyResult, status, applied, skipped, suppliesCharged, skippedSample);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void GRSA_RpcDo_KitApplyResult(GRSA_EApplyStatus status, int applied, int skipped, float suppliesCharged, string skippedSample)
	{
		GRSA_GetOnApplyResult().Invoke(status, applied, skipped, suppliesCharged, skippedSample);
	}
}
