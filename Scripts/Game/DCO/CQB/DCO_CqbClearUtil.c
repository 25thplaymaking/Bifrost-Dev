// CQB town-clearing geometry/query helpers.
class DCO_CqbClearUtil
{
	static const float CELL_SPACING = 2.4;
	static const float NAV_QUERY_HALF_XZ = 1.05;
	static const float NAV_QUERY_HALF_Y = 2.0;
	static const float CELL_DEDUPE_DIST = 1.5;
	static const float SUPPORT_PROBE_HEIGHT = 1.65;
	static const float SUPPORT_PROBE_DEPTH = 3.75;
	static const float CEILING_PROBE_HEIGHT = 4.5;
	static const float MIN_SUPPORT_NORMAL_Y = 0.55;
	static const float DOOR_SIDE_OFFSET = 1.15;
	static const int MAX_GRID_XZ = 8;
	static const int MAX_GRID_FLOORS = 4;
	static const float SECTOR_LINK_DIST = 4.0;
	static const float PORTAL_LINK_DIST = 4.5;
	static const float FLOOR_BAND = 1.8;
	static const float FLOOR_HEIGHT = 2.8;
	static const float LOS_PROBE_HEIGHT = 1.2;
	static const float DOOR_BLOCK_DIST = 1.1;

	static bool IsClearingActive(SCR_AIGroupUtilityComponent util)
	{
		return util && util.DCO_CqbIsClearingActive();
	}

	static bool LosBlocked(AIPathfindingComponent pf, vector a, vector b, float h)
	{
		if (!pf)
			return true;
		vector hit;
		return pf.RayTrace(a + Vector(0, h, 0), b + Vector(0, h, 0), hit);
	}

	static bool IsSelectedBuildingPart(IEntity entity, IEntity building)
	{
		if (!entity || !building)
			return false;
		if (entity == building)
			return true;
		IEntity entityRoot = entity.GetRootParent();
		IEntity buildingRoot = building.GetRootParent();
		return entityRoot && buildingRoot && entityRoot == buildingRoot;
	}

	static bool PositionBelongsToBuilding(BaseWorld world, IEntity building, vector pos)
	{
		if (!world || !building)
			return false;
		DCO_CqbBuildingTraceFilter filter = new DCO_CqbBuildingTraceFilter(building);

		TraceParam supportProbe = new TraceParam();
		supportProbe.Flags = TraceFlags.ENTS;
		supportProbe.Start = pos + Vector(0, SUPPORT_PROBE_HEIGHT, 0);
		supportProbe.End = pos - Vector(0, SUPPORT_PROBE_DEPTH, 0);
		float travel = world.TraceMove(supportProbe, filter.AcceptSelectedPart);
		if (travel >= 0.999 || !supportProbe.TraceEnt)
			return false;
		if (supportProbe.TraceNorm[1] < MIN_SUPPORT_NORMAL_Y)
			return false;

		// A flat roof can also be navmeshed and supported by this building.
		TraceParam ceilingProbe = new TraceParam();
		ceilingProbe.Flags = TraceFlags.ENTS;
		ceilingProbe.Start = pos + Vector(0, 1.55, 0);
		ceilingProbe.End = pos + Vector(0, CEILING_PROBE_HEIGHT, 0);
		return world.TraceMove(ceilingProbe, filter.AcceptSelectedPart) < 0.999 && ceilingProbe.TraceEnt;
	}

	protected static bool ContainsNearby(array<vector> positions, vector pos)
	{
		float minSq = CELL_DEDUPE_DIST * CELL_DEDUPE_DIST;
		foreach (vector existing : positions)
		{
			if (vector.DistanceSq(existing, pos) <= minSq)
				return true;
		}
		return false;
	}

