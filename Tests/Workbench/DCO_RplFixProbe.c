modded class SCR_CharacterControllerComponent
{
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		super.OnPrepareControls(owner, am, dt, player);
		if (DCO_RplFixProbe.s_Character == owner)
			GetInputContext().SetMovement(DCO_RplFixProbe.s_Input, "0 0 1");
	}
}
modded class DCO_FxAudioComponent
{
	int DCO_TestStarts;
	AudioHandle DCO_TestHandle() { return m_Handle; }
	override protected void UpdateAudio()
	{
		AudioHandle previous = m_Handle;
		super.UpdateAudio();
		if (m_Handle != AudioHandle.Invalid && m_Handle != previous) DCO_TestStarts++;
	}
}
class DCO_RplFixProbe
{
	static ref DCO_RplFixProbe s_Probe;
	static ChimeraCharacter s_Character;
	static float s_Input;
	int passed;
	ref array<string> failures = {};
	ref array<string> measurements = {};
	bool done;
	protected SCR_PlayerController m_Player;
	protected IEntity m_Previous;
	protected IEntity m_AudioEntity;
	protected DCO_FxAudioComponent m_Audio;
	protected SCR_EditableEntityComponent m_Editable;
	protected vector m_Start;
	protected vector m_Initial[4];
	protected int m_Stage;
	protected float m_Time;
	protected float m_NormalDistance;
	void Expect(bool value, string label) { if (value) passed++; else failures.Insert(label); }
	void Start()
	{
		if (!GetGame().InPlayMode() || !Replication.IsServer()) { failures.Insert("Local server play world required"); done = true; return; }
		m_Player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!m_Player) { failures.Insert("Local controller required"); done = true; return; }
		vector camera[4];
		GetGame().GetWorld().GetCurrentCamera(camera);
		vector position = "925 40 2817";
		position[1] = SCR_TerrainHelper.GetTerrainY(position) + 0.2;
		EntitySpawnParams spawn = new EntitySpawnParams();
		spawn.TransformMode = ETransformMode.WORLD;
		spawn.Transform[3] = position;
		s_Character = ChimeraCharacter.Cast(GetGame().SpawnEntityPrefab(Resource.Load("Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Unarmed.et"), GetGame().GetWorld(), spawn));
		Expect(s_Character != null, "Character spawn");
		if (!s_Character) { done = true; return; }
		m_Previous = m_Player.GetControlledEntity();
		Expect(m_Player.SetControlledEntity(s_Character), "Test possession");
		m_Editable = SCR_EditableEntityComponent.Cast(s_Character.FindComponent(SCR_EditableEntityComponent));
		Expect(m_Editable != null, "Character editable");
		if (!m_Editable) { Finish(); return; }
		m_Editable.DCO_SetMissionScale(1);
		m_Editable.DCO_SetMovementFactor(1);
		m_AudioEntity = GetGame().SpawnEntityPrefab(Resource.Load("{3B5DEAE089B2E3A5}Prefabs/E_DCO_FxAudio.et"), GetGame().GetWorld(), spawn);
		m_AudioEntity.SetOrigin(camera[3]);
		m_Audio = DCO_FxAudioComponent.Cast(m_AudioEntity.FindComponent(DCO_FxAudioComponent));
		m_Audio.SetSetting(0, 5);
		m_Audio.SetSetting(2, 0.05);
		m_Audio.SetSetting(4, 0);
		m_Audio.SetSetting(5, 5);
		m_Audio.SetSetting(6, 0);
		m_Audio.StartPlayback();
		m_Time = GetGame().GetWorld().GetWorldTime();
		GetGame().GetCallqueue().CallLater(Tick, 100, true);
	}
	void Tick()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (!s_Character || !m_AudioEntity) { failures.Insert("Test entities lost"); Finish(); return; }
		if (m_Stage == 0 && now - m_Time >= 2500)
		{
			Expect(m_Audio.DCO_TestStarts == 1, "One-shot starts exactly once");
			Expect(m_Audio.DCO_TestHandle() == AudioHandle.Invalid, "One-shot handle finishes during long fade");
			measurements.Insert("Sound played: " + AudioSystem.IsSoundPlayed(m_Audio.DCO_TestHandle()));
			s_Character.GetWorldTransform(m_Initial);
			m_Start = s_Character.GetOrigin();
			s_Input = 2;
			m_Stage = 1; m_Time = now;
		}
		else if (m_Stage == 1 && now - m_Time >= 4000)
		{
			m_NormalDistance = vector.DistanceXZ(m_Start, s_Character.GetOrigin());
			measurements.Insert("Normal distance: " + m_NormalDistance);
			SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(s_Character.GetCharacterController());
			measurements.Insert("Input/velocity: " + controller.GetMovementInput() + " / " + controller.GetMovementVelocity());
			Expect(m_NormalDistance > 2, "Native movement responds to input");
			s_Character.Teleport(m_Initial);
			m_Start = s_Character.GetOrigin();
			m_Editable.DCO_SetMovementFactor(2);
			m_Stage = 2; m_Time = now;
		}
		else if (m_Stage == 2 && now - m_Time >= 4000)
		{
			float distance = vector.DistanceXZ(m_Start, s_Character.GetOrigin());
			measurements.Insert("Assisted distance: " + distance);
			Expect(distance > m_NormalDistance * 1.4, "Speed multiplier increases measured travel");
			m_Editable.DCO_SetMovementFactor(1);
			s_Input = 0;
			vector target = m_Initial[3] + "4 0 0";
			target[1] = SCR_TerrainHelper.GetTerrainY(target) + 0.1;
			Expect(m_Player.DCO_TeleportMissionCharacter(s_Character, target), "Player-controller teleport succeeds");
			m_Start = target;
			string result;
			Expect(!DCO_GMMissionServer.Teleport(null, m_AudioEntity, result), "Null controller rejected");
			m_Audio.SetSetting(6, 1);
			m_Audio.StartPlayback();
			m_Stage = 3; m_Time = now;
		}
		else if (m_Stage == 3 && now - m_Time >= 2500)
		{
			Expect(vector.DistanceXZ(s_Character.GetOrigin(), m_Start) < 0.2, "Teleport applies target position on simulation tick");
			Expect(m_Audio.DCO_TestStarts >= 3, "Loop restarts finite recording");
			m_Audio.SetSetting(1, 0);
			m_Stage = 4; m_Time = now;
			measurements.Insert("Starts at stop: " + m_Audio.DCO_TestStarts);
		}
		else if (m_Stage == 4 && now - m_Time >= 1500)
		{
			Expect(m_Audio.DCO_TestHandle() == AudioHandle.Invalid, "Stopping does not replay during fade");
			Finish();
		}
	}
	void Finish()
	{
		GetGame().GetCallqueue().Remove(Tick);
		s_Input = 0;
		if (m_Player) m_Player.SetControlledEntity(m_Previous);
		if (s_Character) SCR_EntityHelper.DeleteEntityAndChildren(s_Character);
		s_Character = null;
		if (m_AudioEntity) SCR_EntityHelper.DeleteEntityAndChildren(m_AudioEntity);
		done = true;
	}
}
