// DCO additive GM panel mounter.

[ComponentEditorProps(category: "GameScripted/Editor", description: "DCO additive GM panel mounter")]
class DCO_MenuLayoutEditorComponentClass : SCR_BaseEditorComponentClass
{
}

class DCO_MenuLayoutEditorComponent : SCR_BaseEditorComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, "DCO GM panel layout to mount at the workspace root.", "layout")]
	protected ResourceName m_DCOPanelLayout;

	protected ref DCO_GMUIController m_UI;

	override void EOnEditorPostActivate()
	{
		if (m_UI)
			return;
		if (m_DCOPanelLayout.IsEmpty())
		{
			Print("[DCO-GM] mount SKIP: m_DCOPanelLayout is empty", LogLevel.WARNING);
			return;
		}

		m_UI = new DCO_GMUIController(m_DCOPanelLayout);
		m_UI.Activate();	// builds the shell now if mouse+keyboard; waits otherwise.
	}

	override void EOnEditorClose()
	{
		TeardownShell();
	}

	override void EOnEditorDelete()
	{
		TeardownShell();
	}

	protected void TeardownShell()
	{
		if (m_UI)
		{
			m_UI.Deactivate();	// restores any hidden engine UI + tears down the shell.
			m_UI = null;
		}
	}
}