	// Add positions immediately to both sides of every door in the selected building, tagged as door cells.
	protected static void CollectDoorSideCells(AIPathfindingComponent pf, BaseWorld world, IEntity building,
		vector queryCenter, float queryRadius, array<vector> cells, array<bool> doorFlags, out int doorCount,
		out int candidateCount, out int navCount, out int supportCount)
	{
		DCO_CqbDoorCollector collector = new DCO_CqbDoorCollector();
		world.QueryEntitiesBySphere(queryCenter, queryRadius, collector.Collect);
		foreach (ScriptedUserAction action : collector.m_aActions)
		{
			if (!action)
				continue;
			IEntity owner = action.GetOwner();
			if (!owner || !IsSelectedBuildingPart(owner, building))
				continue;
			doorCount++;

			vector transform[4];
			owner.GetWorldTransform(transform);
			vector axisX = transform[0];
			vector axisZ = transform[2];
			axisX[1] = 0;
			axisZ[1] = 0;
			if (axisX.LengthSq() > 0.01)
				axisX.Normalize();
			if (axisZ.LengthSq() > 0.01)
				axisZ.Normalize();

			array<vector> axes = {axisX, -axisX, axisZ, -axisZ};
			foreach (vector axis : axes)
			{
				if (axis.LengthSq() < 0.5)
					continue;
				candidateCount++;
				vector snapped;
				if (!pf.GetClosestPositionOnNavmesh(owner.GetOrigin() + axis * DOOR_SIDE_OFFSET,
					Vector(0.7, NAV_QUERY_HALF_Y, 0.7), snapped))
					continue;
				navCount++;
				if (!PositionBelongsToBuilding(world, building, snapped))
					continue;
				supportCount++;
				if (!ContainsNearby(cells, snapped))
				{
					cells.Insert(snapped);
					doorFlags.Insert(true);
				}
			}
		}
	}

	// Build deterministic room-scale coverage cells plus tagged door-side cells.
	static EDCO_CqbSurveyResult CollectInteriorCells(AIPathfindingComponent pf, IEntity building,
		out array<vector> cells, out array<bool> doorFlags)
	{
		cells.Clear();
		doorFlags.Clear();
		BaseWorld world = GetGame().GetWorld();
		if (!world || !pf || !building)
			return EDCO_CqbSurveyResult.INVALID;

		NavmeshWorldComponent navmesh = pf.GetNavmeshComponent();
		if (!navmesh)
			return EDCO_CqbSurveyResult.INVALID;

		vector mins;
		vector maxs;
		building.GetBounds(mins, maxs);
		float sizeX = Math.AbsFloat(maxs[0] - mins[0]);
		float sizeY = Math.AbsFloat(maxs[1] - mins[1]);
		float sizeZ = Math.AbsFloat(maxs[2] - mins[2]);
		if (sizeX < 0.5 || sizeY < 0.5 || sizeZ < 0.5)
			return EDCO_CqbSurveyResult.READY;

		int countX = Math.ClampInt(Math.Ceil(sizeX / CELL_SPACING), 1, MAX_GRID_XZ);
		int countZ = Math.ClampInt(Math.Ceil(sizeZ / CELL_SPACING), 1, MAX_GRID_XZ);
		int countY = Math.ClampInt(Math.Ceil(sizeY / FLOOR_HEIGHT), 1, MAX_GRID_FLOORS);
		bool waiting = false;

		// Request all tiles as one batch.
		for (int y = 0; y < countY; y++)
		{
			for (int x = 0; x < countX; x++)
			{
				for (int z = 0; z < countZ; z++)
				{
					vector localPos = Vector(
						mins[0] + sizeX * ((x + 0.5) / countX),
						mins[1] + sizeY * ((y + 0.5) / countY),
						mins[2] + sizeZ * ((z + 0.5) / countZ));
					vector queryPos = building.CoordToParent(localPos);
					if (!navmesh.IsTileLoaded(queryPos))
					{
						navmesh.LoadTileIn(queryPos);
						waiting = true;
					}
				}
			}
		}

		if (waiting)
			return EDCO_CqbSurveyResult.WAITING;

		int sampleCount = 0;
		int navCount = 0;
		int supportCount = 0;
		for (int sy = 0; sy < countY; sy++)
		{
			for (int sx = 0; sx < countX; sx++)
			{
				for (int sz = 0; sz < countZ; sz++)
				{
					sampleCount++;
					vector sampleLocal = Vector(
						mins[0] + sizeX * ((sx + 0.5) / countX),
						mins[1] + sizeY * ((sy + 0.5) / countY),
						mins[2] + sizeZ * ((sz + 0.5) / countZ));
					vector sampleWorld = building.CoordToParent(sampleLocal);
					vector snapped;
					if (!pf.GetClosestPositionOnNavmesh(sampleWorld,
						Vector(NAV_QUERY_HALF_XZ, NAV_QUERY_HALF_Y, NAV_QUERY_HALF_XZ), snapped))
						continue;
					navCount++;
					if (!PositionBelongsToBuilding(world, building, snapped))
						continue;
					supportCount++;
					if (!ContainsNearby(cells, snapped))
					{
						cells.Insert(snapped);
						doorFlags.Insert(false);
					}
				}
			}
		}

		vector localCenter = (mins + maxs) * 0.5;
		vector queryCenter = building.CoordToParent(localCenter);
		float queryRadius = Math.Sqrt(sizeX * sizeX + sizeY * sizeY + sizeZ * sizeZ) * 0.5 + 3.0;
		int doorCount = 0;
		int doorCandidates = 0;
		int doorNav = 0;
		int doorSupport = 0;
		CollectDoorSideCells(pf, world, building, queryCenter, queryRadius, cells, doorFlags,
			doorCount, doorCandidates, doorNav, doorSupport);

		if (DCO_MoraleSettings.Get().m_bDebugCqbClear)
		{
			Print(string.Format("[DCO-WPI] CQB survey geometry: bounds %1 x %2 x %3; grid %4 x %5 x %6; samples=%7 nav=%8 support=%9",
				sizeX, sizeY, sizeZ, countX, countY, countZ, sampleCount, navCount, supportCount), LogLevel.NORMAL);
			Print(string.Format("[DCO-WPI] CQB survey portals: doors=%1 candidates=%2 nav=%3 support=%4; final distinct cells=%5",
				doorCount, doorCandidates, doorNav, doorSupport, cells.Count()), LogLevel.NORMAL);
		}

		return EDCO_CqbSurveyResult.READY;
	}

