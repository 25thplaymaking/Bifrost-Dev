
// QueryEntitiesBySphere probe: is there any building within the radius?
class DCO_ATBuildingProbe
{
	bool m_bFound = false;

	bool Collect(IEntity e)
	{
		if (!e)
			return true;
		if (Building.Cast(e) || e.FindComponent(SCR_DestructibleBuildingComponent))
		{
			m_bFound = true;
			return false;	// stop the query - one building is enough.
		}
		return true;
	}
}

modded class SCR_AICombatComponent
{
	protected float m_fDCO_LauncherLicenseUntil = -1;

	void DCO_SetLauncherPolicy(bool allow)
	{
		if (!allow)
		{
			m_fDCO_LauncherLicenseUntil = -1;
			return;
		}
		BaseWorld world = GetGame().GetWorld();
		if (world)
			m_fDCO_LauncherLicenseUntil = world.GetWorldTime() + 1500.0;
	}

	override void EvaluateWeaponAndTarget(out bool outWeaponEvent, out bool outSelectedTargetChanged,
		out BaseTarget outPrevTarget, out BaseTarget outCurrentTarget,
		out bool outRetreatTargetChanged, out bool outCompartmentChanged)
	{
		super.EvaluateWeaponAndTarget(outWeaponEvent, outSelectedTargetChanged, outPrevTarget, outCurrentTarget,
			outRetreatTargetChanged, outCompartmentChanged);

		BaseWorld world = GetGame().GetWorld();
		BaseTarget target = GetCurrentTarget();
		if (!world || world.GetWorldTime() > m_fDCO_LauncherLicenseUntil || !target
			|| target.GetUnitType() != EAIUnitType.UnitType_Infantry)
			return;

		array<int> launcherOnly = {EWeaponType.WT_ROCKETLAUNCHER};
		if (!m_WeaponTargetSelector.SelectWeaponAgainstUnitTypeAndDistance(
			EAIUnitType.UnitType_VehicleUnarmored, target.GetDistance(), true, false, launcherOnly))
			return;

		BaseWeaponComponent weapon;
		BaseMagazineComponent magazine;
		int muzzleId;
		m_WeaponTargetSelector.GetSelectedWeapon(weapon, muzzleId, magazine);
		if (!weapon || weapon.GetWeaponType() != EWeaponType.WT_ROCKETLAUNCHER || !magazine || magazine.GetAmmoCount() <= 0)
			return;

		bool changed = weapon != m_SelectedWeaponComp || muzzleId != m_iSelectedMuzzle || magazine != m_SelectedMagazineComp;
		m_SelectedWeaponComp = weapon;
		m_iSelectedMuzzle = muzzleId;
		m_SelectedMagazineComp = magazine;
		m_WeaponTargetSelector.GetSelectedWeaponProperties(m_fSelectedWeaponMinDist, m_fSelectedWeaponMaxDist,
			m_bSelectedWeaponDirectDamage);

		if (changed)
		{
			array<BaseMuzzleComponent> muzzles = {};
			weapon.GetMuzzlesList(muzzles);
			if (m_ConfigComponent)
			{
				if (muzzleId < 0 || muzzleId >= muzzles.Count())
					m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(weapon.GetWeaponType(), 0);
				else
					m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(
						weapon.GetWeaponType(), muzzles[muzzleId].GetMuzzleType());
			}
			outWeaponEvent = true;
		}
	}
}

