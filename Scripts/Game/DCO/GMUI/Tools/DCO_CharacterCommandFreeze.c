class DCO_CharacterCommandFreeze : ScriptedCommand
{
	void DCO_CharacterCommandFreeze(BaseAnimPhysComponent pAnimPhysComponent, IEntity owner)
	{
		m_Owner = owner;
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

	// Complete release from inside the command update; no transform is corrected after physics.
	override bool PostPhysUpdate(float pDt)
	{
		if (!m_bLoggedTick)
		{
			m_bLoggedTick = true;
			Print("[DCO-GM] freeze command TICKING (root movement held)", LogLevel.NORMAL);
		}

		if (m_bFinish)
		{
			SetFlagFinished(true);
			return false;
		}

		return true;
	}

	protected IEntity		m_Owner;
	protected TAnimGraphVariable	m_vMovementSpeed = -1;
	protected TAnimGraphVariable	m_vMovementDirection = -1;
	protected bool			m_bFinish;
	protected bool		m_bLoggedTick;
}

// Keeping the freeze INSTALLED — the half that SetCurrentCommand alone does NOT buy you.
modded class SCR_CharacterCommandHandlerComponent
{
	override bool SubhandlerStatesBegin(CharacterInputContext pInputCtx, float pDt, int pCurrentCommandID)
	{
		if (pCurrentCommandID == ECommandIDs.SCRIPTED && m_CharacterAnimComp)
		{
			DCO_CharacterCommandFreeze frz = DCO_CharacterCommandFreeze.Cast(m_CharacterAnimComp.GetCommandScripted());
			if (frz && frz.ShouldClaimFrame())
				return true;	// the freeze owns this frame — do not let default selection evict it.
		}
		// FALLTHROUGH IS `false`, NEVER `super` — see the class header.
		return false;
	}
}