	static void BuildPlan(AIPathfindingComponent pf, array<vector> cells, array<bool> doorFlags, vector fromPos, DCO_CqbPlan plan)
	{
		plan.m_aCells.Clear();
		plan.m_aSectors.Clear();
		if (!cells || !doorFlags || cells.Count() != doorFlags.Count())
			return;
		for (int i = 0; i < cells.Count(); i++)
		{
			DCO_CqbCell cell = new DCO_CqbCell();
			cell.m_vPos = cells[i];
			cell.m_bDoor = doorFlags[i];
			plan.m_aCells.Insert(cell);
		}
		int total = plan.m_aCells.Count();

		array<vector> doorPositions = {};
		for (int dp = 0; dp < total; dp++)
		{
			if (plan.m_aCells[dp].m_bDoor)
				doorPositions.Insert(plan.m_aCells[dp].m_vPos);
		}
		array<int> parent = {};
		for (int p = 0; p < total; p++)
			parent.Insert(p);
		float linkSq = SECTOR_LINK_DIST * SECTOR_LINK_DIST;
		for (int a = 0; a < total; a++)
		{
			if (plan.m_aCells[a].m_bDoor)
				continue;
			for (int b = a + 1; b < total; b++)
			{
				if (plan.m_aCells[b].m_bDoor)
					continue;
				vector pa = plan.m_aCells[a].m_vPos;
				vector pb = plan.m_aCells[b].m_vPos;
				if (Math.AbsFloat(pa[1] - pb[1]) > FLOOR_BAND)
					continue;
				if (vector.DistanceSq(pa, pb) > linkSq)
					continue;
				int rootA = FindRoot(parent, a);
				int rootB = FindRoot(parent, b);
				if (rootA == rootB)
					continue;
				if (LosBlocked(pf, pa, pb, LOS_PROBE_HEIGHT))
					continue;
				if (DoorBetween(doorPositions, pa, pb))
					continue;
				parent[rootA] = rootB;
			}
		}

		float baseY = 0;
		bool baseSet = false;
		for (int c = 0; c < total; c++)
		{
			DCO_CqbCell baseCell = plan.m_aCells[c];
			if (baseCell.m_bDoor)
				continue;
			if (!baseSet || baseCell.m_vPos[1] < baseY)
			{
				baseY = baseCell.m_vPos[1];
				baseSet = true;
			}
		}
		array<ref DCO_CqbSector> rawSectors = {};
		array<int> rootOf = {};
		array<vector> centroids = {};
		for (int c2 = 0; c2 < total; c2++)
		{
			DCO_CqbCell roomCell = plan.m_aCells[c2];
			if (roomCell.m_bDoor)
				continue;
			int root = FindRoot(parent, c2);
			int sIdx = rootOf.Find(root);
			if (sIdx < 0)
			{
				DCO_CqbSector newSector = new DCO_CqbSector();
				newSector.m_vMins = roomCell.m_vPos;
				newSector.m_vMaxs = roomCell.m_vPos;
				rawSectors.Insert(newSector);
				rootOf.Insert(root);
				centroids.Insert(vector.Zero);
				sIdx = rawSectors.Count() - 1;
			}
			DCO_CqbSector memberSector = rawSectors[sIdx];
			memberSector.m_aCellIdx.Insert(c2);
			for (int axis = 0; axis < 3; axis++)
			{
				if (roomCell.m_vPos[axis] < memberSector.m_vMins[axis])
					memberSector.m_vMins[axis] = roomCell.m_vPos[axis];
				if (roomCell.m_vPos[axis] > memberSector.m_vMaxs[axis])
					memberSector.m_vMaxs[axis] = roomCell.m_vPos[axis];
			}
			centroids[sIdx] = centroids[sIdx] + roomCell.m_vPos;
		}
		if (rawSectors.IsEmpty())
			return;
		for (int s2 = 0; s2 < rawSectors.Count(); s2++)
		{
			DCO_CqbSector sizedSector = rawSectors[s2];
			int memberCount = sizedSector.m_aCellIdx.Count();
			if (memberCount > 0)
				centroids[s2] = centroids[s2] * (1.0 / memberCount);
			sizedSector.m_iFloor = Math.Round((centroids[s2][1] - baseY) / FLOOR_HEIGHT);
			sizedSector.m_vMins = sizedSector.m_vMins - Vector(0.6, 0.3, 0.6);
			sizedSector.m_vMaxs = sizedSector.m_vMaxs + Vector(0.6, 1.7, 0.6);
		}
		BuildPortalSequence(pf, plan, rawSectors, centroids, fromPos, total);
	}

