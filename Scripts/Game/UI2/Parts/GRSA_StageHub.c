//! Session-scoped owner of the one shared studio: a single stage core (world, environment rig,
//! dust, glide camera) plus the two subject hosts that populate it — the draft soldier on the
//! floor spot and the draft weapon on the bench. Screens fetch their hosts here so every tab
//! looks into the same world and tab changes become camera moves instead of world rebuilds; the
//! shell ticks the hub every frame and shuts it down when the editor closes.
class GRSA_StageHub
{
	protected static GRSA_StageHub s_Instance;

	protected ref GRSA_StageCore m_Core;
	protected ref GRSA_SoldierStage m_Soldier;
	protected ref GRSA_WeaponStage m_Weapon;

	//------------------------------------------------------------------------------------------------
	//! The shell OWNS the hub and installs it here before its tabs are created. The static stays
	//! a weak lookup: a static ref owning the studio would survive into game teardown on a script
	//! reload with the menu open and trip the engine's resource-leak assert (GameApp fatal).
	static void Install(notnull GRSA_StageHub hub)
	{
		if (s_Instance && s_Instance != hub)
			Shutdown();

		s_Instance = hub;
	}

	//------------------------------------------------------------------------------------------------
	static GRSA_StageHub Get()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	GRSA_StageCore GetCore()
	{
		if (!m_Core)
			m_Core = new GRSA_StageCore();

		return m_Core;
	}

	//------------------------------------------------------------------------------------------------
	GRSA_SoldierStage GetSoldier()
	{
		if (!m_Soldier)
			m_Soldier = new GRSA_SoldierStage(GetCore());

		return m_Soldier;
	}

	//------------------------------------------------------------------------------------------------
	GRSA_WeaponStage GetWeapon()
	{
		if (!m_Weapon)
			m_Weapon = new GRSA_WeaponStage(GetCore());

		return m_Weapon;
	}

	//------------------------------------------------------------------------------------------------
	//! One subject and camera update per frame no matter which tab is open; also keeps the studio dust alive.
	static void Tick(float tDelta)
	{
		if (!s_Instance)
			return;

		if (s_Instance.m_Soldier)
			s_Instance.m_Soldier.Tick(tDelta);
		if (s_Instance.m_Weapon)
			s_Instance.m_Weapon.Tick(tDelta);
		if (s_Instance.m_Core)
			s_Instance.m_Core.Tick(tDelta);
	}

	//------------------------------------------------------------------------------------------------
	//! Dropping the core's world reference deletes the world and everything standing in it.
	static void Shutdown()
	{
		if (!s_Instance)
			return;

		if (s_Instance.m_Soldier)
			s_Instance.m_Soldier.Destroy();
		if (s_Instance.m_Weapon)
			s_Instance.m_Weapon.Destroy();
		if (s_Instance.m_Core)
			s_Instance.m_Core.Release();

		s_Instance = null;
	}
}
