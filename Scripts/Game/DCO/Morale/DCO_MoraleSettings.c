class DCO_MoraleSettings
{
	protected static ref DCO_MoraleSettings s_Instance;

	bool	m_bEnabled				= true;
	bool	m_bDebug				= false;
	float	m_fUpdateIntervalSec	= 2.0;
	float	m_fFleeThreshold		= 25.0;
	float	m_fRallyThreshold		= 55.0;
	float	m_fRecoveryPerTick		= 7.0;
	float	m_fDrainPerTick			= 9.0;
	float	m_fCasualtyWeight		= 45.0;
	float	m_fHeavyThreat			= 0.5;

	// Once below the flee threshold, breaking happens by chance per tick, not deterministically.
	float	m_fFleeChancePerTick				= 0.40;	// per-tick chance to break/flee once below flee threshold.

	float	m_fMoraleLostPerCasualty	= 12.0;

	// Unit cohesion / leadership: morale is a collective property - a squad holds together and breaks as a whole, not man-by-man.
	float	m_fMoraleLostOnLeaderLoss	= 15.0;

	bool	m_bEnableStanceCooldown		= false;
	float	m_fStanceCooldownSec		= 3.0;	// min seconds between AI in-place stance changes.
	bool	m_bStanceCooldownInCover	= false;
	bool	m_bStanceCooldownDebug		= false;
	float	m_fFleeDistance			= 120.0;

	bool	m_bEnablePanic			= false;
	float	m_fPanicBand			= 15.0;	// morale points above the flee threshold that count as "panic".
	float	m_fPanicChancePerTick	= 0.5;	// per-tick chance to panic while in the band.
	float	m_fPanicDurationSec		= 4.0;	// how long the cower/hold-fire lasts.
	float	m_fPanicCooldownSec		= 12.0;	// min seconds between panic episodes for a group.

	// Morale to accuracy: tie combat power to morale.
	bool	m_bEnableMoraleAccuracy	= false;
	float	m_fAccuracyRookieMorale	= 20.0;
	float	m_fAccuracyRegularMorale = 45.0;	// at/below this morale: REGULAR skill; above: default.
	float	m_fLowMoraleFireRateCoef = 0.7;

	bool	m_bEnableVisionLimit		= false;
	float	m_fNightPerceptionFactor	= 0.5;
	float	m_fVisionSunriseHour		= 6.0;
	float	m_fVisionSunsetHour			= 19.0;
	float	m_fVisionTwilightHours		= 1.5;
	float	m_fVisionCheckSec			= 10.0;	// how often vision limiting is re-evaluated per group.

	bool	m_bEnableIdleBehaviour		= false;
	float	m_fIdleDelaySec				= 60.0;	// stationary, taskless time before a group is considered idle.
	float	m_fIdleMoveEpsilon			= 3.0;
	float	m_fIdleSpreadRadius			= 10.0;
	float	m_fIdlePatrolRadius			= 150.0;
	float	m_fIdlePatrolIntervalSec	= 45.0;	// seconds between patrol rolls.
	float	m_fIdlePatrolChance			= 0.5;

	bool	m_bEnableCoverStance		= false;
	float	m_fCoverStanceCheckSec		= 4.0;	// how often stance fitting runs per group.
	float	m_fCoverStanceMaxRange		= 120.0;
	float	m_fCoverStandEye			= 1.5;
	float	m_fCoverCrouchEye			= 0.9;
	float	m_fCoverProneEye			= 0.4;

	bool	m_bEnableMemberMorale			= false;
	float	m_fMemberMoraleCheckSec			= 2.0;	// how often per-member morale is evaluated.
	float	m_fMemberMoraleSuppressionWeight = 50.0;	// how strongly personal suppression subtracts from morale.
	float	m_fMemberMoraleHesitateThreshold = 50.0;	// personal morale at/below this: the soldier hesitates / heads down.
	float	m_fMemberMoraleHesitateSec		= 2.5;
	bool	m_bMemberMoraleCower			= false;
	float	m_fMemberMoraleCowerThreshold	= 20.0;

	bool	m_bEnableEmergencyRearm		= false;
	float	m_fRearmCheckSec			= 5.0;	// how often members are checked for being out of ammo.
	int		m_iRearmMagazineCount		= 3;	// magazines to top a dry weapon up to.
	bool	m_bRearmScavenge			= false;	// also walk to + pick up the nearest dropped ground weapon.
	float	m_fRearmScavengeRange		= 30.0;
	float	m_fRearmPickupDist			= 2.0;

	bool	m_bEnableCQB				= false;
	float	m_fCqbCheckSec				= 2.5;	// how often the CQB push is evaluated.
	float	m_fCqbEngageRange			= 60.0;
	float	m_fCqbBuildingScan			= 12.0;
	float	m_fCqbArriveDist			= 6.0;
	float	m_fCqbReorderDist			= 8.0;

	bool	m_bEnableCqbClear		= false;	// master toggle.
	bool	m_bCqbClearMarkedOnly	= false;	// false = every on-foot group clears; true = only groups flagged CQB Clearer.
	float	m_fCqbClearRadius		= 80.0;
	float	m_fCqbNodeDwellSec		= 3.0;	// seconds a member holds + scans each interior node before advancing.
	float	m_fCqbStackSpacing		= 1.5;	// metres between stacked members at the breach point.
	int		m_iCqbMaxBuildings		= 6;
	float	m_fCqbClearCheckSec		= 2.0;
	float	m_fCqbReorderDistClear	= 4.0;
	float	m_fCqbApproachTimeoutSec	= 30.0;
	float	m_fCqbPerceptionBoost	= 2.0;
	bool	m_bDebugCqbClear		= false;	// draw building states + stack/node markers.

	bool	m_bEnableMachineGunner		= false;
	float	m_fMGCheckSec				= 3.0;	// how often MG emplacement is evaluated.
	bool	m_bMGReposition				= true;
	float	m_fMGRepositionRadius		= 25.0;
	float	m_fMGSightHeight			= 0.8;
	float	m_fMGEngageRange			= 300.0;

	// Smoke-covered retreat: when a group breaks and flees, pop smoke toward the enemy to screen the withdrawal.
	bool	m_bEnableFleeSmoke			= false;

	bool	m_bEnableSuppression			= false;
	float	m_fSuppressionCheckSec			= 1.5;
	float	m_fSuppressionThreshold			= 0.5;
	float	m_fSuppressedFireRateCoef		= 0.4;
	bool	m_bSuppressionSeekCover			= true;	// pinned members sprint to the nearest cover / break LOS.
	float	m_fSuppressionCoverRadius		= 12.0;
	float	m_fSuppressionSmokeFraction		= 0.5;	// fraction of the group pinned that triggers a smoke screen.

	bool	m_bSuppressionDigIn				= true;
	float	m_fSuppressionDashMax			= 8.0;
	float	m_fSuppressionMaxDescend		= 4.0;

	bool	m_bEnableMoraleContagion			= false;
	float	m_fContagionRadius					= 150.0;	// metres - nearby friendly groups that can spread panic.
	float	m_fContagionCheckSec				= 5.0;	// how often contagion is evaluated.
	float	m_fContagionMoralePerBrokenGroup	= 4.0;	// morale lost per nearby broken friendly group, per check.
	float	m_fContagionMaxLossPerTick			= 12.0;	// cap on contagion morale loss per check.

	// Friendly-fire / fratricide avoidance: when a member would fire on an enemy but a friendly is in its firing lane, briefly hold its fire.
	bool	m_bEnableFriendlyFire		= false;
	float	m_fFriendlyFireHold			= 1.0;	// seconds of held-fire when a friendly is in the lane.
	float	m_fFriendlyLaneCheckSec		= 0.5;	// how often the lanes are checked.

	bool	m_bEnableStragglerMerge		= false;
	int		m_iStragglerSize			= 2;	// merge when the group is at or below this many members.
	float	m_fMergeRadius				= 150.0;	// metres to look for a larger friendly group to join.
	float	m_fMergeCheckSec			= 8.0;	// how often a straggler looks to merge.

	float	m_fQRFCriticalMorale	= 40.0;
	float	m_fQRFHoldLeash			= 30.0;

	bool	m_bClearWaypointOnOverride	= false;

	bool	m_bEnableFleeFromArmor		= false;
	float	m_fFleeFromArmorRange		= 150.0;	// metres - an enemy vehicle this close triggers the flee.
	float	m_fFleeFromArmorCheckSec	= 4.0;	// how often it is evaluated.

	bool	m_bEnableAdaptiveFormation	= false;
	float	m_fFormationCheckSec		= 5.0;
	string	m_sFormationTravel			= "Column";	// formation when not in contact.
	string	m_sFormationContact			= "Line";	// formation when a perceived enemy is present.

	static DCO_MoraleSettings Get()
	{
		if (!s_Instance)
		{
			s_Instance = new DCO_MoraleSettings();
			DCO_JsonConfig.LoadInto(s_Instance);
		}
		return s_Instance;
	}

	float UpdateIntervalMs()
	{
		return m_fUpdateIntervalSec * 1000.0;
	}
}
