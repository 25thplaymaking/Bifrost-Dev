// ALL-PLAYERS FPS MONITOR.

// Runs on EVERY client: answers server polls by measuring + reporting.
class DCO_FpsMonitorClient
{
	protected static ref DCO_FpsMonitorClient s_Inst;

	static const int MEASURE_MS   = 2000;	// frame-count window per poll.
	static const int KEEPALIVE_MS = 20000;
	static const int STALE_MS     = 30000;	// GM-side: a sample older than this stops displaying.

	protected int  m_Frames;	// frame counter for the active measure window.
	protected bool m_bMeasuring;

	protected bool m_bGlobal;
	protected ref map<int, bool>  m_Watched = new map<int, bool>();
	protected bool m_bKeepaliveOn;
	protected ref map<int, int>   m_Fps     = new map<int, int>();	// playerId -> last reported FPS.
	protected ref map<int, float> m_FpsTime = new map<int, float>();

	static DCO_FpsMonitorClient Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_FpsMonitorClient();
		return s_Inst;
	}

	void BeginMeasure()
	{
		if (m_bMeasuring)
			return;	// a window is already running; this poll rides it.
		m_bMeasuring = true;
		m_Frames = 0;
		GetGame().GetCallqueue().CallLater(CountFrame, 0, true);	// delay 0 + repeat = once per frame.
		GetGame().GetCallqueue().CallLater(EndMeasure, MEASURE_MS, false);
	}

	protected void CountFrame()
	{
		m_Frames++;
	}

	protected void EndMeasure()
	{
		GetGame().GetCallqueue().Remove(CountFrame);
		m_bMeasuring = false;
		int fps = Math.Round(m_Frames * 1000.0 / MEASURE_MS);

		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return;
		if (Replication.IsServer())
			DCO_FpsMonitorServer.Get().Report(pc.GetPlayerId(), fps);	// listen host: we ARE the server.
		else
			pc.DCO_SendFpsReport(fps);
	}

	void SetActive(bool on)
	{
		if (m_bGlobal == on)
			return;
		m_bGlobal = on;
		SendGlobal(on);
		SyncKeepalive();
	}

	void SetWatch(int playerId, bool on)
	{
		if (on == m_Watched.Contains(playerId))
			return;
		if (on)
			m_Watched.Set(playerId, true);
		else
			m_Watched.Remove(playerId);
		SendWatch(playerId, on);
		SyncKeepalive();
		Print(string.Format("[DCO-GM] FPS watch for player %1: %2", playerId, on), LogLevel.NORMAL);
	}

	// Double-click on a PLAYERS-list row: flip the watch.
	bool ToggleWatch(int playerId)
	{
		bool on = !m_Watched.Contains(playerId);
		SetWatch(playerId, on);
		return on;
	}

	bool IsWatched(int playerId)
	{
		return m_Watched.Contains(playerId);
	}

	protected void SyncKeepalive()
	{
		bool want = m_bGlobal || !m_Watched.IsEmpty();
		if (want == m_bKeepaliveOn)
			return;
		m_bKeepaliveOn = want;
		GetGame().GetCallqueue().Remove(Keepalive);
		if (want)
		{
			GetGame().GetCallqueue().CallLater(Keepalive, KEEPALIVE_MS, true);
		}
		else
		{
			m_Fps.Clear();	// nothing subscribed anymore - drop the table so a re-enable starts clean.
			m_FpsTime.Clear();
		}
	}

	protected void Keepalive()
	{
		if (m_bGlobal)
			SendGlobal(true);
		for (int i = 0; i < m_Watched.Count(); i++)
			SendWatch(m_Watched.GetKey(i), true);
	}

	protected void SendGlobal(bool on)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return;
		if (Replication.IsServer())
			DCO_FpsMonitorServer.Get().SubscribeGlobal(pc.GetPlayerId(), on);
		else
			pc.DCO_SendFpsSubscribe(on);
	}

	protected void SendWatch(int playerId, bool on)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!pc)
			return;
		if (Replication.IsServer())
			DCO_FpsMonitorServer.Get().Watch(pc.GetPlayerId(), playerId, on);
		else
			pc.DCO_SendFpsWatch(playerId, on);
	}

	void StoreSample(int playerId, int fps)
	{
		if (!m_bGlobal && !m_Watched.Contains(playerId))
			return;	// raced a just-removed subscription - drop.
		m_Fps.Set(playerId, fps);
		BaseWorld w = GetGame().GetWorld();
		if (w)
			m_FpsTime.Set(playerId, w.GetWorldTime());
	}

	// PLAYERS-list read: the player's last FPS, or -1 when that player isn't covered / no fresh sample yet.
	int GetFps(int playerId)
	{
		if (!m_bGlobal && !m_Watched.Contains(playerId))
			return -1;
		int fps;
		if (!m_Fps.Find(playerId, fps))
			return -1;
		float t;
		BaseWorld w = GetGame().GetWorld();
		if (w && m_FpsTime.Find(playerId, t) && (w.GetWorldTime() - t) > STALE_MS)
			return -1;
		return fps;
	}

	bool IsActive()
	{
		return m_bGlobal;
	}
}

