class DCO_FxAudioComponentClass : ScriptComponentClass {}

class DCO_FxAudioComponent : ScriptComponent
{
	[Attribute("0"), RplProp()] protected int m_iPreset;
	[RplProp()] protected bool m_bPlaying;
	[Attribute("0.7"), RplProp()] protected float m_fVolume;
	[Attribute("100"), RplProp()] protected float m_fRadius;
	[Attribute("1"), RplProp()] protected float m_fFadeIn;
	[Attribute("1"), RplProp()] protected float m_fFadeOut;
	[Attribute("1"), RplProp()] protected bool m_bLoop;
	[Attribute("1"), RplProp()] protected bool m_bOcclusion;
	[Attribute("0.3"), RplProp()] protected float m_fReverb;
	[RplProp()] protected int m_iSequence;
	protected float m_fStartedAt;
	protected SoundComponent m_Sound;
	protected AudioHandle m_Handle = AudioHandle.Invalid;
	protected float m_fGain;
	protected int m_iLocalSequence = -1;
	protected int m_iLocalPreset = -1;
	protected float m_fNextAttempt;
	protected bool m_bLocalPlayed;
	protected float m_fLocalStartedAt;

	static string PresetName(int preset)
	{
		switch (preset)
		{
			case 0: return "Crowd — indoor gathering";
			case 1: return "Conversation — outside a bar";
			case 2: return "Dog — barking and growling";
			case 3: return "Voice — shouted battle cry";
			case 4: return "Battle — distant rifle fire";
			case 5: return "Rifle — single M16 shot";
		}
		return "";
	}
	static string EventName(int preset)
	{
		switch (preset)
		{
			case 0: return "BifrostCrowd";
			case 1: return "BifrostTalking";
			case 2: return "BifrostBarking";
			case 3: return "BifrostShouting";
			case 4: return "BifrostBattle";
			case 5: return "BifrostRifle";
		}
		return "";
	}
	protected float Duration()
	{
		switch (m_iPreset)
		{
			case 0: return 128.877;
			case 1: return 68.795;
			case 2: return 14.120;
			case 3: return 7.723;
			case 4: return 35.832;
			case 5: return 0.935;
		}
		return 1;
	}
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetGame().InPlayMode()) return;
		m_Sound = SoundComponent.Cast(owner.FindComponent(SoundComponent));
		GetGame().GetCallqueue().CallLater(UpdateAudio, 100, true);
	}
	override void OnDelete(IEntity owner)
	{
		if (GetGame()) GetGame().GetCallqueue().Remove(UpdateAudio);
		if (m_Handle != AudioHandle.Invalid) AudioSystem.TerminateSound(m_Handle);
		super.OnDelete(owner);
	}
	void StartPlayback()
	{
		if (!Replication.IsServer()) return;
		m_bPlaying = true;
		m_iSequence++;
		m_fStartedAt = GetGame().GetWorld().GetWorldTime();
		Replication.BumpMe();
	}
	float Setting(int setting)
	{
		switch (setting)
		{
			case 0: return m_iPreset;
			case 1: return m_bPlaying;
			case 2: return m_fVolume;
			case 3: return m_fRadius;
			case 4: return m_fFadeIn;
			case 5: return m_fFadeOut;
			case 6: return m_bLoop;
			case 7: return m_bOcclusion;
			case 8: return m_fReverb;
		}
		return 0;
	}
	void SetSetting(int setting, float value)
	{
		if (!Replication.IsServer() || !(value >= 0 && value <= 2000)) return;
		switch (setting)
		{
			case 0:
				m_iPreset = Math.ClampInt(Math.Round(value), 0, 5);
				if (m_bPlaying) StartPlayback();
				break;
			case 1:
				if (value == 1) StartPlayback();
				else m_bPlaying = false;
				break;
			case 2: m_fVolume = Math.Clamp(value, 0, 1); break;
			case 3: m_fRadius = Math.Clamp(value, 5, 2000); break;
			case 4: m_fFadeIn = Math.Clamp(value, 0, 30); break;
			case 5: m_fFadeOut = Math.Clamp(value, 0, 30); break;
			case 6: m_bLoop = value == 1; break;
			case 7: m_bOcclusion = value == 1; break;
			case 8: m_fReverb = Math.Clamp(value, 0, 1); break;
		}
		Replication.BumpMe();
	}
	protected void UpdateAudio()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (Replication.IsServer() && m_bPlaying && !m_bLoop && now - m_fStartedAt >= Duration() * 1000)
		{
			m_bPlaying = false;
			Replication.BumpMe();
		}
		if (System.IsConsoleApp() || !m_Sound) return;
		vector camera[4];
		GetGame().GetWorld().GetCurrentCamera(camera);
		vector position = GetOwner().GetOrigin();
		float distance = vector.Distance(position, camera[3]);
		bool audible = m_bPlaying && distance < m_fRadius;
		if (m_iLocalSequence != m_iSequence || m_iLocalPreset != m_iPreset)
		{
			if (m_Handle != AudioHandle.Invalid) AudioSystem.TerminateSoundFadeOut(m_Handle, true, m_fFadeOut);
			m_Handle = AudioHandle.Invalid;
			m_iLocalSequence = m_iSequence;
			m_iLocalPreset = m_iPreset;
			m_fGain = 0;
			m_fNextAttempt = 0;
			m_bLocalPlayed = false;
		}
		float desired;
		if (audible) desired = m_fVolume;
		float fade = m_fFadeOut;
		if (desired > m_fGain) fade = m_fFadeIn;
		float step = 1;
		if (fade > 0) step = Math.Max(m_fVolume, m_fGain) * 0.1 / fade;
		m_fGain += Math.Clamp(desired - m_fGain, -step, step);
		float attenuation = Math.Pow(Math.Clamp(1 - distance / m_fRadius, 0, 1), 2);
		float cutoff = 20000;
		if (m_bOcclusion && audible)
		{
			TraceParam trace = new TraceParam();
			trace.Start = position + "0 0.3 0";
			trace.End = camera[3];
			trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
			trace.TargetLayers = EPhysicsLayerDefs.FireGeometry;
			PlayerController listener = GetGame().GetPlayerController();
			if (listener) trace.Exclude = listener.GetControlledEntity();
			if (GetGame().GetWorld().TraceMove(trace, null) < 0.98)
			{
				cutoff = 900;
				attenuation *= 0.3;
			}
		}
		m_Sound.SetSignalValueStr("BifrostGain", m_fGain * attenuation);
		m_Sound.SetSignalValueStr("BifrostLowpass", cutoff);
		m_Sound.SetSignalValueStr("BifrostReverb", m_fReverb);
		if (m_Handle != AudioHandle.Invalid && now - m_fLocalStartedAt >= Duration() * 1000)
		{
			AudioSystem.TerminateSound(m_Handle);
			m_Handle = AudioHandle.Invalid;
		}
		if (m_Handle != AudioHandle.Invalid && !m_Sound.IsHandleValid(m_Handle)) m_Handle = AudioHandle.Invalid;
		if (m_fGain <= 0.001)
		{
			if (m_Handle != AudioHandle.Invalid) AudioSystem.TerminateSound(m_Handle);
			m_Handle = AudioHandle.Invalid;
			return;
		}
		if (m_Handle == AudioHandle.Invalid && audible && (m_bLoop || !m_bLocalPlayed) && now >= m_fNextAttempt)
		{
			m_Handle = m_Sound.SoundEvent(EventName(m_iPreset));
			if (m_Handle != AudioHandle.Invalid)
			{
				m_bLocalPlayed = true;
				m_fLocalStartedAt = now;
			}
			else m_fNextAttempt = now + 2000;
		}
	}
}