	protected static void BuildPortalSequence(AIPathfindingComponent pf, DCO_CqbPlan plan,
		array<ref DCO_CqbSector> rawSectors, array<vector> centroids, vector fromPos, int total)
	{
		// Portals: attach each door cell to every sector it can see into.
		float portalSq = PORTAL_LINK_DIST * PORTAL_LINK_DIST;
		array<int> doorLinks = {};
		for (int dInit = 0; dInit < total; dInit++)
			doorLinks.Insert(0);
		for (int d = 0; d < total; d++)
		{
			if (!plan.m_aCells[d].m_bDoor)
				continue;
			vector dPos = plan.m_aCells[d].m_vPos;
			for (int ds = 0; ds < rawSectors.Count(); ds++)
			{
				DCO_CqbSector portalSector = rawSectors[ds];
				bool linked = false;
				foreach (int cellIdx : portalSector.m_aCellIdx)
				{
					vector cPos = plan.m_aCells[cellIdx].m_vPos;
					if (Math.AbsFloat(cPos[1] - dPos[1]) > FLOOR_BAND)
						continue;
					if (vector.DistanceSq(cPos, dPos) > portalSq)
						continue;
					if (LosBlocked(pf, dPos, cPos, LOS_PROBE_HEIGHT))
						continue;
					linked = true;
					break;
				}
				if (linked)
				{
					portalSector.m_aPortalIdx.Insert(d);
					doorLinks[d] = doorLinks[d] + 1;
				}
			}
		}

		int count = rawSectors.Count();
		array<bool> visited = {};
		for (int v = 0; v < count; v++)
			visited.Insert(false);
		int startIdx = -1;
		float startBest = 0;
		for (int st = 0; st < count; st++)
		{
			bool exterior = false;
			foreach (int pIdx : rawSectors[st].m_aPortalIdx)
			{
				if (doorLinks[pIdx] == 1)
				{
					exterior = true;
					break;
				}
			}
			float dSq = vector.DistanceSq(centroids[st], fromPos);
			if (!exterior)
				dSq += 250000.0;	// interior starts only when no entrance exists at all.
			if (startIdx < 0 || dSq < startBest)
			{
				startIdx = st;
				startBest = dSq;
			}
		}

		array<int> order = {};
		int cur = startIdx;
		while (cur >= 0)
		{
			visited[cur] = true;
			order.Insert(cur);
			int next = -1;
			float nextBest = 0;
			for (int cand = 0; cand < count; cand++)
			{
				if (visited[cand])
					continue;
				float score = vector.DistanceSq(centroids[cur], centroids[cand]);
				score += 10000.0 * Math.AbsInt(rawSectors[cur].m_iFloor - rawSectors[cand].m_iFloor);
				bool adjacent = false;
				foreach (int myPortal : rawSectors[cur].m_aPortalIdx)
				{
					if (rawSectors[cand].m_aPortalIdx.Find(myPortal) >= 0)
					{
						adjacent = true;
						break;
					}
				}
				if (!adjacent)
					score += 60000.0;	// clear the hallway-connected room next; never hop across the plan.
				if (next < 0 || score < nextBest)
				{
					next = cand;
					nextBest = score;
				}
			}
			cur = next;
		}

		// Rebuild in visit order and remap cell ownership to final indexes.
		foreach (int rawIdx : order)
		{
			DCO_CqbSector orderedSector = rawSectors[rawIdx];
			int finalIdx = plan.m_aSectors.Count();
			plan.m_aSectors.Insert(orderedSector);
			foreach (int ownedIdx : orderedSector.m_aCellIdx)
				plan.m_aCells[ownedIdx].m_iSector = finalIdx;
		}

		// Entry portals + entry-near-first cell order, resolved along the sequence.
		for (int f = 0; f < plan.m_aSectors.Count(); f++)
		{
			DCO_CqbSector entrySector = plan.m_aSectors[f];
			vector anchor = fromPos;
			if (f > 0)
				anchor = SectorCentroid(plan, f - 1);

			int entry = -1;
			float entryBest = 0;
			foreach (int portalIdx : entrySector.m_aPortalIdx)
			{
				bool usable = false;
				if (f == 0)
				{
					usable = doorLinks[portalIdx] == 1;
				}
				else
				{
					for (int e = 0; e < f; e++)
					{
						if (plan.m_aSectors[e].m_aPortalIdx.Find(portalIdx) >= 0)
						{
							usable = true;
							break;
						}
					}
				}
				if (!usable)
					continue;
				float pdSq = vector.DistanceSq(plan.m_aCells[portalIdx].m_vPos, anchor);
				if (entry < 0 || pdSq < entryBest)
				{
					entry = portalIdx;
					entryBest = pdSq;
				}
			}
			if (entry < 0)
			{
				foreach (int fallbackIdx : entrySector.m_aPortalIdx)
				{
					float pdSq2 = vector.DistanceSq(plan.m_aCells[fallbackIdx].m_vPos, anchor);
					if (entry < 0 || pdSq2 < entryBest)
					{
						entry = fallbackIdx;
						entryBest = pdSq2;
					}
				}
			}
			entrySector.m_iEntryPortal = entry;

			vector cellAnchor = anchor;
			if (entry >= 0)
				cellAnchor = plan.m_aCells[entry].m_vPos;
			SortCellsByDistance(plan, entrySector.m_aCellIdx, cellAnchor);
		}
	}

