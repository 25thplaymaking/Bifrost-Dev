class DCO_CharacterCommandFreeze : ScriptedCommand
{
	void DCO_CharacterCommandFreeze(BaseAnimPhysComponent pAnimPhysComponent)
	{
		CharacterAnimationComponent anim = CharacterAnimationComponent.Cast(pAnimPhysComponent);
		if (anim)
		{
			m_vMovementSpeed = anim.BindVariableFloat("MovementSpeed");
			m_vMovementDirection = anim.BindVariableFloat("MovementDirection");
		}
	}

	// Ask the freeze to end.
	void RequestFinish()
	{
		m_bFinish = true;
	}

	// Whether the frame-claim below should still apply.
	bool ShouldClaimFrame()
	{
		return !m_bFinish;
	}

	// Kept for the existing gizmo hook.
	void Reanchor()
	{
	}

	// Stop the locomotion graph too.
	override void PreAnimUpdate(float pDt)
	{
		if (m_vMovementSpeed != -1)
			PreAnim_SetFloat(m_vMovementSpeed, 0);
		if (m_vMovementDirection != -1)
			PreAnim_SetFloat(m_vMovementDirection, 0);
	}

	// Kill root motion at source: whatever the animation graph wants to translate this frame, it gets zero.
	override void PrePhysUpdate(float pDt)
	{
		PrePhys_SetTranslation(vector.Zero);
	}

	// Complete release from inside the command update.
	override bool PostPhysUpdate(float pDt)
	{
		if (m_bFinish)
		{
			SetFlagFinished(true);
			return false;
		}

		return true;
	}

	protected TAnimGraphVariable	m_vMovementSpeed = -1;
	protected TAnimGraphVariable	m_vMovementDirection = -1;
	protected bool			m_bFinish;
}

// Keeps the scripted freeze installed while it owns the current frame.
modded class SCR_CharacterCommandHandlerComponent
{
	override bool SubhandlerStatesBegin(CharacterInputContext pInputCtx, float pDt, int pCurrentCommandID)
	{
		if (pCurrentCommandID == ECommandIDs.SCRIPTED && m_CharacterAnimComp)
		{
			DCO_CharacterCommandFreeze frz = DCO_CharacterCommandFreeze.Cast(m_CharacterAnimComp.GetCommandScripted());
			if (frz && frz.ShouldClaimFrame())
				return true;	// Default command selection must not evict an active freeze.
		}
		return false;
	}
}
