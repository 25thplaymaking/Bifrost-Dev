# Achilles SQF function inventory

Date: 2026-09-04
Source: `ArmaAchilles/Achilles` at `f123656459cab7766aa40c32d5ee12d29ebadaae`

This is the complete file-level inventory of all 294 `fn_*.sqf` files in the reviewed tree. A duplicate filename in different folders is retained because it is a distinct implementation file. Numbered Ares custom-module files are expressed as the complete inclusive range `0-49`.

## `functions_f_achilles` — 105

Common/change/select/feature/replacement/init implementations (100): `ACS_toggleGrouping`, `addBreachDoorAction`, `advancedBlackfishCAS`, `advancedHeliCAS`, `advancedPlaneCAS`, `ambientAnim`, `ambientAnimGetParams`, `animation`, `arrayStdDev`, `breachStun`, `changeAbility`, `changeAccessoires`, `changeNVGBrightness`, `changePylonAmmo`, `changeSide_local`, `changeSideAttribute`, `changeSkills`, `chatter`, `checkLineOfFire2D`, `chute`, `ClassNamesWhichInheritsFromCfgClass`, `CopyObjectsToClipboard`, `createDummyLogic`, `createIED`, `createSuicideBomber`, `curatorObjectEdited`, `curatorObjectPlaced`, `damageBuildings`, `damageComponents`, `deadlyExplosion`, `dikToLetter`, `disablingExplosion`, `drawArrow3D`, `drawRectangle3D`, `effectFire`, `eject_passengers`, `fakeExplosion`, `findInDict`, `forceWeaponFire`, `garage`, `garageZeus`, `getAceMedicalFunction`, `getAllTurretConfig`, `getAllTurrets`, `getCuratorSelected`, `getDirPitchBank`, `getEntityAttributes`, `getLogics`, `getUnitAmmoDef`, `getVehicleAmmo`, `getVirtualArsenal`, `getWeaponsMuzzlesMagazines` (two files), `groupObjects`, `HigherConfigHierarchyLevel`, `IED_DamageHandler`, `InstantBuildingGarrison`, `interpolation_cubicBezier1D`, `interpolation_cubicBezier1D_slope`, `isACELoaded`, `LaunchCM`, `log`, `logicSelector`, `onCuratorStart`, `PasteObjectsFromClipboard`, `PreplaceMode`, `pushBack`, `returnChildren`, `setACEInjury`, `setCuratorVisionModes`, `setDict`, `setDictKeyword`, `setLRFrequencies`, `setMagazineAmmo`, `setSensors`, `setSRFrequencies`, `setTurretAmmo`, `setUnitAmmoDef`, `setVanillaInjury`, `setVehicleAmmo`, `ShowChooseDialog`, `showCuratorAttributes`, `showZeusErrorMessage`, `spawn`, `spawn_remote`, `sum`, `SuppressiveFire`, `surrenderUnit`, `switchUnit_exit`, `switchUnit_start`, `SwitchZeusSide`, `TextToVariableName`, `toggleCuratorVisionMode`, `transferOwnership`, `ungroupObjects`, `updateStandardInventory`, `updateVirtualArsenal`, `vectAngleXY`, `vectDirUpFromDirPitchBank`, `weaponsAllTurrets`.

Scripted waypoint implementations (5): `wpFastrope`, `wpLand`, `wpParadrop`, `wpRepair`, `wpSearchBuilding`.

## `functions_f_ares` — 22

Common (15): `CreateLogic`, `ExecuteCustomModuleCode`, `GetArrayDataFromUser`, `GetFarthest`, `GetGroupUnderCursor`, `GetNearest`, `GetPhoneticName`, `GetSafePos`, `GetUnitUnderCursor`, `IsZeus`, `LogMessage`, `RegisterCustomModule`, `ShowZeusMessage`, `StringContains`, `WaitForZeus`.

Features (7): `addIntel`, `AddUnitsToCurator`, `GenerateArsenalBlacklist`, `GenerateArsenalDataList`, `SearchBuilding`, `TeleportPlayers`, `ZenOccupyHouse`.

## `modules_f_achilles` — 61

