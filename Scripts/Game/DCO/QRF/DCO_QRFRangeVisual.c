class DCO_QRFRangeVisual
{
	protected static const int		DCO_QRF_CIRCLE_COLOR	= 0xFF3FA9F5;	// ARGB - bright blue.
	protected static vector s_vOrigin;
	protected static float s_fRadius;
	protected static bool s_bVisible;

	static void Show(vector origin, float radius)
	{
		if (!DCO_GMRights.IsLocalGameMaster())
		{
			Hide();
			return;
		}

		if (radius < 1.0)
		{
			Hide();
			return;
		}

		s_vOrigin = origin;
		s_fRadius = radius;
		s_bVisible = true;
	}

	static void Hide()
	{
		s_bVisible = false;
	}

	static void Draw(DCO_GMRenderManager render)
	{
		if (!s_bVisible || !render || !DCO_GMRights.IsLocalGameMaster())
			return;
		render.DrawRing(s_vOrigin + Vector(0, 0.3, 0), Vector(1, 0, 0), Vector(0, 0, 1), s_fRadius, DCO_QRF_CIRCLE_COLOR);
	}
}
