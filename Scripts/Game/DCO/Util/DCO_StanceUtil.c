class DCO_StanceUtil
{
	protected static ref map<IEntity, float> s_LastForceMs = new map<IEntity, float>();

	protected static ref map<IEntity, int> s_GMLockedStance = new map<IEntity, int>();

	static void SetGMStanceLock(IEntity ent, int stanceOrd)
	{
		if (!ent)
			return;
		if (stanceOrd < 0)
			s_GMLockedStance.Remove(ent);
		else
			s_GMLockedStance.Set(ent, stanceOrd);
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
		if (s_GMLockedStance.Find(ent, gmOrd) && gmOrd >= 0)
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
		if (s_LastForceMs.Find(ent, last) && (now - last) < minIntervalMs)
			return false;	// throttled: don't yank this member's stance again yet.

		if (s_LastForceMs.Count() > 2000)
			s_LastForceMs.Clear();	// bound memory over a long session.

		// BUG FIX: stances were silently broken - AI would not hold a forced stance regardless of settings.
		int stanceChange = ECharacterStanceChange.STANCECHANGE_NONE;
		switch (desired)
		{
			case ECharacterStance.STAND:	stanceChange = ECharacterStanceChange.STANCECHANGE_TOERECTED;	break;
			case ECharacterStance.CROUCH:	stanceChange = ECharacterStanceChange.STANCECHANGE_TOCROUCH;	break;
			case ECharacterStance.PRONE:	stanceChange = ECharacterStanceChange.STANCECHANGE_TOPRONE;		break;
		}
		if (stanceChange == ECharacterStanceChange.STANCECHANGE_NONE)
			return false;	// unmapped stance: do nothing rather than issue a silent no-op change.

		cc.SetStanceChange(stanceChange);
		s_LastForceMs.Set(ent, now);
		return true;
	}
}