modded class SCR_AIGroupUtilityComponent
{
	protected float				m_fDCO_LastATTime		= -1;
	protected bool				m_bDCO_ATWasInContact	= false;
	protected ref map<IEntity, bool>	m_mDCO_ATUseRocket;	// member -> may rocket DISTANT infantry this engagement.

	void DCO_UpdateLauncherDiscipline()
	{
		if (!Replication.IsServer())
			return;

		DCO_ATSettings cfg = DCO_ATSettings.Get();
		if (!cfg.m_bEnableLauncherDiscipline || !m_Owner || !m_Perception)
			return;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;
		float now = world.GetWorldTime();
		if (m_fDCO_LastATTime >= 0 && (now - m_fDCO_LastATTime) < cfg.m_fLauncherDisciplineCheckSec * 1000.0)
			return;
		m_fDCO_LastATTime = now;

		// On-foot infantry only.
		if (DCO_VehicleUtil.IsGroupInVehicle(m_Owner))
			return;

		array<IEntity> targets = m_Perception.m_aTargetEntities;
		bool inContact = targets && !targets.IsEmpty();

		if (!inContact)
		{
			DCO_ATStowAll(false);
			if (m_bDCO_ATWasInContact && m_mDCO_ATUseRocket)
				m_mDCO_ATUseRocket.Clear();
			m_bDCO_ATWasInContact = false;
			return;
		}

		if (DCO_CqbClearUtil.IsClearingActive(this))
		{
			DCO_ATStowAll(true);
			m_bDCO_ATWasInContact = true;
			return;
		}

		bool rising = !m_bDCO_ATWasInContact;	// just entered contact this check = roll the latch.
		m_bDCO_ATWasInContact = true;

		if (!m_mDCO_ATUseRocket)
			m_mDCO_ATUseRocket = new map<IEntity, bool>();

		for (int di = m_mDCO_ATUseRocket.Count() - 1; di >= 0; di--)
		{
			if (!m_mDCO_ATUseRocket.GetKey(di))
				m_mDCO_ATUseRocket.RemoveElement(di);
		}

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			IEntity ent = agent.GetControlledEntity();
			if (!ent)
				continue;
			if (DCO_PlayerUtil.IsPlayer(ent))
				continue;	// AI only.

			// Roll this member's distant-infantry allowance once, on the engagement's rising edge.
			if (rising)
			{
				bool roll = Math.RandomFloat01() < cfg.m_fLauncherVsInfantryChance;
				m_mDCO_ATUseRocket.Set(ent, roll);
			}

			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
			if (!combat)
				continue;

			bool useRocket;
			if (!m_mDCO_ATUseRocket.Find(ent, useRocket))
				useRocket = false;

			// License only this soldier's valid infantry/building opportunity.
			combat.DCO_SetLauncherPolicy(DCO_ATShouldRequestLauncher(world, combat, cfg, useRocket));

			// Nothing else to do until the launcher is actually raised.
			if (combat.GetCurrentWeaponType() != EWeaponType.WT_ROCKETLAUNCHER)
				continue;

			SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
			if (!cc)
				continue;
			BaseWeaponManagerComponent wm = cc.GetWeaponManagerComponent();
			if (!wm)
				continue;

			// Dry reusable launcher up?
			if (cfg.m_bLauncherStowWhenDry)
			{
				BaseMagazineComponent mag = SCR_AIWeaponHandling.GetCurrentMagazineComponent(wm);
				if (!mag || mag.GetAmmoCount() <= 0)
				{
					combat.DCO_SetLauncherPolicy(false);
					DCO_ATSelectRifle(cc, combat);
					continue;
				}
			}

			if (DCO_ATShouldForceRifle(world, combat, cfg, useRocket))
				DCO_ATSelectRifle(cc, combat);
		}
	}

	// True only when Bifrost should ask engine weapon handling to draw a launcher against infantry.
	protected bool DCO_ATShouldRequestLauncher(BaseWorld world, SCR_AICombatComponent combat, DCO_ATSettings cfg, bool useRocket)
	{
		BaseTarget tgt = combat.GetCurrentTarget();
		if (!tgt || tgt.GetUnitType() != EAIUnitType.UnitType_Infantry)
			return false;
		if (cfg.m_bLauncherVsBuildings && DCO_ATNearBuilding(world, tgt.GetLastSeenPosition(), cfg.m_fLauncherBuildingScan))
			return true;
		return tgt.GetDistance() > cfg.m_fLauncherInfantryRange && useRocket;
	}

	// Target-aware rocket policy for a member currently holding a live launcher.
	protected bool DCO_ATShouldForceRifle(BaseWorld world, SCR_AICombatComponent combat, DCO_ATSettings cfg, bool useRocket)
	{
		BaseTarget tgt = combat.GetCurrentTarget();
		if (!tgt)
			return true;

		if (tgt.GetUnitType() != EAIUnitType.UnitType_Infantry)
			return false;	// vehicle / aircraft / fortification: the launcher is the right asset.

		// Infantry sheltering in/at a building: the structure is a legitimate rocket target.
		if (cfg.m_bLauncherVsBuildings && DCO_ATNearBuilding(world, tgt.GetLastSeenPosition(), cfg.m_fLauncherBuildingScan))
			return false;

		// Plain infantry in the open: rifle work inside rifle range; beyond it only latch winners rocket.
		if (tgt.GetDistance() <= cfg.m_fLauncherInfantryRange)
			return true;
		return !useRocket;
	}

	// Put away every raised launcher in the group.
	protected void DCO_ATStowAll(bool keepVsVehicles)
	{
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			IEntity ent = agent.GetControlledEntity();
			if (!ent || DCO_PlayerUtil.IsPlayer(ent))
				continue;
			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
			if (!combat)
				continue;
			combat.DCO_SetLauncherPolicy(false);
			if (combat.GetCurrentWeaponType() != EWeaponType.WT_ROCKETLAUNCHER)
				continue;
			if (keepVsVehicles)
			{
				BaseTarget tgt = combat.GetCurrentTarget();
				if (tgt && tgt.GetUnitType() != EAIUnitType.UnitType_Infantry)
					continue;
			}
			SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
			if (!cc)
				continue;
			DCO_ATSelectRifle(cc, combat);
		}
	}

	protected bool DCO_ATNearBuilding(BaseWorld world, vector pos, float radius)
	{
		DCO_ATBuildingProbe probe = new DCO_ATBuildingProbe();
		world.QueryEntitiesBySphere(pos, radius, probe.Collect);
		return probe.m_bFound;
	}

	protected void DCO_ATSelectRifle(SCR_CharacterControllerComponent cc, SCR_AICombatComponent combat)
	{
		if (cc.IsChangingItem())
			return;
		BaseWeaponManagerComponent wm = cc.GetWeaponManagerComponent();
		if (!wm)
			return;
		BaseWeaponComponent pick = combat.FindWeaponOfType(EWeaponType.WT_RIFLE);
		if (!pick)
			pick = combat.FindWeaponOfType(EWeaponType.WT_MACHINEGUN);
		if (!pick)
			pick = combat.FindWeaponOfType(EWeaponType.WT_HANDGUN);
		if (pick && pick != wm.GetCurrent())
			cc.SelectWeapon(pick);
	}
}
