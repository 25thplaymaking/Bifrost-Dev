//! SETTINGS tab for the preferences used by the Bifrost Arsenal V2 interface.
class GRSA_SettingsScreen : SCR_SubMenuBase
{
	protected SCR_SpinBoxComponent m_ApplyOnCloseSpin;
	protected SCR_SpinBoxComponent m_OrbitSensitivitySpin;
	protected SCR_SpinBoxComponent m_StudioLightingSpin;
	protected SCR_SpinBoxComponent m_StudioBrightnessSpin;
	protected SCR_SpinBoxComponent m_StudioRearFillSpin;
	protected bool m_bSyncing;

	//------------------------------------------------------------------------------------------------
	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		Widget applyRoot = m_wRoot.FindAnyWidget("SettingApplyOnClose");
		if (applyRoot)
			m_ApplyOnCloseSpin = SCR_SpinBoxComponent.Cast(applyRoot.FindHandler(SCR_SpinBoxComponent));
		if (m_ApplyOnCloseSpin)
		{
			m_ApplyOnCloseSpin.SetLabel("APPLY CHANGES ON CLOSE");
			m_ApplyOnCloseSpin.ClearAll();
			m_ApplyOnCloseSpin.AddItem("Off");
			m_ApplyOnCloseSpin.AddItem("On");
			m_ApplyOnCloseSpin.m_OnChanged.Insert(OnApplyOnCloseChanged);
		}

		Widget sensitivityRoot = m_wRoot.FindAnyWidget("SettingOrbitSensitivity");
		if (sensitivityRoot)
			m_OrbitSensitivitySpin = SCR_SpinBoxComponent.Cast(sensitivityRoot.FindHandler(SCR_SpinBoxComponent));
		if (m_OrbitSensitivitySpin)
		{
			m_OrbitSensitivitySpin.SetLabel("ROTATION SENSITIVITY");
			m_OrbitSensitivitySpin.ClearAll();
			for (int value = GRSA_ClientPrefs.SENSITIVITY_MIN; value <= GRSA_ClientPrefs.SENSITIVITY_MAX; value += 25)
				m_OrbitSensitivitySpin.AddItem(value.ToString() + "%");
			m_OrbitSensitivitySpin.m_OnChanged.Insert(OnOrbitSensitivityChanged);
		}

		Widget lightingRoot = m_wRoot.FindAnyWidget("SettingStudioLighting");
		if (lightingRoot)
			m_StudioLightingSpin = SCR_SpinBoxComponent.Cast(lightingRoot.FindHandler(SCR_SpinBoxComponent));
		if (m_StudioLightingSpin)
		{
			m_StudioLightingSpin.SetLabel("STUDIO LIGHTING");
			m_StudioLightingSpin.ClearAll();
			m_StudioLightingSpin.AddItem("Off");
			m_StudioLightingSpin.AddItem("On");
			m_StudioLightingSpin.m_OnChanged.Insert(OnStudioLightingChanged);
		}

		Widget brightnessRoot = m_wRoot.FindAnyWidget("SettingStudioBrightness");
		if (brightnessRoot)
			m_StudioBrightnessSpin = SCR_SpinBoxComponent.Cast(brightnessRoot.FindHandler(SCR_SpinBoxComponent));
		if (m_StudioBrightnessSpin)
		{
			m_StudioBrightnessSpin.SetLabel("STUDIO BRIGHTNESS");
			m_StudioBrightnessSpin.ClearAll();
			for (int brightness = GRSA_ClientPrefs.LIGHT_MIN; brightness <= GRSA_ClientPrefs.LIGHT_MAX; brightness += GRSA_ClientPrefs.LIGHT_STEP)
				m_StudioBrightnessSpin.AddItem(brightness.ToString() + "%");
			m_StudioBrightnessSpin.m_OnChanged.Insert(OnStudioBrightnessChanged);
		}