	// Centroid of an already-ordered sector's member cells.
	static vector SectorCentroid(DCO_CqbPlan plan, int sectorIdx)
	{
		DCO_CqbSector sector = plan.m_aSectors[sectorIdx];
		vector sum = vector.Zero;
		if (!sector || sector.m_aCellIdx.IsEmpty())
			return sum;
		foreach (int cellIdx : sector.m_aCellIdx)
			sum = sum + plan.m_aCells[cellIdx].m_vPos;
		return sum * (1.0 / sector.m_aCellIdx.Count());
	}

	protected static void SortCellsByDistance(DCO_CqbPlan plan, array<int> cellIdx, vector anchor)
	{
		for (int i = 1; i < cellIdx.Count(); i++)
		{
			int key = cellIdx[i];
			float keySq = vector.DistanceSq(plan.m_aCells[key].m_vPos, anchor);
			int j = i - 1;
			while (j >= 0 && vector.DistanceSq(plan.m_aCells[cellIdx[j]].m_vPos, anchor) > keySq)
			{
				cellIdx[j + 1] = cellIdx[j];
				j--;
			}
			cellIdx[j + 1] = key;
		}
	}

	protected static int FindRoot(array<int> parent, int i)
	{
		while (parent[i] != i)
		{
			parent[i] = parent[parent[i]];
			i = parent[i];
		}
		return i;
	}

