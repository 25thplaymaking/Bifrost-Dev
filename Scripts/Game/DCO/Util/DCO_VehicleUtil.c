// Vehicle-crew detection helper.
class DCO_VehicleUtil
{
	static bool IsGroupInVehicle(SCR_AIGroup group)
	{
		if (!group)
			return false;

		IEntity leader = group.GetLeaderEntity();
		if (leader && CompartmentAccessComponent.GetVehicleIn(leader) != null)
			return true;

		// Robust path: any member mounted.
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;
			IEntity member = agent.GetControlledEntity();
			if (member && CompartmentAccessComponent.GetVehicleIn(member) != null)
				return true;
		}

		return false;
	}

	static void OrderGroupMoveToEntity(SCR_AIGroup group, IEntity target, AICommunicationComponent comms, EMovementType moveType = EMovementType.RUN)
	{
		if (!group || !target || !comms)
			return;

		AIAgent leaderAgent = group.GetLeaderAgent();
		if (!leaderAgent)
			return;

		if (IsGroupInVehicle(group))
		{
			SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(target, target.GetOrigin(), moveType, true, null);
			if (msg)
			{
				comms.RequestBroadcast(msg);
				return;
			}
		}

		SCR_AIMessageHandling.SendMoveMessage(leaderAgent, target, null, comms);
	}

	static void OrderGroupMoveToPosition(SCR_AIGroup group, vector pos, AICommunicationComponent comms, EMovementType moveType = EMovementType.RUN)
	{
		if (!group || !comms)
			return;

		AIAgent leaderAgent = group.GetLeaderAgent();
		if (!leaderAgent)
			return;

		SCR_AIMessage_Move msg = SCR_AIMessage_Move.Create(null, pos, moveType, true, null);
		if (msg)
			comms.RequestBroadcast(msg);
	}
}
