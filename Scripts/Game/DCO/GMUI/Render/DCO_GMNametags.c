// F6 - PLAYER NAMETAGS.
class DCO_GMNametags
{
	protected Widget m_wRoot;
	protected ref array<Widget> m_Tags = {};	// the pooled nametag overlay roots.
	protected ref array<TextWidget> m_Texts = {};	// their name TextWidgets.
	protected bool m_bActive;

	protected const int   POOL = 12;
	protected const float HEAD_OFFSET = 2.0;

	void Init(Widget root)
	{
		if (!root)
			return;
		m_wRoot = root;
		Color nameCol = Color.FromInt(DCO_GMTheme.SEM_HOSTILE);
		for (int i = 0; i < POOL; i++)
		{
			Widget tag = root.FindAnyWidget("DCO_Nametag" + i.ToString());
			TextWidget txt = TextWidget.Cast(root.FindAnyWidget("DCO_NametagText" + i.ToString()));
			Widget bg = root.FindAnyWidget("DCO_NametagBg" + i.ToString());
			if (bg)
				bg.SetVisible(true);
			if (txt)
				txt.SetColor(nameCol);	// affiliation-coloured name, now legible against the chip.
			m_Tags.Insert(tag);
			m_Texts.Insert(txt);
			if (tag)
			{
				FrameSlot.SetAlignment(tag, 0.5, 1.0);	// pivot bottom-centre => the tag sits above the projected head point.
				tag.SetVisible(false);
			}
		}
		Print(string.Format("[DCO-GM] nametags bound (pool=%1)", m_Tags.Count()), LogLevel.NORMAL);
	}

	void Start()
	{
		if (m_bActive)
			return;
		m_bActive = true;
		GetGame().GetCallqueue().CallLater(Update, 33, true);	// ~30 Hz - smooth tracking of moving players/camera.
	}

	void Stop()
	{
		m_bActive = false;
		GetGame().GetCallqueue().Remove(Update);
		HideAll();
	}

	protected void HideAll()
	{
		foreach (Widget t : m_Tags)
		{
			if (t)
				t.SetVisible(false);
		}
	}

	protected void Update()
	{
		if (!m_bActive)
			return;
		WorkspaceWidget ws = GetGame().GetWorkspace();
		BaseWorld world = GetGame().GetWorld();
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!ws || !world || !pm)
		{
			HideAll();
			return;
		}

		array<int> ids = {};
		pm.GetPlayers(ids);

		int used = 0;
		foreach (int id : ids)
		{
			if (used >= POOL)
				break;
			IEntity ent = pm.GetPlayerControlledEntity(id);
			if (!ent)
				continue;
			string name = pm.GetPlayerName(id);
			if (name == string.Empty)
				continue;

			vector head = ent.GetOrigin() + Vector(0, HEAD_OFFSET, 0);
			vector scr = ws.ProjWorldToScreen(head, world);
			if (scr[2] < 0)	// behind the camera -> don't show.
				continue;

			Widget tag = m_Tags[used];
			TextWidget txt = m_Texts[used];
			if (!tag || !txt)
				continue;
			txt.SetText(name);
			FrameSlot.SetPos(tag, scr[0], scr[1]);
			tag.SetVisible(true);
			used++;
		}

		// Hide the unused remainder of the pool.
		for (int i = used; i < POOL; i++)
		{
			if (m_Tags[i])
				m_Tags[i].SetVisible(false);
		}
	}

	void Shutdown()
	{
		Stop();
		m_Tags.Clear();
		m_Texts.Clear();
		m_wRoot = null;
	}
}
