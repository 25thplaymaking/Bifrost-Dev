// CQB town-clearing shared types.
enum EDCO_CqbState
{
	IDLE,
	SELECT,	// pick + claim the building nearest the placed marker.
	SURVEY,	// stream navmesh, build + cache the sector plan.
	CLEAR,	// serialized sectors: stack -> flow -> dwell, one sector at a time.
	DONE,	// nothing left to clear; complete the placed waypoint.
}

enum EDCO_CqbSurveyResult
{
	INVALID,	// missing building/pathfinding/navmesh.
	WAITING,	// one or more navmesh tiles are still streaming.
	READY,
}

enum EDCO_CqbCellState
{
	PENDING,
	MOVING,	// an assignee is en route.
	CLEARED,	// an assignee physically arrived.
	SKIPPED,	// navmesh-valid but no assignee could reach it.
}

enum EDCO_CqbSectorState
{
	PENDING,
	STACK,	// entry members forming up behind the entry portal.
	FLOW,	// chain-released members fanning out to this sector's cells.
	DWELL,	// all cells resolved; one quiet dwell with a member still inside.
	CLEARED,
	SKIPPED,	// every room cell proved unreachable.
}

// One navmesh- and building-floor-validated coverage position.
class DCO_CqbCell
{
	vector				m_vPos;
	EDCO_CqbCellState	m_eState = EDCO_CqbCellState.PENDING;
	int					m_iSector = -1;
	bool				m_bDoor = false;
	AIAgent				m_Assignee;	// entry member currently owning this cell.
	float				m_fIssuedTime = -1;
	float				m_fLastProgressTime = -1;
	float				m_fLastDoorAssistTime = -1;
	float				m_fBestDistSq = 1000000000.0;
	int					m_iRetries = 0;
}

// A room-scale clear unit: an LOS/floor cluster of coverage cells joined to its neighbours by door portals.
class DCO_CqbSector
{
	ref array<int>		m_aCellIdx = {};	// room-cell indexes into the plan, entry-near first.
	ref array<int>		m_aPortalIdx = {};	// door-cell indexes touching this sector.
	EDCO_CqbSectorState	m_eState = EDCO_CqbSectorState.PENDING;
	int					m_iFloor = 0;	// floor band, for ordering/debug only.
	vector				m_vMins;
	vector				m_vMaxs;
	int					m_iEntryPortal = -1;	// door-cell index used to enter; -1 = open flow.
	float				m_fDwellStart = -1;
	float				m_fDwellEnteredTime = -1;
	int					m_iScanStep = 0;	// rotates smart-look targets.
}

// A building's full clear plan: survey geometry + sector order.
class DCO_CqbPlan
{
	ref array<ref DCO_CqbCell>		m_aCells = {};
	ref array<ref DCO_CqbSector>	m_aSectors = {};

	int RoomCellCount()
	{
		int n = 0;
		foreach (DCO_CqbCell cell : m_aCells)
		{
			if (cell && !cell.m_bDoor)
				n++;
		}
		return n;
	}

	// Geometry-only copy with pristine runtime state.
	DCO_CqbPlan DeepCopy()
	{
		DCO_CqbPlan copy = new DCO_CqbPlan();
		foreach (DCO_CqbCell cell : m_aCells)
		{
			if (!cell)
			{
				copy.m_aCells.Insert(null);
				continue;
			}
			DCO_CqbCell c = new DCO_CqbCell();
			c.m_vPos = cell.m_vPos;
			c.m_iSector = cell.m_iSector;
			c.m_bDoor = cell.m_bDoor;
			copy.m_aCells.Insert(c);
		}
		foreach (DCO_CqbSector sector : m_aSectors)
		{
			if (!sector)
			{
				copy.m_aSectors.Insert(null);
				continue;
			}
			DCO_CqbSector s = new DCO_CqbSector();
			foreach (int ci : sector.m_aCellIdx)
				s.m_aCellIdx.Insert(ci);
			foreach (int pi : sector.m_aPortalIdx)
				s.m_aPortalIdx.Insert(pi);
			s.m_iFloor = sector.m_iFloor;
			s.m_vMins = sector.m_vMins;
			s.m_vMaxs = sector.m_vMaxs;
			s.m_iEntryPortal = sector.m_iEntryPortal;
			copy.m_aSectors.Insert(s);
		}
		return copy;
	}

	void ResetRuntime()
	{
		foreach (DCO_CqbCell cell : m_aCells)
		{
			if (!cell)
				continue;
			cell.m_eState = EDCO_CqbCellState.PENDING;
			cell.m_Assignee = null;
			cell.m_fIssuedTime = -1;
			cell.m_fLastProgressTime = -1;
			cell.m_fLastDoorAssistTime = -1;
			cell.m_fBestDistSq = 1000000000.0;
			cell.m_iRetries = 0;
		}
		foreach (DCO_CqbSector sector : m_aSectors)
		{
			if (!sector)
				continue;
			sector.m_eState = EDCO_CqbSectorState.PENDING;
			sector.m_fDwellStart = -1;
			sector.m_fDwellEnteredTime = -1;
			sector.m_iScanStep = 0;
		}
	}
}

// One building-clearing job for one group.
class DCO_CqbJob
{
	IEntity									m_Building;
	ref DCO_CqbPlan							m_Plan;
	ref array<ref SCR_AIGroupFireteamLock>	m_aFireteamLocks = {};	// every fireteam that contributed an entry member.
	ref array<AIAgent>						m_aEntryTeam = {};
	ref array<AIAgent>						m_aSecurityTeam = {};	// remaining direct members: outside cover.
	int										m_iActiveSector = -1;
	ref array<AIAgent>						m_aActiveEntry = {};	// usable entry members in release order for the active sector.
	int										m_iReleased = 0;	// entry members chain-released into the active sector.
	ref array<vector>						m_aStackSlots = {};	// validated stack positions for the active sector.
	float									m_fSectorIssueTime = -1;
	float									m_fBreachTime = -1;
	int										m_iEntryCap = 4;
	float									m_fLastReleaseTime = -1;	// anti-stall: chain releases may not gap longer than the release timeout.
	float									m_fSurveyStart = -1;
	int										m_iCleared = 0;	// room cells physically visited.
	int										m_iSkipped = 0;	// room cells proved unreachable.
	int										m_iRecoveryPasses = 0;
}