[BaseContainerProps()]
class DCO_AudioSettingEditorAttribute : SCR_BaseValueListEditorAttribute
{
	[Attribute("0")] protected int m_iSetting;
	protected DCO_FxAudioComponent Emitter(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner()) return null;
		return DCO_FxAudioComponent.Cast(editable.GetOwner().FindComponent(DCO_FxAudioComponent));
	}
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_FxAudioComponent emitter = Emitter(item);
		if (!emitter) return null;
		if (m_iSetting == 0 || m_iSetting == 1 || m_iSetting == 6 || m_iSetting == 7)
			return SCR_BaseEditorAttributeVar.CreateInt(Math.Round(emitter.Setting(m_iSetting)));
		return SCR_BaseEditorAttributeVar.CreateFloat(emitter.Setting(m_iSetting));
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var || !Replication.IsServer() || !DCO_GMRights.Allow(playerID, "Audio settings")) return;
		DCO_FxAudioComponent emitter = Emitter(item);
		if (!emitter) return;
		float value;
		if (m_iSetting == 0 || m_iSetting == 1 || m_iSetting == 6 || m_iSetting == 7) value = var.GetInt();
		else value = var.GetFloat();
		emitter.SetSetting(m_iSetting, value);
	}
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		if (m_iSetting == 0)
		{
			for (int preset = 0; preset < 6; preset++) outEntries.Insert(new SCR_BaseEditorAttributeEntryText(DCO_FxAudioComponent.PresetName(preset)));
			return outEntries.Count();
		}
		if (m_iSetting == 1 || m_iSetting == 6 || m_iSetting == 7)
		{
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText("Off"));
			outEntries.Insert(new SCR_BaseEditorAttributeEntryText("On"));
			return outEntries.Count();
		}
		return super.GetEntries(outEntries);
	}
}

[BaseContainerProps()]
class DCO_AudioPresetEditorAttribute : DCO_AudioSettingEditorAttribute {}

[BaseContainerProps()]
class DCO_AudioPlayingEditorAttribute : DCO_AudioSettingEditorAttribute {}

[BaseContainerProps()]
class DCO_AudioVolumeEditorAttribute : DCO_AudioSettingEditorAttribute {}

[BaseContainerProps()]
class DCO_AudioRadiusEditorAttribute : DCO_AudioSettingEditorAttribute {}

[BaseContainerProps()]
class DCO_AudioFadeInEditorAttribute : DCO_AudioSettingEditorAttribute {}

[BaseContainerProps()]
class DCO_AudioFadeOutEditorAttribute : DCO_AudioSettingEditorAttribute {}

[BaseContainerProps()]
class DCO_AudioLoopEditorAttribute : DCO_AudioSettingEditorAttribute {}

[BaseContainerProps()]
class DCO_AudioOcclusionEditorAttribute : DCO_AudioSettingEditorAttribute {}

[BaseContainerProps()]
class DCO_AudioReverbEditorAttribute : DCO_AudioSettingEditorAttribute {}
