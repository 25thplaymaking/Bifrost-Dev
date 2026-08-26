modded class SCR_AIMoveActivity
{
	override void InitParameters(vector position, IEntity entity, EMovementType movementType, bool useVehicles, float priorityLevel)
	{
		EMovementType chosen = movementType;
		vector chosenPos = position;

		if (m_Utility && DCO_VehicleUtil.IsGroupInVehicle(SCR_AIGroup.Cast(m_Utility.GetOwner())))
		{
			super.InitParameters(position, entity, movementType, useVehicles, priorityLevel);
			return;
		}

		DCO_TacticalMoveSettings cfg = DCO_TacticalMoveSettings.Get();

		if (cfg && cfg.m_bEnableTacticalMove && Replication.IsServer())
		{
			float threat = -1;
			float dist = -1;
			vector groupPos = vector.Zero;
			bool haveGroup = false;
			if (m_Utility)
			{
				threat = m_Utility.GetThreatMeasure();
				IEntity groupOwner = m_Utility.GetOwner();
				if (groupOwner)
				{
					groupPos = groupOwner.GetOrigin();
					haveGroup = true;
					dist = vector.Distance(groupPos, position);
				}
			}

			bool threatOk = cfg.m_bForceIgnoreThreat || threat >= cfg.m_fThreatActivation;
			bool longMove = dist >= cfg.m_fMinMoveDistance;

			if (movementType == EMovementType.RUN && !entity && m_Utility && threatOk && longMove)
				chosen = EMovementType.WALK;

			if (cfg.m_bAvoidThreatFunnel && !entity && haveGroup && threatOk && longMove)
			{
				vector threatPos;
				SCR_AIGroupUtilityComponent groupUtil = m_Utility;
				if (groupUtil && groupUtil.DCO_GetThreatOrLastPosition(threatPos))
					chosenPos = DCO_SidestepFunnel(groupPos, position, threatPos, cfg);
			}

			if (cfg.m_bEnableFlanking && !entity && haveGroup && threatOk)
			{
				vector flThreat;
				SCR_AIGroupUtilityComponent flUtil = m_Utility;
				if (flUtil && flUtil.DCO_GetThreatOrLastPosition(flThreat))
					chosenPos = DCO_FlankApproach(groupPos, chosenPos, flThreat, cfg);
			}

			if (cfg.m_bEnableExposureScoring && !entity && haveGroup && threatOk && longMove)
			{
				vector exThreat;
				SCR_AIGroupUtilityComponent exUtil = m_Utility;
				if (exUtil && exUtil.DCO_GetThreatOrLastPosition(exThreat))
					chosenPos = DCO_PickLeastExposed(groupPos, chosenPos, exThreat, cfg);
			}

			if (cfg.m_bDebugLog)
				Print(string.Format("[DCO TacMove] init: inType=%1 hasEntity=%2 threat=%3 dist=%4 -> outType=%5 posMoved=%6 (0=IDLE 1=WALK 2=RUN 3=SPRINT)",
					movementType, entity != null, threat, dist, chosen, vector.Distance(chosenPos, position) > 0.1), LogLevel.NORMAL);
		}

		if (cfg && cfg.m_bEnableProceduralPath && !entity && m_Utility && Replication.IsServer())
			m_Utility.DCO_SetPathDestination(chosenPos);

		super.InitParameters(chosenPos, entity, chosen, useVehicles, priorityLevel);
	}

	protected vector DCO_FlankApproach(vector groupPos, vector dest, vector threatPos, DCO_TacticalMoveSettings cfg)
	{
		vector destToEnemy = threatPos - dest;
		destToEnemy[1] = 0;
		float destEnemyDist = destToEnemy.Length();
		if (destEnemyDist > cfg.m_fFlankMaxEnemyDist)
			return dest;

		// Vector from enemy back out to the group's current side, flattened.
		vector enemyToGroup = groupPos - threatPos;
		enemyToGroup[1] = 0;
		float standoff = enemyToGroup.Length();
		if (standoff < cfg.m_fFlankMinEnemyDist)
			return dest;	// already on top of the enemy - no room/benefit to flank.
		vector dir = enemyToGroup / standoff;	// unit, enemy to group.

		float angleRad = cfg.m_fFlankAngleDeg * 0.0174533;
		float s = Math.Sin(angleRad);
		float c = Math.Cos(angleRad);

		vector left  = Vector(dir[0] * c - dir[2] * s, 0, dir[0] * s + dir[2] * c);
		vector right = Vector(dir[0] * c + dir[2] * s, 0, -dir[0] * s + dir[2] * c);

		vector leftDest  = threatPos + left  * standoff;
		vector rightDest = threatPos + right * standoff;

		// Choose the flank that the enemy is less likely to be watching: the one less line-of-sight exposed to the threat.
		BaseWorld world = GetGame().GetWorld();
		if (world)
		{
			bool leftCovered  = DCO_IsCovered(world, threatPos, leftDest, cfg);
			bool rightCovered = DCO_IsCovered(world, threatPos, rightDest, cfg);
			if (rightCovered && !leftCovered)
				return rightDest;
			if (leftCovered && !rightCovered)
				return leftDest;
		}

		if (vector.DistanceSq(leftDest, groupPos) <= vector.DistanceSq(rightDest, groupPos))
			return leftDest;
		return rightDest;
	}

	protected vector DCO_SidestepFunnel(vector groupPos, vector dest, vector threatPos, DCO_TacticalMoveSettings cfg)
	{
		vector toDest = dest - groupPos;
		toDest[1] = 0;
		float legLen = toDest.Length();
		if (legLen < 1.0)
			return dest;

		vector dir = toDest / legLen;

		vector toThreat = threatPos - groupPos;
		toThreat[1] = 0;

		// How far along the leg the threat projects.
		float proj = vector.Dot(toThreat, dir);
		if (proj <= 0 || proj >= legLen)
			return dest;

		// Perpendicular offset of the threat from the path.
		vector perpVec = toThreat - dir * proj;
		float perpDist = perpVec.Length();
		if (perpDist > cfg.m_fFunnelCorridorHalfWidth)
			return dest;

		// Sidestep direction = perpendicular to the leg, pointing away from the threat.
		vector side;
		if (perpDist > 0.01)
			side = perpVec / perpDist * -1.0;	// away from where the threat offsets the path.
		else
			side = Vector(-dir[2], 0, dir[0]);	// arbitrary but consistent perpendicular.

		return dest + side * cfg.m_fFunnelSidestep;
	}

	// Return the least line-of-sight-exposed of: the chosen destination + two lateral alternatives.
	protected vector DCO_PickLeastExposed(vector groupPos, vector baseDest, vector threatPos, DCO_TacticalMoveSettings cfg)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return baseDest;

		// If the base destination is already hidden from the threat, keep it.
		if (DCO_IsCovered(world, threatPos, baseDest, cfg))
			return baseDest;

		vector toDest = baseDest - groupPos;
		toDest[1] = 0;
		float len = toDest.Length();
		if (len < 1.0)
			return baseDest;

		vector dir = toDest / len;
		vector perp = Vector(-dir[2], 0, dir[0]);
		float off = cfg.m_fExposureSidestep;

		// Take the first lateral alternative the threat can't see.
		vector left = baseDest + perp * off;
		if (DCO_IsCovered(world, threatPos, left, cfg))
			return left;

		vector right = baseDest - perp * off;
		if (DCO_IsCovered(world, threatPos, right, cfg))
			return right;

		return baseDest;	// nothing better - keep the chosen destination.
	}

	protected bool DCO_IsCovered(BaseWorld world, vector threatPos, vector pos, DCO_TacticalMoveSettings cfg)
	{
		vector eye = Vector(0, 1.5, 0);	// rough eye height for both endpoints.
		TraceParam param = new TraceParam();
		param.Start = threatPos + eye;
		param.End = pos + eye;
		param.LayerMask = cfg.m_iExposureLayerMask;
		float frac = world.TraceMove(param, null);
		return frac < 0.99;	// hit geometry before reaching pos = covered from the threat.
	}
}
