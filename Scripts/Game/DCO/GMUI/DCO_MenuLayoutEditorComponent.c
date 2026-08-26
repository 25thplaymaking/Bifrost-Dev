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
		{
			Print("[DCO-GM] editor mode activated; existing shell preserved", LogLevel.NORMAL);
			return;
		}
		if (m_DCOPanelLayout.IsEmpty())
		{
			Print("[DCO-GM] mount SKIP: m_DCOPanelLayout is empty", LogLevel.WARNING);
			return;
		}

		m_UI = new DCO_GMUIController(m_DCOPanelLayout);
		m_UI.Activate();	// builds the shell now if mouse+keyboard; waits otherwise.

		Print("[DCO-GM] DCO_GMUIController activated (EOnEditorPostActivate)", LogLevel.NORMAL);
	}

	override void EOnEditorPostDeactivate()
	{
		// Preserve the shell during mode changes; cleanup occurs when the editor closes.
		if (m_UI)
			Print("[DCO-GM] editor mode deactivated; shell preserved until editor close", LogLevel.NORMAL);
	}

	override void EOnEditorClose()
	{
		TeardownShell("editor closed");
	}

	override void EOnEditorDelete()
	{
		TeardownShell("editor component deleted");
	}

	protected void TeardownShell(string reason)
	{
		if (m_UI)
		{
			m_UI.Deactivate();	// restores any hidden engine UI + tears down the shell.
			m_UI = null;
			Print("[DCO-GM] shell lifecycle cleanup: " + reason, LogLevel.NORMAL);
		}
	}
}