	protected static bool DoorBetween(array<vector> doorPositions, vector a, vector b)
	{
		vector ab = b - a;
		ab[1] = 0;
		float abLenSq = ab.LengthSq();
		if (abLenSq < 0.01)
			return false;
		float blockSq = DOOR_BLOCK_DIST * DOOR_BLOCK_DIST;
		foreach (vector d : doorPositions)
		{
			if (Math.AbsFloat(d[1] - (a[1] + b[1]) * 0.5) > FLOOR_BAND)
				continue;
			vector ad = d - a;
			float t = (ad[0] * ab[0] + ad[2] * ab[2]) / abLenSq;
			if (t < 0.1 || t > 0.9)
				continue;
			vector closest = a + ab * t;
			float dx = d[0] - closest[0];
			float dz = d[2] - closest[2];
			if (dx * dx + dz * dz < blockSq)
				return true;
		}
		return false;
	}

	static bool GetLiveThreatInSector(SCR_AIGroupPerception perception, DCO_CqbSector sector, float margin, out vector outPos)
	{
		if (!perception || !perception.m_aTargets || !sector)
			return false;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return false;
		float now = pm.GetTime();
		foreach (SCR_AITargetInfo info : perception.m_aTargets)
		{
			if (!DCO_ContactUtil.IsLive(info, now, DCO_ContactUtil.FRESH_CONTACT_S))
				continue;
			vector pos = info.m_vWorldPos;
			if (pos[0] >= sector.m_vMins[0] - margin && pos[0] <= sector.m_vMaxs[0] + margin
				&& pos[1] >= sector.m_vMins[1] - margin && pos[1] <= sector.m_vMaxs[1] + margin
				&& pos[2] >= sector.m_vMins[2] - margin && pos[2] <= sector.m_vMaxs[2] + margin)
			{
				outPos = pos;
				return true;
			}
		}
		return false;
	}

	static bool GetLiveThreatNearCells(SCR_AIGroupPerception perception, array<ref DCO_CqbCell> cells, float radius, out vector outPos)
	{
		if (!perception || !perception.m_aTargets || cells.IsEmpty())
			return false;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return false;
		float now = pm.GetTime();
		float radiusSq = radius * radius;
		float bestSq = 1000000000.0;
		bool found = false;

		foreach (SCR_AITargetInfo info : perception.m_aTargets)
		{
			if (!DCO_ContactUtil.IsLive(info, now, DCO_ContactUtil.FRESH_CONTACT_S))
				continue;
			foreach (DCO_CqbCell cell : cells)
			{
				if (!cell)
					continue;
				float distSq = vector.DistanceSq(info.m_vWorldPos, cell.m_vPos);
				if (distSq > radiusSq || distSq >= bestSq)
					continue;
				bestSq = distSq;
				outPos = info.m_vWorldPos;
				found = true;
			}
		}
		return found;
	}


	// Close-range deadlock assist only.
	static int OpenNearbyDoors(BaseWorld world, AIAgent agent, float radius)
	{
		if (!world || !agent)
			return 0;
		IEntity performer = agent.GetControlledEntity();
		if (!performer || DCO_PlayerUtil.IsPlayer(performer))
			return 0;
		return OpenDoorsAtPosition(world, performer, performer.GetOrigin(), radius);
	}

