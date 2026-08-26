class DCO_TacticalMoveSettings
{
	protected static ref DCO_TacticalMoveSettings s_Instance;

	// Unified movement-intent arbitration.
	bool	m_bEnableTacticalCoordinator	= false;

	// Master switch for the whole tactical-movement layer.
	bool	m_bEnableTacticalMove		= false;

	float	m_fMinMoveDistance			= 80.0;

	float	m_fCheckInInterval			= 60.0;

	float	m_fObservePause				= 3.0;

	float	m_fThreatActivation			= 0.10;

	bool	m_bAvoidThreatFunnel		= false;
	float	m_fFunnelSidestep			= 45.0;	// metres to offset the approach away from the enemy axis.
	float	m_fFunnelCorridorHalfWidth	= 30.0;	// threat counts as "in the funnel" within this of the path.

	bool	m_bEnableFlanking			= false;
	float	m_fFlankAngleDeg			= 55.0;
	float	m_fFlankMaxEnemyDist		= 250.0;
	float	m_fFlankMinEnemyDist		= 25.0;

	bool	m_bEnableExposureScoring	= false;
	int		m_iExposureLayerMask		= 0;
	float	m_fExposureSidestep			= 30.0;

	bool	m_bEnableProceduralPath		= false;
	bool	m_bPathDebugDraw			= false;
	float	m_fPathReassessSec			= 3.0;	// how often the route is re-decided as the group moves.
	float	m_fPathLegLength			= 25.0;
	float	m_fPathConeHalfAngleDeg		= 50.0;	// half-angle of the forward sample cone toward the goal.
	int		m_iPathConeSamples			= 5;	// candidate directions sampled across the cone.
	float	m_fPathArriveDist			= 8.0;
	int		m_iPathMaxLegs				= 8;
	float	m_fPathThreatActivation		= 0.10;
	float	m_fPathConcealmentBonus		= 40.0;	// score bonus for a leg concealed from the threat.
	float	m_fPathStraightBias			= 10.0;	// penalty for veering off the straight line to the goal.

	bool	m_bEnableTravelingOverwatch		= false;
	float	m_fPathTravelOverwatchThreat	= 0.30;

	bool	m_bEnableBaseOfFireSplit	= false;
	float	m_fPathSplitThreatActivation = 0.5;

	bool	m_bPathHaltOnContact		= true;
	float	m_fPathContactHaltRange		= 200.0;
	bool	m_bPathLeaderInBase			= true;
	bool	m_bPathBaseMaintainLos		= true;
	float	m_fPathBaseLosRadius		= 15.0;

	bool	m_bEnableCoverSeek		= false;
	float	m_fCoverSeekCheckSec	= 5.0;	// how often cover placement is re-evaluated.
	float	m_fCoverSeekRadius		= 12.0;
	int		m_iCoverSeekSamples		= 6;	// directions sampled in the ring around each member.
	float	m_fCoverMaxClimb		= 2.0;

	bool	m_bEnableReactToContact		= false;
	float	m_fReactCheckSec			= 2.0;	// how often the contact drill re-evaluates.
	float	m_fSupportFireRateCoef		= 1.3;	// fire-rate multiplier for the base-of-fire / support-by-fire volume.
	float	m_fAssaultFlankAngleDeg		= 60.0;

	bool	m_bEnableStandoff			= true;	// ON by default - deliberate exception to the usual OFF-by-default rule.
	float	m_fStandoffRifle			= 35.0;
	float	m_fStandoffMG				= 80.0;
	float	m_fStandoffSniper			= 120.0;
	float	m_fStandoffLauncher			= 60.0;
	float	m_fStandoffPistol			= 12.0;
	float	m_fStandoffBand				= 25.0;
	float	m_fBreakContactDistance		= 100.0;

	bool	m_bEnableTacticalBrain		= false;
	float	m_fBrainCheckSec			= 3.0;	// how often the COA is re-evaluated.
	float	m_fBrainAssaultPowerRatio	= 1.3;	// own:enemy strength ratio at/above which the brain chooses ASSAULT_FLANK.
	float	m_fBrainBreakPowerRatio		= 0.5;	// own:enemy strength ratio at/below which the brain chooses BREAK_CONTACT.
	float	m_fBrainEnemyScanRadius		= 200.0;

	bool	m_bEnableDcoFormations		= false;
	float	m_fFormationShapeCheckSec	= 5.0;	// how often the shape is re-imposed.
	float	m_fFormationSpacing			= 5.0;
	int		m_iFormationTravelShape		= 1;
	int		m_iFormationContactShape	= 2;

	bool	m_bDebugLog					= false;
	bool	m_bForceIgnoreThreat		= false;

	static DCO_TacticalMoveSettings Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_TacticalMoveSettings();
		return s_Instance;
	}
}
