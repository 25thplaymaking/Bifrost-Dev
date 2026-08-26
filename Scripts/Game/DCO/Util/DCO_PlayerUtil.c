// Player guard.
class DCO_PlayerUtil
{
	// True if the entity is currently controlled by a player.
	static bool IsPlayer(IEntity ent)
	{
		if (!ent)
			return false;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return false;
		return pm.GetPlayerIdFromControlledEntity(ent) != 0;
	}
}