		Widget fillRoot = m_wRoot.FindAnyWidget("SettingStudioRearFill");
		if (fillRoot)
			m_StudioRearFillSpin = SCR_SpinBoxComponent.Cast(fillRoot.FindHandler(SCR_SpinBoxComponent));
		if (m_StudioRearFillSpin)
		{
			m_StudioRearFillSpin.SetLabel("REAR FILL LIGHT");
			m_StudioRearFillSpin.ClearAll();
			for (int fill = GRSA_ClientPrefs.FILL_MIN; fill <= GRSA_ClientPrefs.FILL_MAX; fill += GRSA_ClientPrefs.FILL_STEP)
				m_StudioRearFillSpin.AddItem(fill.ToString() + "%");
			m_StudioRearFillSpin.m_OnChanged.Insert(OnStudioRearFillChanged);
		}

		SyncFromPrefs();
	}

	//------------------------------------------------------------------------------------------------
	override void OnTabShow()
	{
		super.OnTabShow();
		SyncFromPrefs();
		GRSA_Theme.Apply(m_wRoot);
		SeedFocus();
	}

	//------------------------------------------------------------------------------------------------
	protected void SeedFocus()
	{
		if (m_ApplyOnCloseSpin && m_ApplyOnCloseSpin.GetRootWidget())
			GetGame().GetWorkspace().SetFocusedWidget(m_ApplyOnCloseSpin.GetRootWidget());
	}

	//------------------------------------------------------------------------------------------------
	//! Control setters fire m_OnChanged, the sync guard keeps them from writing back into prefs.
	protected void SyncFromPrefs()
	{
		m_bSyncing = true;

		GRSA_ClientPrefs prefs = GRSA_ClientPrefs.Get();
		if (m_ApplyOnCloseSpin)
		{
			int applyIndex;
			if (prefs.m_bApplyOnClose)
				applyIndex = 1;
			m_ApplyOnCloseSpin.SetCurrentItem(applyIndex);
		}
		if (m_OrbitSensitivitySpin)
			m_OrbitSensitivitySpin.SetCurrentItem((prefs.m_iOrbitSensitivity - GRSA_ClientPrefs.SENSITIVITY_MIN) / 25);
		if (m_StudioLightingSpin)
			m_StudioLightingSpin.SetCurrentItem(prefs.m_bStudioLighting);
		if (m_StudioBrightnessSpin)
			m_StudioBrightnessSpin.SetCurrentItem((prefs.m_iStudioBrightness - GRSA_ClientPrefs.LIGHT_MIN) / GRSA_ClientPrefs.LIGHT_STEP);
		if (m_StudioRearFillSpin)
			m_StudioRearFillSpin.SetCurrentItem((prefs.m_iStudioRearFill - GRSA_ClientPrefs.FILL_MIN) / GRSA_ClientPrefs.FILL_STEP);

		m_bSyncing = false;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnApplyOnCloseChanged(SCR_SpinBoxComponent spin, int index)
	{
		if (m_bSyncing)
			return;

		GRSA_ClientPrefs.Get().m_bApplyOnClose = index > 0;
		GRSA_ClientPrefs.Save();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnOrbitSensitivityChanged(SCR_SpinBoxComponent spin, int index)
	{
		if (m_bSyncing)
			return;

		GRSA_ClientPrefs.Get().m_iOrbitSensitivity = GRSA_ClientPrefs.SENSITIVITY_MIN + index * 25;
		GRSA_ClientPrefs.Save();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnStudioLightingChanged(SCR_SpinBoxComponent spin, int index)
	{
		if (m_bSyncing)
			return;

		GRSA_ClientPrefs.Get().m_bStudioLighting = index > 0;
		GRSA_ClientPrefs.Save();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnStudioBrightnessChanged(SCR_SpinBoxComponent spin, int index)
	{
		if (m_bSyncing)
			return;

		GRSA_ClientPrefs.Get().m_iStudioBrightness = GRSA_ClientPrefs.LIGHT_MIN + index * GRSA_ClientPrefs.LIGHT_STEP;
		GRSA_ClientPrefs.Save();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnStudioRearFillChanged(SCR_SpinBoxComponent spin, int index)
	{
		if (m_bSyncing)
			return;

		GRSA_ClientPrefs.Get().m_iStudioRearFill = GRSA_ClientPrefs.FILL_MIN + index * GRSA_ClientPrefs.FILL_STEP;
		GRSA_ClientPrefs.Save();
	}
}
