class DCO_StanceUtilState
{
	ref map<IEntity, float> m_LastForceMs = new map<IEntity, float>();
	ref map<IEntity, int> m_GMLockedStance = new map<IEntity, int>();
}

class DCO_StanceUtil
{
	protected static ref DCO_StanceUtilState s_State;

	protected static DCO_StanceUtilState State()
	{
		if (!s_State)
			s_State = new DCO_StanceUtilState();
		return s_State;
	}

	static void SetGMStanceLock(IEntity ent, int stanceOrd)
	{
		if (!ent)
			return;
		if (stanceOrd < 0)
			State().m_GMLockedStance.Remove(ent);
		else
			State().m_GMLockedStance.Set(ent, stanceOrd);
	}

	static bool IsMidMove(IEntity ent, float minSpeed = 0.4)
	{
		if (!ent)
			return false;
		CharacterControllerComponent cc = CharacterControllerComponent.Cast(ent.FindComponent(CharacterControllerComponent));
		if (!cc)
			return false;
		vector vel = cc.GetVelocity();
		vel[1] = 0;
		return vel.LengthSq() > minSpeed * minSpeed;
	}

	// Force a member's stance only if it is not already in that stance and DCO has not forced this member's stance within minIntervalMs.
	static bool TrySetStance(IEntity ent, ECharacterStance desired, float minIntervalMs)
	{
		if (!ent)
			return false;

		if (DCO_PlayerUtil.IsPlayer(ent))
			return false;	// never force a player's stance.

		int gmOrd;
		if (State().m_GMLockedStance.Find(ent, gmOrd) && gmOrd >= 0)
			desired = gmOrd;

		SCR_CharacterControllerComponent cc = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
		if (!cc)
			return false;

		// Already in the desired stance: never re-snap.
		int desiredInt = desired;
		if (cc.GetStance() == desiredInt)
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;
		float now = world.GetWorldTime();
		float last;
		if (State().m_LastForceMs.Find(ent, last) && (now - last) < minIntervalMs)
			return false;	// throttled: don't yank this member's stance again yet.

		if (State().m_LastForceMs.Count() > 2000)
			State().m_LastForceMs.Clear();	// bound memory over a long session.

		// Let the native AI stance path drive the animation transition.
		SCR_AIStanceHandling.SetStance(cc, desired);
		State().m_LastForceMs.Set(ent, now);
		return true;
	}
}