	// True while a standard door within radius of pos is still closed and not yet swinging.
	static bool AnyClosedDoorAtPosition(BaseWorld world, vector pos, float radius)
	{
		if (!world)
			return false;
		DCO_CqbDoorCollector collector = new DCO_CqbDoorCollector();
		world.QueryEntitiesBySphere(pos, radius, collector.Collect);
		float radiusSq = radius * radius;
		foreach (ScriptedUserAction action : collector.m_aActions)
		{
			if (!action)
				continue;
			IEntity actionOwner = action.GetOwner();
			if (!actionOwner || vector.DistanceSq(actionOwner.GetOrigin(), pos) > radiusSq)
				continue;
			DoorComponent door = DoorComponent.Cast(actionOwner.FindComponent(DoorComponent));
			if (!door)
				continue;
			if (Math.AbsFloat(door.GetNormalizedDoorState()) < 0.6 && !door.IsOpening())
				return true;
		}
		return false;
	}

	static void OrderCellsForBreach(DCO_CqbPlan plan, DCO_CqbSector sector, vector portalPos, vector inward, vector side)
	{
		if (!plan || !sector || sector.m_aCellIdx.Count() < 2)
			return;

		array<int> leftIdx = {};
		array<float> leftDepth = {};
		array<int> rightIdx = {};
		array<float> rightDepth = {};
		foreach (int cellIdx : sector.m_aCellIdx)
		{
			vector rel = plan.m_aCells[cellIdx].m_vPos - portalPos;
			float depth = rel[0] * inward[0] + rel[2] * inward[2];
			float lateral = rel[0] * side[0] + rel[2] * side[2];
			float score = depth - Math.AbsFloat(lateral) * 0.75;
			if (lateral < 0)
			{
				InsertByDepth(leftIdx, leftDepth, cellIdx, score);
			}
			else
			{
				InsertByDepth(rightIdx, rightDepth, cellIdx, score);
			}
		}

		sector.m_aCellIdx.Clear();
		int l = 0;
		int r = 0;
		while (l < leftIdx.Count() || r < rightIdx.Count())
		{
			if (l < leftIdx.Count())
			{
				sector.m_aCellIdx.Insert(leftIdx[l]);
				l++;
			}
			if (r < rightIdx.Count())
			{
				sector.m_aCellIdx.Insert(rightIdx[r]);
				r++;
			}
		}
	}

	protected static void InsertByDepth(array<int> indexes, array<float> depths, int cellIdx, float depth)
	{
		int at = indexes.Count();
		for (int i = 0; i < depths.Count(); i++)
		{
			if (depth < depths[i])
			{
				at = i;
				break;
			}
		}
		indexes.InsertAt(cellIdx, at);
		depths.InsertAt(depth, at);
	}

	static int OpenDoorsAtPosition(BaseWorld world, IEntity performer, vector pos, float radius)
	{
		if (!world || !performer || DCO_PlayerUtil.IsPlayer(performer))
			return 0;

		DCO_CqbDoorCollector collector = new DCO_CqbDoorCollector();
		world.QueryEntitiesBySphere(pos, radius, collector.Collect);
		int opened = 0;
		float radiusSq = radius * radius;
		foreach (ScriptedUserAction action : collector.m_aActions)
		{
			if (!action)
				continue;
			IEntity actionOwner = action.GetOwner();
			if (!actionOwner || vector.DistanceSq(actionOwner.GetOrigin(), pos) > radiusSq)
				continue;
			DoorComponent door = DoorComponent.Cast(actionOwner.FindComponent(DoorComponent));
			if (!door)
				continue;
			bool fullyOpen = Math.AbsFloat(door.GetNormalizedDoorState()) > 0.99;
			if (fullyOpen || door.IsOpening())
				continue;
			action.PerformAction(actionOwner, performer);
			opened++;
		}
		return opened;
	}
}

// Entity-trace callback scoped to one selected building.
class DCO_CqbBuildingTraceFilter
{
	protected IEntity m_Building;

	void DCO_CqbBuildingTraceFilter(IEntity building)
	{
		m_Building = building;
	}

	bool AcceptSelectedPart(IEntity entity)
	{
		return DCO_CqbClearUtil.IsSelectedBuildingPart(entity, m_Building);
	}
}

// Query helper for close-range door actions.
class DCO_CqbDoorCollector
{
	ref array<ScriptedUserAction> m_aActions = {};

	bool Collect(IEntity entity)
	{
		if (!entity)
			return true;
		ScriptedUserAction action = SCR_AIOpenDoor.FindDoorUserAction(entity);
		if (action && m_aActions.Find(action) < 0)
			m_aActions.Insert(action);
		return true;
	}
}