class DCO_FpsMonitorServer
{
	protected static ref DCO_FpsMonitorServer s_Inst;

	static const int   POLL_MS       = 8000;
	static const int   STAGGER_MS    = 250;
	static const float SUBSCRIBE_TTL = 35000;

	protected ref map<int, float> m_Expiry  = new map<int, float>();
	protected ref map<int, bool>  m_Global  = new map<int, bool>();
	protected ref map<int, ref map<int, bool>> m_Watches = new map<int, ref map<int, bool>>();	// subscriber -> watched ids.
	protected bool m_bPolling;

	static DCO_FpsMonitorServer Get()
	{
		if (!s_Inst)
			s_Inst = new DCO_FpsMonitorServer();
		return s_Inst;
	}

	void SubscribeGlobal(int subscriberId, bool on)
	{
		if (on)
		{
			m_Global.Set(subscriberId, true);
			Touch(subscriberId);
		}
		else
		{
			m_Global.Remove(subscriberId);
			DropIfEmpty(subscriberId);
		}
	}

	void Watch(int subscriberId, int targetId, bool on)
	{
		map<int, bool> watched;
		if (!m_Watches.Find(subscriberId, watched))
		{
			if (!on)
				return;
			watched = new map<int, bool>();
			m_Watches.Set(subscriberId, watched);
		}
		if (on)
		{
			watched.Set(targetId, true);
			Touch(subscriberId);
		}
		else
		{
			watched.Remove(targetId);
			if (watched.IsEmpty())
				m_Watches.Remove(subscriberId);
			DropIfEmpty(subscriberId);
		}
	}

	// Refresh a subscriber's expiry + make sure the poll loop is running.
	protected void Touch(int subscriberId)
	{
		BaseWorld w = GetGame().GetWorld();
		if (!w)
			return;
		m_Expiry.Set(subscriberId, w.GetWorldTime() + SUBSCRIBE_TTL);
		if (!m_bPolling)
		{
			m_bPolling = true;
			GetGame().GetCallqueue().CallLater(PollCycle, POLL_MS, true);
			PollCycle();	// first cycle immediately - the GM shouldn't wait 8 s for the first numbers.
		}
	}

	protected void DropIfEmpty(int subscriberId)
	{
		if (m_Global.Contains(subscriberId) || m_Watches.Contains(subscriberId))
			return;
		m_Expiry.Remove(subscriberId);
	}

	protected void PollCycle()
	{
		BaseWorld w = GetGame().GetWorld();
		if (!w)
			return;
		float now = w.GetWorldTime();
		for (int i = m_Expiry.Count() - 1; i >= 0; i--)
		{
			if (m_Expiry.GetElement(i) < now)
			{
				int deadId = m_Expiry.GetKey(i);
				m_Expiry.RemoveElement(i);
				m_Global.Remove(deadId);
				m_Watches.Remove(deadId);
			}
		}
		if (m_Expiry.IsEmpty())
		{
			GetGame().GetCallqueue().Remove(PollCycle);
			m_bPolling = false;
			return;
		}

		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		array<int> players = {};
		pm.GetPlayers(players);
		int slot = 0;
		foreach (int pid : players)
		{
			if (!IsCovered(pid))
				continue;	// nobody is looking at this player - zero traffic for them.
			GetGame().GetCallqueue().CallLater(PollOne, slot * STAGGER_MS, false, pid);
			slot++;
		}
	}

	// Whether any live subscriber's scope includes this player.
	protected bool IsCovered(int playerId)
	{
		if (!m_Global.IsEmpty())
			return true;
		for (int i = 0; i < m_Watches.Count(); i++)
		{
			if (m_Watches.GetElement(i).Contains(playerId))
				return true;
		}
		return false;
	}

	protected void PollOne(int playerId)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		SCR_PlayerController pc = SCR_PlayerController.Cast(pm.GetPlayerController(playerId));
		if (!pc)
			return;	// left between the cycle and this staggered send.
		if (pc == GetGame().GetPlayerController())
			DCO_FpsMonitorClient.Get().BeginMeasure();	// the host player's own client - no RPC needed.
		else
			pc.DCO_SendFpsPoll();
	}

	void Report(int playerId, int fps)
	{
		if (!IsCovered(playerId))
			return;
		fps = Math.ClampInt(fps, 0, 1000);
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		for (int i = 0; i < m_Expiry.Count(); i++)
		{
			int gmId = m_Expiry.GetKey(i);
			bool covers = m_Global.Contains(gmId);
			if (!covers)
			{
				map<int, bool> watched;
				covers = m_Watches.Find(gmId, watched) && watched.Contains(playerId);
			}
			if (!covers)
				continue;
			SCR_PlayerController gmPc = SCR_PlayerController.Cast(pm.GetPlayerController(gmId));
			if (!gmPc)
				continue;
			if (gmPc == GetGame().GetPlayerController())
				DCO_FpsMonitorClient.Get().StoreSample(playerId, fps);
			else
				gmPc.DCO_SendFpsSample(playerId, fps);
		}
	}
}
