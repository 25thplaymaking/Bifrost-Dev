class DCO_ContentBrowserGate
{
	protected static const float NATIVE_ACTION_WINDOW_MS = 500.0;
	protected static float s_fAllowedAtMs;
	protected static bool s_bAllowed;

	static void AllowNativeActionBrowser()
	{
		BaseWorld world = GetGame().GetWorld();
		if (world)
		{
			s_fAllowedAtMs = world.GetWorldTime();
			s_bAllowed = true;
		}
	}

	static bool ConsumeNativeActionBrowserAllowance()
	{
		if (!s_bAllowed)
			return false;
		s_bAllowed = false;
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;
		float elapsedMs = world.GetWorldTime() - s_fAllowedAtMs;
		return elapsedMs >= 0 && elapsedMs <= NATIVE_ACTION_WINDOW_MS;
	}

	static void ClearNativeActionBrowserAllowance()
	{
		s_bAllowed = false;
	}
}

modded class SCR_ContentBrowserEditorComponent
{
	override bool OpenBrowserMenu()
	{
		if (DCO_GMUIController.IsActive() && !DCO_ContentBrowserGate.ConsumeNativeActionBrowserAllowance())
			return false;	// The CREATE panel owns direct browser requests while Bifrost is active.
		return super.OpenBrowserMenu();
	}
}