- ACE (3): `ModuleACEHeal`, `ModuleACEImmersiveHeal`, `ModuleACEInjury`.
- Arsenal (5): `ArsenalAddFull`, `ArsenalCopyToClipboard`, `ArsenalCreateCustom`, `ArsenalPaste`, `ArsenalRemove`.
- Behaviours (8): `BehaviourAltitude`, `BehaviourAnimation`, `BehaviourChangeAbility`, `BehaviourChatter`, `BehaviourPatrol`, `BehaviourSitOnChair`, `BehaviourSuicideBomber`, `BehaviourSurrenderUnits`.
- Buildings (3): `BuildingsDestroy`, `LockDoors`, `ToggleLamps`.
- DevTools (5): `DevTools_manageAdvancedCompositions`, `DevToolsBindVariable`, `DevToolsFunctionViewer`, `DevToolsShowInAnimViewer`, `DevToolsShowInConfig`.
- Effects (5): `lightSourceAttributes`, `moduleEffectsFire`, `moduleLightSource`, `modulePersistentSmokePillar`, `spawnSmoke`.
- Environment (3): `EnvironmentEarthquake`, `EnvironmentSetDate`, `EnvironmentSetWeatherModule`.
- Equipment (1): `attachDetachEffect`.
- FireSupport (4): `ModuleFireSupportCAS`, `ModuleFireSupportCreateUniversalTarget`, `ModuleFireSupportNuke`, `ModuleFireSupportSuppressiveFire`.
- MissionFlow (2): `changeSideRelations`, `SpawnCreateEditIntel`.
- Objects (9): `ModuleObjectsMakeInvincible`, `ObjectsAddECM`, `ObjectsAttachTo`, `ObjectsHide`, `ObjectsIED`, `ObjectsRotation`, `ObjectsSetHeight`, `ObjectsToggleSimulation`, `ObjectsTransferOwnership`.
- Player (1): `PlayerSetFrequencies`.
- Reinforcements (1): `ReinforcementsSupplyDrop`.
- Replacement (3): `moduleCAS_server`, `moduleMine`, `moduleRemoteControl`.
- Spawn (6): `SpawnAdvancedCompositions`, `SpawnCarrier`, `SpawnDestroyer`, `SpawnEffect`, `SpawnEmptyObject`, `SpawnExplosives`.
- Zeus (2): `ZeusAssignZeus`, `ZeusSwitchUnit`.

## `modules_f_ares` — 72

- Behaviours (4): `BehaviourSearchNearbyAndGarrison`, `BehaviourSearchNearbyBuilding`, `GarrisonNearest`, `UnGarrison`.
- Custom (50): every `UserDefinedModule0` through `UserDefinedModule49`, inclusive.
- DevTools (2): `CreateMissionSQF`, `ExecuteCode`.
- Equipment (3): `EquipmentFlashlightIRLaserOnOff`, `EquipmentNVDRailAttachment`, `EquipmentTurretOptics`.
- FireSupport (1): `FireSupportArtilleryFireMission`.
- Player (3): `PlayerChangeSide`, `PlayerCreateTeleporter`, `PlayerTeleport`.
- Reinforcements (3): `ReinforcementsCreateLz`, `ReinforcementsCreateRp`, `ReinforcementsCreateUnits`.
- Spawn (2): `SpawnSubmarine`, `SpawnTrawler`.
- Zeus (4): `ZeusAddRemoveEditableObjects`, `ZeusHint`, `ZeusSwitchSideChannel`, `ZeusVisibility`.

## `ui_f` — 33

`AppendToModuleTree`, `HandleCuratorGroupPlaced`, `HandleCuratorKeyPressed`, `HandleCuratorObjectDeleted`, `HandleCuratorObjectEdited`, `HandleCuratorObjectPlaced`, `HandleCuratorWpPlaced`, `HandleMouseDoubleClicked`, `HandleRemoteKeyPressed`, `initCuratorAttribute`, `onDisplayCuratorLoad`, `onDisplayCuratorUnload`, `onGameStarted`, `onModuleTreeLoad`, `RscDisplayAttributes_BuildingsDestroy`, `RscDisplayAttributes_Chatter`, `RscDisplayAttributes_createAdvancedComposition`, `RscDisplayAttributes_CreateReinforcement`, `RscDisplayAttributes_editableObjects`, `RscDisplayAttributes_editAdvancedComposition`, `RscDisplayAttributes_editLigthSource`, `RscDisplayAttributes_LockDoors`, `RscDisplayAttributes_manageAdvancedComposition`, `RscDisplayAttributes_selectAIUnits`, `RscDisplayAttributes_selectPlayers`, `RscDisplayAttributes_spawnAdvancedComposition`, `RscDisplayAttributes_SpawnEmptyObject`, `RscDisplayAttributes_SpawnExplosives`, `RscDisplayAttributes_SupplyDrop`, `RscDisplayAtttributes_SpawnEffect`, `SelectUnits`, `ShowChooseDialog`, `sideTab`.

## `settings_f` — 1

`onSettingsChanged`.

## Count reconciliation

The verified total is `105 + 22 + 61 + 72 + 33 + 1 = 294`.
