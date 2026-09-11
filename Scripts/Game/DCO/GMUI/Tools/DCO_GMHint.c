class DCO_GMHint
{
	protected static Widget s_Card;
	static void Show(string title, string body, float duration)
	{
		Hide();
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace) return;
		s_Card = workspace.CreateWidgets("{DCA6090570000000}UI/layouts/DCO_GMHint.layout");
		if (!s_Card) return;
		TextWidget heading = TextWidget.Cast(s_Card.FindAnyWidget("Title"));
		TextWidget message = TextWidget.Cast(s_Card.FindAnyWidget("Message"));
		if (heading) heading.SetText(title);
		if (message) message.SetText(body);
		float screenWidth, screenHeight;
		workspace.GetScreenSize(screenWidth, screenHeight);
		float width = Math.Min(520, workspace.DPIUnscale(screenWidth) - 40);
		FrameSlot.SetAnchor(s_Card, 1, 0.15);
		FrameSlot.SetAlignment(s_Card, 1, 0);
		FrameSlot.SetPos(s_Card, -28, 0);
		FrameSlot.SetSize(s_Card, width, 280);
		s_Card.Update();
		Widget content = s_Card.FindAnyWidget("Content");
		float contentWidth, contentHeight;
		if (content)
		{
			content.GetScreenSize(contentWidth, contentHeight);
			FrameSlot.SetSize(s_Card, width, workspace.DPIUnscale(contentHeight) + 30);
		}
		s_Card.SetEnabled(false);
		GetGame().GetCallqueue().CallLater(Hide, Math.Round(Math.Clamp(duration, 1, 300) * 1000), false);
	}
	static void Hide()
	{
		GetGame().GetCallqueue().Remove(Hide);
		if (s_Card) s_Card.RemoveFromHierarchy();
		s_Card = null;
	}
}

modded class ArmaReforgerScripted
{
	override void OnGameEnd()
	{
		DCO_GMHint.Hide();
		super.OnGameEnd();
	}
}
