class DCO_JsonConfig
{
	static const string FILE_PATH = "$profile:DCO_Settings.json";

	// Apply the server JSON onto the settings singleton.
	static void LoadInto(DCO_MoraleSettings cfg)
	{
		if (!cfg)
			return;

		if (!FileIO.FileExists(FILE_PATH))
		{
			// No file yet: on the server, deploy a populated default into the profile dir.
			if (Replication.IsServer())
				WriteDefaults(cfg);
			return;
		}

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(FILE_PATH))
		{
			Print("[DCO] DCO_Settings.json present but failed to parse - keeping defaults: " + FILE_PATH, LogLevel.WARNING);
			return;	// present but unparseable - keep defaults rather than half-applying.
		}

		ApplyKeys(ctx, cfg);
		Print("[DCO] Loaded server settings override from " + FILE_PATH, LogLevel.NORMAL);
	}

	protected static void ApplyKeys(JsonLoadContext ctx, DCO_MoraleSettings cfg)
	{
		ApplyMoraleKeys(ctx, cfg);
		ApplyBehaviorKeys(ctx, cfg);
		ApplyTacticalMoveKeys(ctx);
		ApplyFormationKeys(ctx, cfg);
	}

	protected static void ApplyMoraleKeys(JsonLoadContext ctx, DCO_MoraleSettings cfg)
	{
		// --- Core morale ---.
		ctx.ReadValue("m_bEnabled", cfg.m_bEnabled);
		ctx.ReadValue("m_bDebug", cfg.m_bDebug);
		ctx.ReadValue("m_fUpdateIntervalSec", cfg.m_fUpdateIntervalSec);
		ctx.ReadValue("m_fFleeThreshold", cfg.m_fFleeThreshold);
		ctx.ReadValue("m_fRallyThreshold", cfg.m_fRallyThreshold);
		ctx.ReadValue("m_fRecoveryPerTick", cfg.m_fRecoveryPerTick);
		ctx.ReadValue("m_fDrainPerTick", cfg.m_fDrainPerTick);
		ctx.ReadValue("m_fCasualtyWeight", cfg.m_fCasualtyWeight);
		ctx.ReadValue("m_fHeavyThreat", cfg.m_fHeavyThreat);
		ctx.ReadValue("m_fFleeChancePerTick", cfg.m_fFleeChancePerTick);
		ctx.ReadValue("m_fMoraleLostPerCasualty", cfg.m_fMoraleLostPerCasualty);
		ctx.ReadValue("m_fMoraleLostOnLeaderLoss", cfg.m_fMoraleLostOnLeaderLoss);
		ctx.ReadValue("m_fFleeDistance", cfg.m_fFleeDistance);

		// --- Stance cooldown ---.
		ctx.ReadValue("m_bEnableStanceCooldown", cfg.m_bEnableStanceCooldown);
		ctx.ReadValue("m_fStanceCooldownSec", cfg.m_fStanceCooldownSec);
		ctx.ReadValue("m_bStanceCooldownInCover", cfg.m_bStanceCooldownInCover);
		ctx.ReadValue("m_bStanceCooldownDebug", cfg.m_bStanceCooldownDebug);

		// --- Panic ---.
		ctx.ReadValue("m_bEnablePanic", cfg.m_bEnablePanic);
		ctx.ReadValue("m_fPanicBand", cfg.m_fPanicBand);
		ctx.ReadValue("m_fPanicChancePerTick", cfg.m_fPanicChancePerTick);
		ctx.ReadValue("m_fPanicDurationSec", cfg.m_fPanicDurationSec);
		ctx.ReadValue("m_fPanicCooldownSec", cfg.m_fPanicCooldownSec);

		// --- Morale -> accuracy ---.
		ctx.ReadValue("m_bEnableMoraleAccuracy", cfg.m_bEnableMoraleAccuracy);
		ctx.ReadValue("m_fAccuracyRookieMorale", cfg.m_fAccuracyRookieMorale);
		ctx.ReadValue("m_fAccuracyRegularMorale", cfg.m_fAccuracyRegularMorale);
		ctx.ReadValue("m_fLowMoraleFireRateCoef", cfg.m_fLowMoraleFireRateCoef);
	}

	protected static void ApplyBehaviorKeys(JsonLoadContext ctx, DCO_MoraleSettings cfg)
	{
		// --- Vision limit / idle / cover ---.
		ctx.ReadValue("m_bEnableVisionLimit", cfg.m_bEnableVisionLimit);
		ctx.ReadValue("m_fNightPerceptionFactor", cfg.m_fNightPerceptionFactor);
		ctx.ReadValue("m_fVisionSunriseHour", cfg.m_fVisionSunriseHour);
		ctx.ReadValue("m_fVisionSunsetHour", cfg.m_fVisionSunsetHour);
		ctx.ReadValue("m_fVisionTwilightHours", cfg.m_fVisionTwilightHours);
		ctx.ReadValue("m_fVisionCheckSec", cfg.m_fVisionCheckSec);
		ctx.ReadValue("m_bEnableIdleBehaviour", cfg.m_bEnableIdleBehaviour);
		ctx.ReadValue("m_fIdleDelaySec", cfg.m_fIdleDelaySec);
		ctx.ReadValue("m_fIdleMoveEpsilon", cfg.m_fIdleMoveEpsilon);
		ctx.ReadValue("m_fIdleSpreadRadius", cfg.m_fIdleSpreadRadius);
		ctx.ReadValue("m_fIdlePatrolRadius", cfg.m_fIdlePatrolRadius);
		ctx.ReadValue("m_fIdlePatrolIntervalSec", cfg.m_fIdlePatrolIntervalSec);
		ctx.ReadValue("m_fIdlePatrolChance", cfg.m_fIdlePatrolChance);
		ctx.ReadValue("m_bEnableCoverStance", cfg.m_bEnableCoverStance);
		ctx.ReadValue("m_fCoverStanceCheckSec", cfg.m_fCoverStanceCheckSec);
		ctx.ReadValue("m_fCoverStanceMaxRange", cfg.m_fCoverStanceMaxRange);
		ctx.ReadValue("m_fCoverStandEye", cfg.m_fCoverStandEye);
		ctx.ReadValue("m_fCoverCrouchEye", cfg.m_fCoverCrouchEye);
		ctx.ReadValue("m_fCoverProneEye", cfg.m_fCoverProneEye);
		ctx.ReadValue("m_bEnableMemberMorale", cfg.m_bEnableMemberMorale);
		ctx.ReadValue("m_fMemberMoraleCheckSec", cfg.m_fMemberMoraleCheckSec);
		ctx.ReadValue("m_fMemberMoraleSuppressionWeight", cfg.m_fMemberMoraleSuppressionWeight);
		ctx.ReadValue("m_fMemberMoraleHesitateThreshold", cfg.m_fMemberMoraleHesitateThreshold);
		ctx.ReadValue("m_fMemberMoraleHesitateSec", cfg.m_fMemberMoraleHesitateSec);
		ctx.ReadValue("m_bMemberMoraleCower", cfg.m_bMemberMoraleCower);
		ctx.ReadValue("m_fMemberMoraleCowerThreshold", cfg.m_fMemberMoraleCowerThreshold);
		ctx.ReadValue("m_bEnableEmergencyRearm", cfg.m_bEnableEmergencyRearm);
		ctx.ReadValue("m_fRearmCheckSec", cfg.m_fRearmCheckSec);
		ctx.ReadValue("m_iRearmMagazineCount", cfg.m_iRearmMagazineCount);
		ctx.ReadValue("m_bRearmScavenge", cfg.m_bRearmScavenge);
		ctx.ReadValue("m_fRearmScavengeRange", cfg.m_fRearmScavengeRange);
		ctx.ReadValue("m_fRearmPickupDist", cfg.m_fRearmPickupDist);
		ctx.ReadValue("m_bEnableCQB", cfg.m_bEnableCQB);
		ctx.ReadValue("m_fCqbCheckSec", cfg.m_fCqbCheckSec);
		ctx.ReadValue("m_fCqbEngageRange", cfg.m_fCqbEngageRange);
		ctx.ReadValue("m_fCqbBuildingScan", cfg.m_fCqbBuildingScan);
		ctx.ReadValue("m_fCqbArriveDist", cfg.m_fCqbArriveDist);
		ctx.ReadValue("m_fCqbReorderDist", cfg.m_fCqbReorderDist);
		ctx.ReadValue("m_bEnableCqbClear", cfg.m_bEnableCqbClear);
		ctx.ReadValue("m_bCqbClearMarkedOnly", cfg.m_bCqbClearMarkedOnly);
		ctx.ReadValue("m_fCqbClearRadius", cfg.m_fCqbClearRadius);
		ctx.ReadValue("m_fCqbNodeDwellSec", cfg.m_fCqbNodeDwellSec);
		ctx.ReadValue("m_fCqbStackSpacing", cfg.m_fCqbStackSpacing);
		ctx.ReadValue("m_iCqbMaxBuildings", cfg.m_iCqbMaxBuildings);
		ctx.ReadValue("m_fCqbClearCheckSec", cfg.m_fCqbClearCheckSec);
		ctx.ReadValue("m_fCqbReorderDistClear", cfg.m_fCqbReorderDistClear);
		ctx.ReadValue("m_fCqbApproachTimeoutSec", cfg.m_fCqbApproachTimeoutSec);
		ctx.ReadValue("m_fCqbPerceptionBoost", cfg.m_fCqbPerceptionBoost);
		ctx.ReadValue("m_bDebugCqbClear", cfg.m_bDebugCqbClear);
		ctx.ReadValue("m_bEnableMachineGunner", cfg.m_bEnableMachineGunner);
		ctx.ReadValue("m_fMGCheckSec", cfg.m_fMGCheckSec);
		ctx.ReadValue("m_bMGReposition", cfg.m_bMGReposition);
		ctx.ReadValue("m_fMGRepositionRadius", cfg.m_fMGRepositionRadius);
		ctx.ReadValue("m_fMGSightHeight", cfg.m_fMGSightHeight);
		ctx.ReadValue("m_fMGEngageRange", cfg.m_fMGEngageRange);
		ctx.ReadValue("m_bEnableFleeSmoke", cfg.m_bEnableFleeSmoke);

		// --- Fratricide / straggler merge / suppression / contagion ---.
		ctx.ReadValue("m_bEnableSuppression", cfg.m_bEnableSuppression);
		ctx.ReadValue("m_fSuppressionCheckSec", cfg.m_fSuppressionCheckSec);
		ctx.ReadValue("m_fSuppressionThreshold", cfg.m_fSuppressionThreshold);
		ctx.ReadValue("m_fSuppressedFireRateCoef", cfg.m_fSuppressedFireRateCoef);
		ctx.ReadValue("m_bSuppressionSeekCover", cfg.m_bSuppressionSeekCover);
		ctx.ReadValue("m_fSuppressionCoverRadius", cfg.m_fSuppressionCoverRadius);
		ctx.ReadValue("m_fSuppressionSmokeFraction", cfg.m_fSuppressionSmokeFraction);
		ctx.ReadValue("m_bSuppressionDigIn", cfg.m_bSuppressionDigIn);
		ctx.ReadValue("m_fSuppressionDashMax", cfg.m_fSuppressionDashMax);
		ctx.ReadValue("m_fSuppressionMaxDescend", cfg.m_fSuppressionMaxDescend);
		ctx.ReadValue("m_bEnableMoraleContagion", cfg.m_bEnableMoraleContagion);
		ctx.ReadValue("m_fContagionRadius", cfg.m_fContagionRadius);
		ctx.ReadValue("m_fContagionCheckSec", cfg.m_fContagionCheckSec);
		ctx.ReadValue("m_fContagionMoralePerBrokenGroup", cfg.m_fContagionMoralePerBrokenGroup);
		ctx.ReadValue("m_fContagionMaxLossPerTick", cfg.m_fContagionMaxLossPerTick);
		ctx.ReadValue("m_bEnableFriendlyFire", cfg.m_bEnableFriendlyFire);
		ctx.ReadValue("m_fFriendlyFireHold", cfg.m_fFriendlyFireHold);
		ctx.ReadValue("m_fFriendlyLaneCheckSec", cfg.m_fFriendlyLaneCheckSec);
		ctx.ReadValue("m_bEnableStragglerMerge", cfg.m_bEnableStragglerMerge);
		ctx.ReadValue("m_iStragglerSize", cfg.m_iStragglerSize);
		ctx.ReadValue("m_fMergeRadius", cfg.m_fMergeRadius);
		ctx.ReadValue("m_fMergeCheckSec", cfg.m_fMergeCheckSec);

		// --- QRF garrison ---.
		ctx.ReadValue("m_bClearWaypointOnOverride", cfg.m_bClearWaypointOnOverride);
		ctx.ReadValue("m_fQRFCriticalMorale", cfg.m_fQRFCriticalMorale);
		ctx.ReadValue("m_fQRFHoldLeash", cfg.m_fQRFHoldLeash);

		// --- Hide from armour ---.
		ctx.ReadValue("m_bEnableFleeFromArmor", cfg.m_bEnableFleeFromArmor);
		ctx.ReadValue("m_fFleeFromArmorRange", cfg.m_fFleeFromArmorRange);
		ctx.ReadValue("m_fFleeFromArmorCheckSec", cfg.m_fFleeFromArmorCheckSec);
	}

	protected static void ApplyTacticalMoveKeys(JsonLoadContext ctx)
	{
		DCO_TacticalMoveSettings tmove = DCO_TacticalMoveSettings.Get();
		if (tmove)
		{
			ctx.ReadValue("m_bEnableTacticalMove", tmove.m_bEnableTacticalMove);
			ctx.ReadValue("m_fMinMoveDistance", tmove.m_fMinMoveDistance);
			ctx.ReadValue("m_fCheckInInterval", tmove.m_fCheckInInterval);
			ctx.ReadValue("m_fObservePause", tmove.m_fObservePause);
			ctx.ReadValue("m_bAvoidThreatFunnel", tmove.m_bAvoidThreatFunnel);
			ctx.ReadValue("m_fFunnelSidestep", tmove.m_fFunnelSidestep);
			ctx.ReadValue("m_fFunnelCorridorHalfWidth", tmove.m_fFunnelCorridorHalfWidth);
			ctx.ReadValue("m_bEnableFlanking", tmove.m_bEnableFlanking);
			ctx.ReadValue("m_fFlankAngleDeg", tmove.m_fFlankAngleDeg);
			ctx.ReadValue("m_fFlankMaxEnemyDist", tmove.m_fFlankMaxEnemyDist);
			ctx.ReadValue("m_fFlankMinEnemyDist", tmove.m_fFlankMinEnemyDist);
			ctx.ReadValue("m_bEnableExposureScoring", tmove.m_bEnableExposureScoring);
			ctx.ReadValue("m_iExposureLayerMask", tmove.m_iExposureLayerMask);
			ctx.ReadValue("m_fExposureSidestep", tmove.m_fExposureSidestep);
			ctx.ReadValue("m_bEnableProceduralPath", tmove.m_bEnableProceduralPath);
			ctx.ReadValue("m_bPathDebugDraw", tmove.m_bPathDebugDraw);
			ctx.ReadValue("m_fPathReassessSec", tmove.m_fPathReassessSec);
			ctx.ReadValue("m_fPathLegLength", tmove.m_fPathLegLength);
			ctx.ReadValue("m_fPathConeHalfAngleDeg", tmove.m_fPathConeHalfAngleDeg);
			ctx.ReadValue("m_iPathConeSamples", tmove.m_iPathConeSamples);
			ctx.ReadValue("m_fPathArriveDist", tmove.m_fPathArriveDist);
			ctx.ReadValue("m_iPathMaxLegs", tmove.m_iPathMaxLegs);
			ctx.ReadValue("m_fPathThreatActivation", tmove.m_fPathThreatActivation);
			ctx.ReadValue("m_fPathConcealmentBonus", tmove.m_fPathConcealmentBonus);
			ctx.ReadValue("m_fPathStraightBias", tmove.m_fPathStraightBias);
			ctx.ReadValue("m_bEnableBaseOfFireSplit", tmove.m_bEnableBaseOfFireSplit);
			ctx.ReadValue("m_fPathSplitThreatActivation", tmove.m_fPathSplitThreatActivation);
			ctx.ReadValue("m_bPathHaltOnContact", tmove.m_bPathHaltOnContact);
			ctx.ReadValue("m_fPathContactHaltRange", tmove.m_fPathContactHaltRange);
			ctx.ReadValue("m_bPathLeaderInBase", tmove.m_bPathLeaderInBase);
			ctx.ReadValue("m_bPathBaseMaintainLos", tmove.m_bPathBaseMaintainLos);
			ctx.ReadValue("m_fPathBaseLosRadius", tmove.m_fPathBaseLosRadius);
			ctx.ReadValue("m_bEnableTravelingOverwatch", tmove.m_bEnableTravelingOverwatch);
			ctx.ReadValue("m_fPathTravelOverwatchThreat", tmove.m_fPathTravelOverwatchThreat);
			ctx.ReadValue("m_bEnableCoverSeek", tmove.m_bEnableCoverSeek);
			ctx.ReadValue("m_fCoverSeekCheckSec", tmove.m_fCoverSeekCheckSec);
			ctx.ReadValue("m_fCoverSeekRadius", tmove.m_fCoverSeekRadius);
			ctx.ReadValue("m_iCoverSeekSamples", tmove.m_iCoverSeekSamples);
			ctx.ReadValue("m_fCoverMaxClimb", tmove.m_fCoverMaxClimb);
			ctx.ReadValue("m_bEnableReactToContact", tmove.m_bEnableReactToContact);
			ctx.ReadValue("m_fReactCheckSec", tmove.m_fReactCheckSec);
			ctx.ReadValue("m_fSupportFireRateCoef", tmove.m_fSupportFireRateCoef);
			ctx.ReadValue("m_fAssaultFlankAngleDeg", tmove.m_fAssaultFlankAngleDeg);
			ctx.ReadValue("m_bEnableStandoff", tmove.m_bEnableStandoff);
			ctx.ReadValue("m_fStandoffRifle", tmove.m_fStandoffRifle);
			ctx.ReadValue("m_fStandoffMG", tmove.m_fStandoffMG);
			ctx.ReadValue("m_fStandoffSniper", tmove.m_fStandoffSniper);
			ctx.ReadValue("m_fStandoffLauncher", tmove.m_fStandoffLauncher);
			ctx.ReadValue("m_fStandoffPistol", tmove.m_fStandoffPistol);
			ctx.ReadValue("m_fStandoffBand", tmove.m_fStandoffBand);
			ctx.ReadValue("m_fBreakContactDistance", tmove.m_fBreakContactDistance);
			ctx.ReadValue("m_bEnableTacticalBrain", tmove.m_bEnableTacticalBrain);
			ctx.ReadValue("m_fBrainCheckSec", tmove.m_fBrainCheckSec);
			ctx.ReadValue("m_fBrainAssaultPowerRatio", tmove.m_fBrainAssaultPowerRatio);
			ctx.ReadValue("m_fBrainBreakPowerRatio", tmove.m_fBrainBreakPowerRatio);
			ctx.ReadValue("m_fBrainEnemyScanRadius", tmove.m_fBrainEnemyScanRadius);
			ctx.ReadValue("m_bEnableDcoFormations", tmove.m_bEnableDcoFormations);
			ctx.ReadValue("m_fFormationShapeCheckSec", tmove.m_fFormationShapeCheckSec);
			ctx.ReadValue("m_fFormationSpacing", tmove.m_fFormationSpacing);
			ctx.ReadValue("m_iFormationTravelShape", tmove.m_iFormationTravelShape);
			ctx.ReadValue("m_iFormationContactShape", tmove.m_iFormationContactShape);
		}
	}

	protected static void ApplyFormationKeys(JsonLoadContext ctx, DCO_MoraleSettings cfg)
	{
		// --- Adaptive formation ---.
		ctx.ReadValue("m_bEnableAdaptiveFormation", cfg.m_bEnableAdaptiveFormation);
		ctx.ReadValue("m_fFormationCheckSec", cfg.m_fFormationCheckSec);
		ctx.ReadValue("m_sFormationTravel", cfg.m_sFormationTravel);
		ctx.ReadValue("m_sFormationContact", cfg.m_sFormationContact);
	}

	protected static void WriteDefaults(DCO_MoraleSettings cfg)
	{
		JsonSaveContext ctx = new JsonSaveContext();
		WriteMoraleKeys(ctx, cfg);
		WriteBehaviorKeys(ctx, cfg);
		WriteTacticalMoveKeys(ctx);
		WriteFormationKeys(ctx, cfg);
		WriteSubsystemKeys(ctx);

		if (ctx.SaveToFile(FILE_PATH))
			Print("[DCO] No server config found - wrote default DCO_Settings.json to " + FILE_PATH, LogLevel.NORMAL);
		else
			Print("[DCO] Failed to write default DCO_Settings.json to " + FILE_PATH, LogLevel.WARNING);
	}

	protected static void WriteMoraleKeys(JsonSaveContext ctx, DCO_MoraleSettings cfg)
	{
		// --- Core morale ---.
		ctx.WriteValue("m_bEnabled", cfg.m_bEnabled);
		ctx.WriteValue("m_bDebug", cfg.m_bDebug);
		ctx.WriteValue("m_fUpdateIntervalSec", cfg.m_fUpdateIntervalSec);
		ctx.WriteValue("m_fFleeThreshold", cfg.m_fFleeThreshold);
		ctx.WriteValue("m_fRallyThreshold", cfg.m_fRallyThreshold);
		ctx.WriteValue("m_fRecoveryPerTick", cfg.m_fRecoveryPerTick);
		ctx.WriteValue("m_fDrainPerTick", cfg.m_fDrainPerTick);
		ctx.WriteValue("m_fCasualtyWeight", cfg.m_fCasualtyWeight);
		ctx.WriteValue("m_fHeavyThreat", cfg.m_fHeavyThreat);
		ctx.WriteValue("m_fFleeChancePerTick", cfg.m_fFleeChancePerTick);
		ctx.WriteValue("m_fMoraleLostPerCasualty", cfg.m_fMoraleLostPerCasualty);
		ctx.WriteValue("m_fMoraleLostOnLeaderLoss", cfg.m_fMoraleLostOnLeaderLoss);
		ctx.WriteValue("m_fFleeDistance", cfg.m_fFleeDistance);

		// --- Stance cooldown ---.
		ctx.WriteValue("m_bEnableStanceCooldown", cfg.m_bEnableStanceCooldown);
		ctx.WriteValue("m_fStanceCooldownSec", cfg.m_fStanceCooldownSec);
		ctx.WriteValue("m_bStanceCooldownInCover", cfg.m_bStanceCooldownInCover);
		ctx.WriteValue("m_bStanceCooldownDebug", cfg.m_bStanceCooldownDebug);

		// --- Panic ---.
		ctx.WriteValue("m_bEnablePanic", cfg.m_bEnablePanic);
		ctx.WriteValue("m_fPanicBand", cfg.m_fPanicBand);
		ctx.WriteValue("m_fPanicChancePerTick", cfg.m_fPanicChancePerTick);
		ctx.WriteValue("m_fPanicDurationSec", cfg.m_fPanicDurationSec);
		ctx.WriteValue("m_fPanicCooldownSec", cfg.m_fPanicCooldownSec);

		// --- Morale -> accuracy ---.
		ctx.WriteValue("m_bEnableMoraleAccuracy", cfg.m_bEnableMoraleAccuracy);
		ctx.WriteValue("m_fAccuracyRookieMorale", cfg.m_fAccuracyRookieMorale);
		ctx.WriteValue("m_fAccuracyRegularMorale", cfg.m_fAccuracyRegularMorale);
		ctx.WriteValue("m_fLowMoraleFireRateCoef", cfg.m_fLowMoraleFireRateCoef);
	}

	protected static void WriteBehaviorKeys(JsonSaveContext ctx, DCO_MoraleSettings cfg)
	{
		// --- Vision limit / idle / cover ---.
		ctx.WriteValue("m_bEnableVisionLimit", cfg.m_bEnableVisionLimit);
		ctx.WriteValue("m_fNightPerceptionFactor", cfg.m_fNightPerceptionFactor);
		ctx.WriteValue("m_fVisionSunriseHour", cfg.m_fVisionSunriseHour);
		ctx.WriteValue("m_fVisionSunsetHour", cfg.m_fVisionSunsetHour);
		ctx.WriteValue("m_fVisionTwilightHours", cfg.m_fVisionTwilightHours);
		ctx.WriteValue("m_fVisionCheckSec", cfg.m_fVisionCheckSec);
		ctx.WriteValue("m_bEnableIdleBehaviour", cfg.m_bEnableIdleBehaviour);
		ctx.WriteValue("m_fIdleDelaySec", cfg.m_fIdleDelaySec);
		ctx.WriteValue("m_fIdleMoveEpsilon", cfg.m_fIdleMoveEpsilon);
		ctx.WriteValue("m_fIdleSpreadRadius", cfg.m_fIdleSpreadRadius);
		ctx.WriteValue("m_fIdlePatrolRadius", cfg.m_fIdlePatrolRadius);
		ctx.WriteValue("m_fIdlePatrolIntervalSec", cfg.m_fIdlePatrolIntervalSec);
		ctx.WriteValue("m_fIdlePatrolChance", cfg.m_fIdlePatrolChance);
		ctx.WriteValue("m_bEnableCoverStance", cfg.m_bEnableCoverStance);
		ctx.WriteValue("m_fCoverStanceCheckSec", cfg.m_fCoverStanceCheckSec);
		ctx.WriteValue("m_fCoverStanceMaxRange", cfg.m_fCoverStanceMaxRange);
		ctx.WriteValue("m_fCoverStandEye", cfg.m_fCoverStandEye);
		ctx.WriteValue("m_fCoverCrouchEye", cfg.m_fCoverCrouchEye);
		ctx.WriteValue("m_fCoverProneEye", cfg.m_fCoverProneEye);
		ctx.WriteValue("m_bEnableMemberMorale", cfg.m_bEnableMemberMorale);
		ctx.WriteValue("m_fMemberMoraleCheckSec", cfg.m_fMemberMoraleCheckSec);
		ctx.WriteValue("m_fMemberMoraleSuppressionWeight", cfg.m_fMemberMoraleSuppressionWeight);
		ctx.WriteValue("m_fMemberMoraleHesitateThreshold", cfg.m_fMemberMoraleHesitateThreshold);
		ctx.WriteValue("m_fMemberMoraleHesitateSec", cfg.m_fMemberMoraleHesitateSec);
		ctx.WriteValue("m_bMemberMoraleCower", cfg.m_bMemberMoraleCower);
		ctx.WriteValue("m_fMemberMoraleCowerThreshold", cfg.m_fMemberMoraleCowerThreshold);
		ctx.WriteValue("m_bEnableEmergencyRearm", cfg.m_bEnableEmergencyRearm);
		ctx.WriteValue("m_fRearmCheckSec", cfg.m_fRearmCheckSec);
		ctx.WriteValue("m_iRearmMagazineCount", cfg.m_iRearmMagazineCount);
		ctx.WriteValue("m_bRearmScavenge", cfg.m_bRearmScavenge);
		ctx.WriteValue("m_fRearmScavengeRange", cfg.m_fRearmScavengeRange);
		ctx.WriteValue("m_fRearmPickupDist", cfg.m_fRearmPickupDist);
		ctx.WriteValue("m_bEnableCQB", cfg.m_bEnableCQB);
		ctx.WriteValue("m_fCqbCheckSec", cfg.m_fCqbCheckSec);
		ctx.WriteValue("m_fCqbEngageRange", cfg.m_fCqbEngageRange);
		ctx.WriteValue("m_fCqbBuildingScan", cfg.m_fCqbBuildingScan);
		ctx.WriteValue("m_fCqbArriveDist", cfg.m_fCqbArriveDist);
		ctx.WriteValue("m_fCqbReorderDist", cfg.m_fCqbReorderDist);
		ctx.WriteValue("m_bEnableCqbClear", cfg.m_bEnableCqbClear);
		ctx.WriteValue("m_bCqbClearMarkedOnly", cfg.m_bCqbClearMarkedOnly);
		ctx.WriteValue("m_fCqbClearRadius", cfg.m_fCqbClearRadius);
		ctx.WriteValue("m_fCqbNodeDwellSec", cfg.m_fCqbNodeDwellSec);
		ctx.WriteValue("m_fCqbStackSpacing", cfg.m_fCqbStackSpacing);
		ctx.WriteValue("m_iCqbMaxBuildings", cfg.m_iCqbMaxBuildings);
		ctx.WriteValue("m_fCqbClearCheckSec", cfg.m_fCqbClearCheckSec);
		ctx.WriteValue("m_fCqbReorderDistClear", cfg.m_fCqbReorderDistClear);
		ctx.WriteValue("m_fCqbApproachTimeoutSec", cfg.m_fCqbApproachTimeoutSec);
		ctx.WriteValue("m_fCqbPerceptionBoost", cfg.m_fCqbPerceptionBoost);
		ctx.WriteValue("m_bDebugCqbClear", cfg.m_bDebugCqbClear);
		ctx.WriteValue("m_bEnableMachineGunner", cfg.m_bEnableMachineGunner);
		ctx.WriteValue("m_fMGCheckSec", cfg.m_fMGCheckSec);
		ctx.WriteValue("m_bMGReposition", cfg.m_bMGReposition);
		ctx.WriteValue("m_fMGRepositionRadius", cfg.m_fMGRepositionRadius);
		ctx.WriteValue("m_fMGSightHeight", cfg.m_fMGSightHeight);
		ctx.WriteValue("m_fMGEngageRange", cfg.m_fMGEngageRange);
		ctx.WriteValue("m_bEnableFleeSmoke", cfg.m_bEnableFleeSmoke);

		// --- Fratricide / straggler merge / suppression / contagion ---.
		ctx.WriteValue("m_bEnableSuppression", cfg.m_bEnableSuppression);
		ctx.WriteValue("m_fSuppressionCheckSec", cfg.m_fSuppressionCheckSec);
		ctx.WriteValue("m_fSuppressionThreshold", cfg.m_fSuppressionThreshold);
		ctx.WriteValue("m_fSuppressedFireRateCoef", cfg.m_fSuppressedFireRateCoef);
		ctx.WriteValue("m_bSuppressionSeekCover", cfg.m_bSuppressionSeekCover);
		ctx.WriteValue("m_fSuppressionCoverRadius", cfg.m_fSuppressionCoverRadius);
		ctx.WriteValue("m_fSuppressionSmokeFraction", cfg.m_fSuppressionSmokeFraction);
		ctx.WriteValue("m_bSuppressionDigIn", cfg.m_bSuppressionDigIn);
		ctx.WriteValue("m_fSuppressionDashMax", cfg.m_fSuppressionDashMax);
		ctx.WriteValue("m_fSuppressionMaxDescend", cfg.m_fSuppressionMaxDescend);
		ctx.WriteValue("m_bEnableMoraleContagion", cfg.m_bEnableMoraleContagion);
		ctx.WriteValue("m_fContagionRadius", cfg.m_fContagionRadius);
		ctx.WriteValue("m_fContagionCheckSec", cfg.m_fContagionCheckSec);
		ctx.WriteValue("m_fContagionMoralePerBrokenGroup", cfg.m_fContagionMoralePerBrokenGroup);
		ctx.WriteValue("m_fContagionMaxLossPerTick", cfg.m_fContagionMaxLossPerTick);
		ctx.WriteValue("m_bEnableFriendlyFire", cfg.m_bEnableFriendlyFire);
		ctx.WriteValue("m_fFriendlyFireHold", cfg.m_fFriendlyFireHold);
		ctx.WriteValue("m_fFriendlyLaneCheckSec", cfg.m_fFriendlyLaneCheckSec);
		ctx.WriteValue("m_bEnableStragglerMerge", cfg.m_bEnableStragglerMerge);
		ctx.WriteValue("m_iStragglerSize", cfg.m_iStragglerSize);
		ctx.WriteValue("m_fMergeRadius", cfg.m_fMergeRadius);
		ctx.WriteValue("m_fMergeCheckSec", cfg.m_fMergeCheckSec);

		// --- QRF garrison ---.
		ctx.WriteValue("m_bClearWaypointOnOverride", cfg.m_bClearWaypointOnOverride);
		ctx.WriteValue("m_fQRFCriticalMorale", cfg.m_fQRFCriticalMorale);
		ctx.WriteValue("m_fQRFHoldLeash", cfg.m_fQRFHoldLeash);

		// --- Hide from armour ---.
		ctx.WriteValue("m_bEnableFleeFromArmor", cfg.m_bEnableFleeFromArmor);
		ctx.WriteValue("m_fFleeFromArmorRange", cfg.m_fFleeFromArmorRange);
		ctx.WriteValue("m_fFleeFromArmorCheckSec", cfg.m_fFleeFromArmorCheckSec);
	}

	protected static void WriteTacticalMoveKeys(JsonSaveContext ctx)
	{
		DCO_TacticalMoveSettings tmove = DCO_TacticalMoveSettings.Get();
		if (tmove)
		{
			ctx.WriteValue("m_bEnableTacticalMove", tmove.m_bEnableTacticalMove);
			ctx.WriteValue("m_fMinMoveDistance", tmove.m_fMinMoveDistance);
			ctx.WriteValue("m_fCheckInInterval", tmove.m_fCheckInInterval);
			ctx.WriteValue("m_fObservePause", tmove.m_fObservePause);
			ctx.WriteValue("m_bAvoidThreatFunnel", tmove.m_bAvoidThreatFunnel);
			ctx.WriteValue("m_fFunnelSidestep", tmove.m_fFunnelSidestep);
			ctx.WriteValue("m_fFunnelCorridorHalfWidth", tmove.m_fFunnelCorridorHalfWidth);
			ctx.WriteValue("m_bEnableFlanking", tmove.m_bEnableFlanking);
			ctx.WriteValue("m_fFlankAngleDeg", tmove.m_fFlankAngleDeg);
			ctx.WriteValue("m_fFlankMaxEnemyDist", tmove.m_fFlankMaxEnemyDist);
			ctx.WriteValue("m_fFlankMinEnemyDist", tmove.m_fFlankMinEnemyDist);
			ctx.WriteValue("m_bEnableExposureScoring", tmove.m_bEnableExposureScoring);
			ctx.WriteValue("m_iExposureLayerMask", tmove.m_iExposureLayerMask);
			ctx.WriteValue("m_fExposureSidestep", tmove.m_fExposureSidestep);
			ctx.WriteValue("m_bEnableProceduralPath", tmove.m_bEnableProceduralPath);
			ctx.WriteValue("m_bPathDebugDraw", tmove.m_bPathDebugDraw);
			ctx.WriteValue("m_fPathReassessSec", tmove.m_fPathReassessSec);
			ctx.WriteValue("m_fPathLegLength", tmove.m_fPathLegLength);
			ctx.WriteValue("m_fPathConeHalfAngleDeg", tmove.m_fPathConeHalfAngleDeg);
			ctx.WriteValue("m_iPathConeSamples", tmove.m_iPathConeSamples);
			ctx.WriteValue("m_fPathArriveDist", tmove.m_fPathArriveDist);
			ctx.WriteValue("m_iPathMaxLegs", tmove.m_iPathMaxLegs);
			ctx.WriteValue("m_fPathThreatActivation", tmove.m_fPathThreatActivation);
			ctx.WriteValue("m_fPathConcealmentBonus", tmove.m_fPathConcealmentBonus);
			ctx.WriteValue("m_fPathStraightBias", tmove.m_fPathStraightBias);
			ctx.WriteValue("m_bEnableBaseOfFireSplit", tmove.m_bEnableBaseOfFireSplit);
			ctx.WriteValue("m_fPathSplitThreatActivation", tmove.m_fPathSplitThreatActivation);
			ctx.WriteValue("m_bPathHaltOnContact", tmove.m_bPathHaltOnContact);
			ctx.WriteValue("m_fPathContactHaltRange", tmove.m_fPathContactHaltRange);
			ctx.WriteValue("m_bPathLeaderInBase", tmove.m_bPathLeaderInBase);
			ctx.WriteValue("m_bPathBaseMaintainLos", tmove.m_bPathBaseMaintainLos);
			ctx.WriteValue("m_fPathBaseLosRadius", tmove.m_fPathBaseLosRadius);
			ctx.WriteValue("m_bEnableTravelingOverwatch", tmove.m_bEnableTravelingOverwatch);
			ctx.WriteValue("m_fPathTravelOverwatchThreat", tmove.m_fPathTravelOverwatchThreat);
			ctx.WriteValue("m_bEnableCoverSeek", tmove.m_bEnableCoverSeek);
			ctx.WriteValue("m_fCoverSeekCheckSec", tmove.m_fCoverSeekCheckSec);
			ctx.WriteValue("m_fCoverSeekRadius", tmove.m_fCoverSeekRadius);
			ctx.WriteValue("m_iCoverSeekSamples", tmove.m_iCoverSeekSamples);
			ctx.WriteValue("m_fCoverMaxClimb", tmove.m_fCoverMaxClimb);
			ctx.WriteValue("m_bEnableReactToContact", tmove.m_bEnableReactToContact);
			ctx.WriteValue("m_fReactCheckSec", tmove.m_fReactCheckSec);
			ctx.WriteValue("m_fSupportFireRateCoef", tmove.m_fSupportFireRateCoef);
			ctx.WriteValue("m_fAssaultFlankAngleDeg", tmove.m_fAssaultFlankAngleDeg);
			ctx.WriteValue("m_bEnableStandoff", tmove.m_bEnableStandoff);
			ctx.WriteValue("m_fStandoffRifle", tmove.m_fStandoffRifle);
			ctx.WriteValue("m_fStandoffMG", tmove.m_fStandoffMG);
			ctx.WriteValue("m_fStandoffSniper", tmove.m_fStandoffSniper);
			ctx.WriteValue("m_fStandoffLauncher", tmove.m_fStandoffLauncher);
			ctx.WriteValue("m_fStandoffPistol", tmove.m_fStandoffPistol);
			ctx.WriteValue("m_fStandoffBand", tmove.m_fStandoffBand);
			ctx.WriteValue("m_fBreakContactDistance", tmove.m_fBreakContactDistance);
			ctx.WriteValue("m_bEnableTacticalBrain", tmove.m_bEnableTacticalBrain);
			ctx.WriteValue("m_fBrainCheckSec", tmove.m_fBrainCheckSec);
			ctx.WriteValue("m_fBrainAssaultPowerRatio", tmove.m_fBrainAssaultPowerRatio);
			ctx.WriteValue("m_fBrainBreakPowerRatio", tmove.m_fBrainBreakPowerRatio);
			ctx.WriteValue("m_fBrainEnemyScanRadius", tmove.m_fBrainEnemyScanRadius);
			ctx.WriteValue("m_bEnableDcoFormations", tmove.m_bEnableDcoFormations);
			ctx.WriteValue("m_fFormationShapeCheckSec", tmove.m_fFormationShapeCheckSec);
			ctx.WriteValue("m_fFormationSpacing", tmove.m_fFormationSpacing);
			ctx.WriteValue("m_iFormationTravelShape", tmove.m_iFormationTravelShape);
			ctx.WriteValue("m_iFormationContactShape", tmove.m_iFormationContactShape);
		}
	}

	protected static void WriteFormationKeys(JsonSaveContext ctx, DCO_MoraleSettings cfg)
	{
		// --- Adaptive formation ---.
		ctx.WriteValue("m_bEnableAdaptiveFormation", cfg.m_bEnableAdaptiveFormation);
		ctx.WriteValue("m_fFormationCheckSec", cfg.m_fFormationCheckSec);
		ctx.WriteValue("m_sFormationTravel", cfg.m_sFormationTravel);
		ctx.WriteValue("m_sFormationContact", cfg.m_sFormationContact);
	}

	protected static void WriteSubsystemKeys(JsonSaveContext ctx)
	{
		DCO_BaseSettings base = DCO_BaseSettings.Get();
		if (base)
		{
			ctx.WriteValue("base_enable",     base.m_bEnableBaseSettings);
			ctx.WriteValue("base_ai_skill",   base.m_iAiSkill);
			ctx.WriteValue("base_perception", base.m_iPerception);
			ctx.WriteValue("base_reaction",   base.m_iReactionTime);
			ctx.WriteValue("base_fire_rate",  base.m_iFireRate);
			ctx.WriteValue("base_grade",      base.m_eGlobalGrade);
			ctx.WriteValue("base_stance",     base.m_eGlobalStance);
			ctx.WriteValue("base_formation",  base.m_eDefaultSpawnFormation);
			ctx.WriteValue("base_debug",      base.m_bDebugBaseSettings);
		}

		DCO_ATSettings at = DCO_ATSettings.Get();
		if (at)
		{
			ctx.WriteValue("at_enable_discipline",  at.m_bEnableLauncherDiscipline);
			ctx.WriteValue("at_stow_when_dry",      at.m_bLauncherStowWhenDry);
			ctx.WriteValue("at_infantry_range",     at.m_fLauncherInfantryRange);
			ctx.WriteValue("at_vs_buildings",       at.m_bLauncherVsBuildings);
			ctx.WriteValue("at_building_scan",      at.m_fLauncherBuildingScan);
			ctx.WriteValue("at_vs_infantry_chance", at.m_fLauncherVsInfantryChance);
			ctx.WriteValue("at_check_sec",          at.m_fLauncherDisciplineCheckSec);
		}
	}

	static void LoadBaseInto(DCO_BaseSettings cfg)
	{
		if (!cfg || !FileIO.FileExists(FILE_PATH))
			return;
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(FILE_PATH))
			return;
		ctx.ReadValue("base_enable",     cfg.m_bEnableBaseSettings);
		ctx.ReadValue("base_ai_skill",   cfg.m_iAiSkill);
		ctx.ReadValue("base_perception", cfg.m_iPerception);
		ctx.ReadValue("base_reaction",   cfg.m_iReactionTime);
		ctx.ReadValue("base_fire_rate",  cfg.m_iFireRate);
		// Enum fields: ReadValue into int temp, then cast.
		int gradeInt = cfg.m_eGlobalGrade;
		ctx.ReadValue("base_grade", gradeInt);
		cfg.m_eGlobalGrade = gradeInt;
		int stanceInt = cfg.m_eGlobalStance;
		ctx.ReadValue("base_stance", stanceInt);
		cfg.m_eGlobalStance = stanceInt;
		ctx.ReadValue("base_formation",  cfg.m_eDefaultSpawnFormation);
		ctx.ReadValue("base_debug",      cfg.m_bDebugBaseSettings);
	}

	static void LoadATInto(DCO_ATSettings cfg)
	{
		if (!cfg || !FileIO.FileExists(FILE_PATH))
			return;
		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(FILE_PATH))
			return;
		ctx.ReadValue("at_enable_discipline",  cfg.m_bEnableLauncherDiscipline);
		ctx.ReadValue("at_stow_when_dry",      cfg.m_bLauncherStowWhenDry);
		ctx.ReadValue("at_infantry_range",     cfg.m_fLauncherInfantryRange);
		ctx.ReadValue("at_vs_buildings",       cfg.m_bLauncherVsBuildings);
		ctx.ReadValue("at_building_scan",      cfg.m_fLauncherBuildingScan);
		ctx.ReadValue("at_vs_infantry_chance", cfg.m_fLauncherVsInfantryChance);
		ctx.ReadValue("at_check_sec",          cfg.m_fLauncherDisciplineCheckSec);
	}
}
