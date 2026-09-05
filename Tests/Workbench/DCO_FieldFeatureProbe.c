class DCO_FieldFeatureProbe : DCO_GMHint
{
	int m_Passed;
	ref array<string> m_Failures = {};
	void Expect(bool condition, string label)
	{
		if (condition) m_Passed++;
		else m_Failures.Insert(label);
	}
	void Run()
	{
		if (!GetGame().InPlayMode() || !GetGame().GetWorld())
		{
			m_Failures.Insert("Run this probe in a local play world with a viewport.");
			return;
		}
		Show("GM message", "A field-test message that remains beside the action. " + "A field-test message that remains beside the action. " + "A field-test message that remains beside the action. ", 1);
		Expect(s_Card != null, "Hint layout created");
		if (s_Card)
		{
			s_Card.Update();
			float width, height;
			s_Card.GetScreenSize(width, height);
			Expect(width > 100 && height > 30, "Hint has visible dimensions");
			TextWidget message = TextWidget.Cast(s_Card.FindAnyWidget("Message"));
			Expect(message && message.GetText().Contains("field-test"), "Hint message populated");
		}
		Hide();
		Expect(s_Card == null, "Hint teardown");
		Resource prefab = Resource.Load("{3B5DEAE089B2E3A5}Prefabs/E_DCO_FxAudio.et");
		Expect(prefab && prefab.IsValid(), "Audio prefab resource");
		if (!prefab || !prefab.IsValid() || !GetGame().GetWorld()) return;
		IEntity entity = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
		Expect(entity != null, "Audio emitter created");
		if (entity)
		{
			SoundComponent sound = SoundComponent.Cast(entity.FindComponent(SoundComponent));
			Expect(sound != null, "Native sound component");
			if (sound)
			{
				vector listener[4];
				GetGame().GetWorld().GetCurrentCamera(listener);
				entity.SetOrigin(listener[3]);
				sound.SetTransformation(listener);
				sound.SetSignalValueStr("BifrostGain", 0.1);
				for (int preset = 0; preset < 6; preset++)
				{
					Expect(sound.GetEventIndex(DCO_FxAudioComponent.EventName(preset)) >= 0, "Audio event " + preset);
					AudioHandle handle = sound.SoundEvent(DCO_FxAudioComponent.EventName(preset));
					Expect(handle != AudioHandle.Invalid && sound.IsHandleValid(handle), "Audio playback handle " + preset);
					if (handle != AudioHandle.Invalid) AudioSystem.TerminateSound(handle);
				}
				array<string> signals = {};
				sound.GetSignalNames(signals);
				Expect(signals.Contains("BifrostGain"), "Volume signal");
				Expect(signals.Contains("BifrostLowpass"), "Wall filter signal");
				Expect(signals.Contains("BifrostReverb"), "Reverb signal");
			}
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
		}
		Resource pointPrefab = Resource.Load("{DCA6090560000000}Prefabs/E_DCO_Teleporter.et");
		IEntity pointEntity = GetGame().SpawnEntityPrefab(pointPrefab, GetGame().GetWorld());
		Expect(pointEntity != null, "Teleporter created");
		if (pointEntity)
		{
			DCO_GMMissionInteractionComponent point = DCO_GMMissionInteractionComponent.Cast(pointEntity.FindComponent(DCO_GMMissionInteractionComponent));
			Expect(point && point.m_bStandalone && point.Target() == pointEntity, "Independent teleporter target");
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(pointEntity.FindComponent(SCR_EditableEntityComponent));
			DCO_TeleporterActivationEditorAttribute attribute = new DCO_TeleporterActivationEditorAttribute();
			Expect(editable && attribute.ReadVariable(editable, null) != null, "Teleporter exposes editable attributes");
			SCR_EntityHelper.DeleteEntityAndChildren(pointEntity);
		}
	}
}
