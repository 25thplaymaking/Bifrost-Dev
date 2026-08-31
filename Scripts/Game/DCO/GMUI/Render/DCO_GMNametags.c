// F6 - PLAYER NAMETAGS.
class DCO_GMNametags
{
	protected Widget m_wRoot;
	protected ref array<Widget> m_Tags = {};	// the pooled nametag overlay roots.
	protected ref array<TextWidget> m_Texts = {};	// their name TextWidgets.
	protected bool m_bActive;

	protected const int   POOL = 12;
	protected const float HEAD_OFFSET = 2.0;
	protected const float TAG_WIDTH = 260;
	protected const float TAG_HEIGHT = 30;
	protected const float EDGE_GAP = 8;

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
				FrameSlot.SetSizeToContent(tag, false);
				FrameSlot.SetSize(tag, TAG_WIDTH, TAG_HEIGHT);
				FrameSlot.SetAlignment(tag, 0.5, 1.0);	// pivot bottom-centre => the tag sits above the projected head point.
				tag.SetVisible(false);
			}
		}
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
		float rootX, rootY, rootW, rootH;
		m_wRoot.GetScreenPos(rootX, rootY);
		m_wRoot.GetScreenSize(rootW, rootH);
		rootX = ws.DPIUnscale(rootX);
		rootY = ws.DPIUnscale(rootY);
		rootW = ws.DPIUnscale(rootW);
		rootH = ws.DPIUnscale(rootH);

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
			txt.SetText(BoundName(name));
			float tagX = rootX + rootW * 0.5;
			if (rootW > TAG_WIDTH + EDGE_GAP * 2)
				tagX = Math.Clamp(scr[0], rootX + EDGE_GAP + TAG_WIDTH * 0.5, rootX + rootW - EDGE_GAP - TAG_WIDTH * 0.5);
			float tagY = rootY + rootH * 0.5;
			if (rootH > TAG_HEIGHT + EDGE_GAP * 2)
				tagY = Math.Clamp(scr[1], rootY + EDGE_GAP + TAG_HEIGHT, rootY + rootH - EDGE_GAP);
			FrameSlot.SetPos(tag, tagX, tagY);
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

	protected string BoundName(string value)
	{
		if (value.Length() <= 46)
			return value;
		return value.Substring(0, 43) + "...";
	}

	void Shutdown()
	{
		Stop();
		m_Tags.Clear();
		m_Texts.Clear();
		m_wRoot = null;
	}
}
