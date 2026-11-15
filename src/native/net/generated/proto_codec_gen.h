#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "net/proto_wire.h"
#include "net/generated/proto_gen.h"

/*
 * 由 scripts/gen_proto_codec.py 生成, 请勿手改.
 * 每个 message 一对 Serialize / Deserialize, 只处理已知字段, 未知字段跳过.
 */
namespace immirror::net::gen {

void Serialize(const PB_HeartBeatCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_HeartBeatCH* value);

void Serialize(const PB_SyncChunkDataCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SyncChunkDataCH* value);

void Serialize(const PB_RoleEnterWorldCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoleEnterWorldCH* value);

void Serialize(const PB_RoleLeaveWorldCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoleLeaveWorldCH* value);

void Serialize(const PB_RoleMoveCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoleMoveCH* value);

void Serialize(const PB_TrainMoveCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TrainMoveCH* value);

void Serialize(const PB_ActorTeleportCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorTeleportCH* value);

void Serialize(const PB_GunInfoCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GunInfoCH* value);

void Serialize(const PB_SetInfoCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetInfoCH* value);

void Serialize(const PB_ItemGridUserData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemGridUserData* value);

void Serialize(const PB_BlockInteractCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BlockInteractCH* value);

void Serialize(const PB_BlockPunchCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BlockPunchCH* value);

void Serialize(const PB_ItemUseCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemUseCH* value);

void Serialize(const PB_SpecialSkillCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SpecialSkillCH* value);

void Serialize(const PB_SetHookCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetHookCH* value);

void Serialize(const PB_ItemSkillUseCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemSkillUseCH* value);

void Serialize(const PB_ActorInteractCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorInteractCH* value);

void Serialize(const PB_ActorAnimCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorAnimCH* value);

void Serialize(const PB_BackPackGridSwapCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackGridSwapCH* value);

void Serialize(const PB_BackPackMoveItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackMoveItemCH* value);

void Serialize(const PB_BackPackGridDiscardCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackGridDiscardCH* value);

void Serialize(const PB_BackPackEquipWeaponCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackEquipWeaponCH* value);

void Serialize(const PB_NeedContainerPasswordCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_NeedContainerPasswordCH* value);

void Serialize(const PB_CloseContainerCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloseContainerCH* value);

void Serialize(const PB_SetContainerTextCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetContainerTextCH* value);

void Serialize(const PB_BackPackStoreCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackStoreCH* value);

void Serialize(const PB_BackPackLootCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackLootCH* value);

void Serialize(const PB_BackPackSortCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackSortCH* value);

void Serialize(const PB_BackPackSetItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackSetItemCH* value);

void Serialize(const PB_BackPackSetItemWithoutLimitCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackSetItemWithoutLimitCH* value);

void Serialize(const PB_StorageBoxSortCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_StorageBoxSortCH* value);

void Serialize(const PB_CraftItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CraftItemCH* value);

void Serialize(const PB_EnchantItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EnchantItemCH* value);

void Serialize(const PB_EnchantItemRandomCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EnchantItemRandomCH* value);

void Serialize(const PB_RepairItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RepairItemCH* value);

void Serialize(const PB_GunDoReloadCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GunDoReloadCH* value);

void Serialize(const PB_AccountHorseCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_AccountHorseCH* value);

void Serialize(const PB_ActorReviveCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorReviveCH* value);

void Serialize(const PB_JruisdicTionCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_JruisdicTionCH* value);

void Serialize(const PB_ChatCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ChatCH* value);

void Serialize(const PB_PlayerMountActorCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerMountActorCH* value);

void Serialize(const PB_PlayerMoveInputCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerMoveInputCH* value);

void Serialize(const PB_PlayerSleepCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerSleepCH* value);

void Serialize(const PB_NpcTradeCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_NpcTradeCH* value);

void Serialize(const PB_YMVoiceCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_YMVoiceCH* value);

void Serialize(const PB_GVChangeRoleCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GVChangeRoleCH* value);

void Serialize(const PB_YMChangeRoleCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_YMChangeRoleCH* value);

void Serialize(const PB_GetAccountItemsCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GetAccountItemsCH* value);

void Serialize(const PB_SpecialItemUseCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SpecialItemUseCH* value);

void Serialize(const PB_SetSpectatorModeCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetSpectatorModeCH* value);

void Serialize(const PB_SetSpectatorTypeCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetSpectatorTypeCH* value);

void Serialize(const PB_SetSpectatorPlayerCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetSpectatorPlayerCH* value);

void Serialize(const PB_SetPlayerModelAniCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetPlayerModelAniCH* value);

void Serialize(const PB_SendMyViewmodeToSpectatorCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SendMyViewmodeToSpectatorCH* value);

void Serialize(const PB_SetBobbingToSpectatorCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetBobbingToSpectatorCH* value);

void Serialize(const PB_BallOperateCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BallOperateCH* value);

void Serialize(const PB_RocketTeleportCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RocketTeleportCH* value);

void Serialize(const PB_CloseDialogueCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloseDialogueCH* value);

void Serialize(const PB_AnswerTaskCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_AnswerTaskCH* value);

void Serialize(const PB_CompleteTaskCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CompleteTaskCH* value);

void Serialize(const PB_PlayActCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayActCH* value);

void Serialize(const PB_BluePrintPreBlockCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BluePrintPreBlockCH* value);

void Serialize(const PB_GravityOperateCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GravityOperateCH* value);

void Serialize(const PB_MakeCustomModelCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MakeCustomModelCH* value);

void Serialize(const PB_SelectMobSpawnerCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SelectMobSpawnerCH* value);

void Serialize(const PB_GetNpcShopInfoCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GetNpcShopInfoCH* value);

void Serialize(const PB_BuyNpcShopItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BuyNpcShopItemCH* value);

void Serialize(const PB_CloseEditActorModelCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloseEditActorModelCH* value);

void Serialize(const PB_PackGiftNotifyItemChgCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PackGiftNotifyItemChgCH* value);

void Serialize(const PB_VehiclePreBlockCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehiclePreBlockCH* value);

void Serialize(const PB_VehicleItemUseCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleItemUseCH* value);

void Serialize(const PB_VehicleStartBlockCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleStartBlockCH* value);

void Serialize(const PB_VehicleAttribChangeCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAttribChangeCH* value);

void Serialize(const PB_WorkshopItemInfoCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_WorkshopItemInfoCH* value);

void Serialize(const PB_PlayerVehicleMoveInputCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerVehicleMoveInputCH* value);

void Serialize(const PB_PlayerResetVehicleCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerResetVehicleCH* value);

void Serialize(const PB_PlayerMotionStateChangeCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerMotionStateChangeCH* value);

void Serialize(const PB_PlayerClickCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerClickCH* value);

void Serialize(const PB_PlayerSelectShortcutCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerSelectShortcutCH* value);

void Serialize(const PB_TRIGGERPLAYERATTRICH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TRIGGERPLAYERATTRICH* value);

void Serialize(const PB_ReqDownLoadResUrlCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ReqDownLoadResUrlCH* value);

void Serialize(const PB_CloseFullyCustomModelUICH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloseFullyCustomModelUICH* value);

void Serialize(const PB_PlayerNavFinishedCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerNavFinishedCH* value);

void Serialize(const PB_VehicleAssembleLineCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineCH* value);

void Serialize(const PB_VehicleAssembleLineOperateCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineOperateCH* value);

void Serialize(const PB_UpdateActionerDataCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_UpdateActionerDataCH* value);

void Serialize(const PB_CSPlayerPermitCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CSPlayerPermitCH* value);

void Serialize(const PB_VehicleWorkshopLineCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleWorkshopLineCH* value);

void Serialize(const PB_CSChangePlayerTeamCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CSChangePlayerTeamCH* value);

void Serialize(const PB_CSRentRoomAutoMuteCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CSRentRoomAutoMuteCH* value);

void Serialize(const PB_VehicleAssembleLineUpdateCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineUpdateCH* value);

void Serialize(const PB_MapEditHandleCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MapEditHandleCH* value);

void Serialize(const PB_MapEditRevokeCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MapEditRevokeCH* value);

void Serialize(const PB_CloudRoomOwnerStartGameCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloudRoomOwnerStartGameCH* value);

void Serialize(const PB_CSKickOffDataCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CSKickOffDataCH* value);

void Serialize(const PB_UsePackingFCMItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_UsePackingFCMItemCH* value);

void Serialize(const PB_CreatePackingCMCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CreatePackingCMCH* value);

void Serialize(const PB_InputContentCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_InputContentCH* value);

void Serialize(const PB_InputKeyCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_InputKeyCH* value);

void Serialize(const PB_SensorContainerDataCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SensorContainerDataCH* value);

void Serialize(const PB_PlayerCarryActorCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerCarryActorCH* value);

void Serialize(const PB_VillagerModifyName& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VillagerModifyName* value);

void Serialize(const PB_PlayerGotoPosCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerGotoPosCH* value);

void Serialize(const PB_CustomModelPrepareCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CustomModelPrepareCH* value);

void Serialize(const PB_MoveMobBackpackItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MoveMobBackpackItemCH* value);

void Serialize(const PB_InteractMobBackpackItemCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_InteractMobBackpackItemCH* value);

void Serialize(const PB_AltarLuckyDrawCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_AltarLuckyDrawCH* value);

void Serialize(const PB_PlayerRestoreTransformSkinCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerRestoreTransformSkinCH* value);

void Serialize(const PB_PlayerDeformationSkinCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerDeformationSkinCH* value);

void Serialize(const PB_PlayerResetDeformationCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerResetDeformationCH* value);

void Serialize(const PB_PlayerBaseAttrCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerBaseAttrCH* value);

void Serialize(const PB_PlayerArchEntityCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerArchEntityCH* value);

void Serialize(const PB_PrayTreeTimeCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PrayTreeTimeCH* value);

void Serialize(const PB_SUMMONPETCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SUMMONPETCH* value);

void Serialize(const PB_HomeLandRanchUpdateAnimalStateCH& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_HomeLandRanchUpdateAnimalStateCH* value);

void Serialize(const PB_RequestModelName& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RequestModelName* value);

void Serialize(const PB_Vector3& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Vector3* value);

void Serialize(const PB_Quaternion& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Quaternion* value);

void Serialize(const PB_Pos& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Pos* value);

void Serialize(const PB_BodyDir& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BodyDir* value);

void Serialize(const PB_Item& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Item* value);

void Serialize(const PB_ArmFumo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ArmFumo* value);

void Serialize(const PB_Arm& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Arm* value);

void Serialize(const PB_ItemSpecial& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemSpecial* value);

void Serialize(const PB_ItemGridData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemGridData* value);

void Serialize(const PB_ItemGrid& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemGrid* value);

void Serialize(const PB_RoleBackpack& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoleBackpack* value);

void Serialize(const PB_PlayerVipInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerVipInfo* value);

void Serialize(const PB_ChunkBlob& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ChunkBlob* value);

void Serialize(const PB_ChunkSaveDB& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ChunkSaveDB* value);

void Serialize(const PB_GridPak& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GridPak* value);

void Serialize(const PB_ShortcutPak& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ShortcutPak* value);

void Serialize(const PB_ArmPak& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ArmPak* value);

void Serialize(const PB_ActorBuff& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorBuff* value);

void Serialize(const PB_BuffAttr& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BuffAttr* value);

void Serialize(const PB_RolePackage& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RolePackage* value);

void Serialize(const PB_ActorBuffList& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorBuffList* value);

void Serialize(const PB_RoleData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoleData* value);

void Serialize(const PB_BodyEffectBrief& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BodyEffectBrief* value);

void Serialize(const PB_PlayerInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerInfo* value);

void Serialize(const PB_RedStoneIdx& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RedStoneIdx* value);

void Serialize(const PB_RedStoneData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RedStoneData* value);

void Serialize(const PB_GlobalBin& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GlobalBin* value);

void Serialize(const PB_OWGlobalMisc& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OWGlobalMisc* value);

void Serialize(const PB_OWGlobal& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OWGlobal* value);

void Serialize(const PB_WorldCreateData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_WorldCreateData* value);

void Serialize(const PB_WorldDesc& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_WorldDesc* value);

void Serialize(const PB_SkillCDData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SkillCDData* value);

void Serialize(const PB_RoleInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoleInfo* value);

void Serialize(const PB_ActorRoleInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorRoleInfo* value);

void Serialize(const PB_ActorGeneral& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorGeneral* value);

void Serialize(const PB_ActorInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorInfo* value);

void Serialize(const PB_MoveMotion& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MoveMotion* value);

void Serialize(const PB_ItemData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemData* value);

void Serialize(const PB_PlayerBriefInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerBriefInfo* value);

void Serialize(const PB_EffectParticle& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectParticle* value);

void Serialize(const PB_EffectPickItem& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectPickItem* value);

void Serialize(const PB_EffectSound& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectSound* value);

void Serialize(const PB_EffectTriggerSound& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectTriggerSound* value);

void Serialize(const PB_EffectActorBody& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectActorBody* value);

void Serialize(const PB_EffectStringActorBody& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectStringActorBody* value);

void Serialize(const PB_EffectDestroyBlock& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectDestroyBlock* value);

void Serialize(const PB_EffectCrackBlock& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectCrackBlock* value);

void Serialize(const PB_EffectVehicle& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectVehicle* value);

void Serialize(const PB_EffectPlayMusicGrid& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectPlayMusicGrid* value);

void Serialize(const PB_EffectStopMusicGrid& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectStopMusicGrid* value);

void Serialize(const PB_IntertactData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_IntertactData* value);

void Serialize(const PB_TaskContentData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TaskContentData* value);

void Serialize(const PB_TaskInfoData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TaskInfoData* value);

void Serialize(const PB_PreBlockData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PreBlockData* value);

void Serialize(const PB_CustomModelClassData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CustomModelClassData* value);

void Serialize(const PB_MobDisplayData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MobDisplayData* value);

void Serialize(const PB_NpcShopItemData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_NpcShopItemData* value);

void Serialize(const PB_NpcShopData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_NpcShopData* value);

void Serialize(const PB_ActorOneAvatarModelData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorOneAvatarModelData* value);

void Serialize(const PB_ActorOneBoneModelData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorOneBoneModelData* value);

void Serialize(const PB_BasketBallOperate& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BasketBallOperate* value);

void Serialize(const PB_CSAuthorityData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CSAuthorityData* value);

void Serialize(const PB_CSPermitData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CSPermitData* value);

void Serialize(const PB_Edu_RoleInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Edu_RoleInfo* value);

void Serialize(const PB_ImportModelData& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ImportModelData* value);

void Serialize(const PB_GraphicsAttr& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GraphicsAttr* value);

void Serialize(const PB_MsgErrorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MsgErrorHC* value);

void Serialize(const PB_HeartBeatHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_HeartBeatHC* value);

void Serialize(const PB_SyncChunkDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SyncChunkDataHC* value);

void Serialize(const PB_BlockUpdateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BlockUpdateHC* value);

void Serialize(const PB_RoleEnterWorldHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoleEnterWorldHC* value);

void Serialize(const PB_RoleLeaveWorldHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoleLeaveWorldHC* value);

void Serialize(const PB_ActorEnterAOIHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorEnterAOIHC* value);

void Serialize(const AssembleBlockInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, AssembleBlockInfo* value);

void Serialize(const PB_VehicleAssembleBlockUpdateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAssembleBlockUpdateHC* value);

void Serialize(const PB_VehicleAssembleBlockAllHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAssembleBlockAllHC* value);

void Serialize(const PB_ActorLeaveAOIHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorLeaveAOIHC* value);

void Serialize(const PB_ActorMoveHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorMoveHC* value);

void Serialize(const PB_TrainMoveHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TrainMoveHC* value);

void Serialize(const PB_ActorTeleportHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorTeleportHC* value);

void Serialize(const PB_ActorMotionHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorMotionHC* value);

void Serialize(const PB_MechaMotionHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MechaMotionHC* value);

void Serialize(const PB_SyncTriggerBlock& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SyncTriggerBlock* value);

void Serialize(const PB_BlockInteractHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BlockInteractHC* value);

void Serialize(const PB_BlockPunchHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BlockPunchHC* value);

void Serialize(const PB_ItemUseHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemUseHC* value);

void Serialize(const PB_SetHookHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetHookHC* value);

void Serialize(const PB_ItemSkillUseHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ItemSkillUseHC* value);

void Serialize(const PB_ActorInteractHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorInteractHC* value);

void Serialize(const PB_ActorAnimHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorAnimHC* value);

void Serialize(const PB_BackPackGridUpdateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackGridUpdateHC* value);

void Serialize(const PB_BackPackEquipWeaponHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BackPackEquipWeaponHC* value);

void Serialize(const PB_CloseContainerHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloseContainerHC* value);

void Serialize(const PB_OpenContainerHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OpenContainerHC* value);

void Serialize(const PB_NeedContainerPasswordHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_NeedContainerPasswordHC* value);

void Serialize(const PB_UpdateContainerHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_UpdateContainerHC* value);

void Serialize(const PB_ActorEquipItemHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorEquipItemHC* value);

void Serialize(const PB_EnchantItemSuccessHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EnchantItemSuccessHC* value);

void Serialize(const PB_RepairItemSuccessHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RepairItemSuccessHC* value);

void Serialize(const PB_GunDoReloadHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GunDoReloadHC* value);

void Serialize(const PB_AccountHorseHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_AccountHorseHC* value);

void Serialize(const PB_ActorAttrChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorAttrChangeHC* value);

void Serialize(const PB_ActorBuffChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorBuffChangeHC* value);

void Serialize(const PB_ActorReviveHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorReviveHC* value);

void Serialize(const PB_PlayerAttrChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerAttrChangeHC* value);

void Serialize(const PB_MobBodyChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MobBodyChangeHC* value);

void Serialize(const PB_JruisdicTionHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_JruisdicTionHC* value);

void Serialize(const PB_ChatHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ChatHC* value);

void Serialize(const PB_WGlobalUpdateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_WGlobalUpdateHC* value);

void Serialize(const PB_PlayersUpdateInfoHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayersUpdateInfoHC* value);

void Serialize(const PB_GameTipsHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GameTipsHC* value);

void Serialize(const PB_PlayEffectHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayEffectHC* value);

void Serialize(const PB_EffectScaleHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_EffectScaleHC* value);

void Serialize(const PB_PlayerMountActorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerMountActorHC* value);

void Serialize(const PB_PlayerSleepHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerSleepHC* value);

void Serialize(const PB_OpenWindowHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OpenWindowHC* value);

void Serialize(const PB_LastPingHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_LastPingHC* value);

void Serialize(const PB_CGameStageHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CGameStageHC* value);

void Serialize(const PB_PlayerPermitHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerPermitHC* value);

void Serialize(const PB_YMVoiceHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_YMVoiceHC* value);

void Serialize(const PB_SkillCDHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SkillCDHC* value);

void Serialize(const PB_Horse_SkillCDHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Horse_SkillCDHC* value);

void Serialize(const PB_ActorMountActorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorMountActorHC* value);

void Serialize(const PB_ActorReverseHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorReverseHC* value);

void Serialize(const PB_ActorBindHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorBindHC* value);

void Serialize(const PB_PlayWeaponEffectHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayWeaponEffectHC* value);

void Serialize(const PB_ScriptVarHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ScriptVarHC* value);

void Serialize(const PB_GVChangeRoleHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GVChangeRoleHC* value);

void Serialize(const PB_YMChangeRoleHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_YMChangeRoleHC* value);

void Serialize(const PB_SpecialItemUseHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SpecialItemUseHC* value);

void Serialize(const PB_LeaveRoomInfoHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_LeaveRoomInfoHC* value);

void Serialize(const PB_InviteJoinRoomHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_InviteJoinRoomHC* value);

void Serialize(const PB_SetSpectatorModeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetSpectatorModeHC* value);

void Serialize(const PB_SetSpectatorTypeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetSpectatorTypeHC* value);

void Serialize(const PB_SetSpectatorPlayerHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetSpectatorPlayerHC* value);

void Serialize(const PB_SetPlayerModelAniHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetPlayerModelAniHC* value);

void Serialize(const PB_SendMyViewmodeToSpectatorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SendMyViewmodeToSpectatorHC* value);

void Serialize(const PB_SetBobbingToSpectatorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SetBobbingToSpectatorHC* value);

void Serialize(const PB_BallOperateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BallOperateHC* value);

void Serialize(const PB_BasketBallOperateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BasketBallOperateHC* value);

void Serialize(const PB_ResetRoundHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ResetRoundHC* value);

void Serialize(const PB_RocketAttribChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RocketAttribChangeHC* value);

void Serialize(const PB_ActorBodyTextureHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorBodyTextureHC* value);

void Serialize(const PB_AttractAttribChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_AttractAttribChangeHC* value);

void Serialize(const PB_WorldTimesHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_WorldTimesHC* value);

void Serialize(const PB_StatisticHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_StatisticHC* value);

void Serialize(const PB_TotemPointHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TotemPointHC* value);

void Serialize(const PB_HorseFlyStateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_HorseFlyStateHC* value);

void Serialize(const PB_OpenDialogueHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OpenDialogueHC* value);

void Serialize(const PB_CloseDialogueHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloseDialogueHC* value);

void Serialize(const PB_UpdateTaskHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_UpdateTaskHC* value);

void Serialize(const PB_SyncTaskEnterWorldHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SyncTaskEnterWorldHC* value);

void Serialize(const PB_CompleteTaskHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CompleteTaskHC* value);

void Serialize(const PB_PlayerAddAvartarHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerAddAvartarHC* value);

void Serialize(const PB_PlayerChangeModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerChangeModelHC* value);

void Serialize(const PB_PlayerAvartarColorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerAvartarColorHC* value);

void Serialize(const PB_PlayActHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayActHC* value);

void Serialize(const PB_CreateBlueprintHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CreateBlueprintHC* value);

void Serialize(const PB_MeasureDistanceHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_MeasureDistanceHC* value);

void Serialize(const PB_BluePrintPreBlockHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_BluePrintPreBlockHC* value);

void Serialize(const PB_GravityOperateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GravityOperateHC* value);

void Serialize(const PB_PlayerBodyColorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerBodyColorHC* value);

void Serialize(const PB_CustomModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CustomModelHC* value);

void Serialize(const PB_CustomModelPrepareHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CustomModelPrepareHC* value);

void Serialize(const PB_CustomItemIDsHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CustomItemIDsHC* value);

void Serialize(const PB_PlayerSpawnPointHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerSpawnPointHC* value);

void Serialize(const PB_CustomModelClassHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CustomModelClassHC* value);

void Serialize(const PB_TransferOneRecordHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TransferOneRecordHC* value);

void Serialize(const PB_TransferRecordHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TransferRecordHC* value);

void Serialize(const PB_TransferNameTipHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TransferNameTipHC* value);

void Serialize(const PB_TransferAddDelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TransferAddDelHC* value);

void Serialize(const PB_TransferDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TransferDataHC* value);

void Serialize(const PB_TransferTargetHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TransferTargetHC* value);

void Serialize(const PB_OpenUIHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OpenUIHC* value);

void Serialize(const PB_RespNpcShopInfoHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RespNpcShopInfoHC* value);

void Serialize(const PB_NotifyNpcShopBuySkuHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_NotifyNpcShopBuySkuHC* value);

void Serialize(const PB_VehiclePosDesc& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehiclePosDesc* value);

void Serialize(const PB_VehicleSTrustersPowerLevel& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleSTrustersPowerLevel* value);

void Serialize(const PB_VehicleMoveHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleMoveHC* value);

void Serialize(const PB_OpenEditActorModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OpenEditActorModelHC* value);

void Serialize(const PB_CloseEditActorModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloseEditActorModelHC* value);

void Serialize(const PB_OneCustomActorModelDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OneCustomActorModelDataHC* value);

void Serialize(const PB_CustomActorModelDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CustomActorModelDataHC* value);

void Serialize(const PB_VehiclePreBlockHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehiclePreBlockHC* value);

void Serialize(const PB_VehicleItemIdHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleItemIdHC* value);

void Serialize(const PB_VehicleAttribChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAttribChangeHC* value);

void Serialize(const PB_WorkshopItemInfoHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_WorkshopItemInfoHC* value);

void Serialize(const PB_PlayerCameraRotateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerCameraRotateHC* value);

void Serialize(const PB_PlayerChangeViewModeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerChangeViewModeHC* value);

void Serialize(const PB_PlayerCanMoveHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerCanMoveHC* value);

void Serialize(const PB_PlayerCanControlHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerCanControlHC* value);

void Serialize(const PB_PlayerSetAttrHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerSetAttrHC* value);

void Serialize(const PB_TriggerTimerDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TriggerTimerDataHC* value);

void Serialize(const PB_PlayerFreezingHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerFreezingHC* value);

void Serialize(const PB_WorkshopBuildHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_WorkshopBuildHC* value);

void Serialize(const PB_GameRuleHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GameRuleHC* value);

void Serialize(const PB_PlayerScaleHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerScaleHC* value);

void Serialize(const PB_PlayerNavigateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerNavigateHC* value);

void Serialize(const PB_PlayerCommonSetHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerCommonSetHC* value);

void Serialize(const PB_OpenEditFullyCustomModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OpenEditFullyCustomModelHC* value);

void Serialize(const PB_CloseFullyCustomModelUIHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloseFullyCustomModelUIHC* value);

void Serialize(const PB_RespDownLoadResUrlHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RespDownLoadResUrlHC* value);

void Serialize(const PB_PreOpenEditFCMUIHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PreOpenEditFCMUIHC* value);

void Serialize(const PB_VehicleAssembleLineHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineHC* value);

void Serialize(const PB_VehicleAssembleLineOperateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineOperateHC* value);

void Serialize(const PB_VehicleBindActorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VehicleBindActorHC* value);

void Serialize(const PB_TriggerMusicHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TriggerMusicHC* value);

void Serialize(const PB_CSPlayerPermitHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CSPlayerPermitHC* value);

void Serialize(const PB_CSAuthorityHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CSAuthorityHC* value);

void Serialize(const PB_SSTaskHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SSTaskHC* value);

void Serialize(const PB_CloudServerChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloudServerChangeHC* value);

void Serialize(const PB_TriggerOpenStoreHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_TriggerOpenStoreHC* value);

void Serialize(const PB_UsePackingFcmItemHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_UsePackingFcmItemHC* value);

void Serialize(const PB_CreatePackingCMHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CreatePackingCMHC* value);

void Serialize(const PB_OnePackingCMDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OnePackingCMDataHC* value);

void Serialize(const PB_OnePackingFCMDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OnePackingFCMDataHC* value);

void Serialize(const PB_PackingFCMDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PackingFCMDataHC* value);

void Serialize(const PB_CloudRoomStatusTimeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CloudRoomStatusTimeHC* value);

void Serialize(const PB_SensorContainerDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SensorContainerDataHC* value);

void Serialize(const PB_DoorDataHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_DoorDataHC* value);

void Serialize(const PB_PlayerCarryActorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerCarryActorHC* value);

void Serialize(const PB_VillagerBodyChangeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VillagerBodyChangeHC* value);

void Serialize(const PB_PlayerTameActorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerTameActorHC* value);

void Serialize(const PB_VillagerCloth& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VillagerCloth* value);

void Serialize(const PB_ActorHeadDisplayIconHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorHeadDisplayIconHC* value);

void Serialize(const PB_ActorPlayAnimByIdHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActorPlayAnimByIdHC* value);

void Serialize(const PB_VillageTotemTipHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VillageTotemTipHC* value);

void Serialize(const PB_VillageTotemActiveHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_VillageTotemActiveHC* value);

void Serialize(const PB_SaveTombStoneHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SaveTombStoneHC* value);

void Serialize(const PB_PlayerLevelModeHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerLevelModeHC* value);

void Serialize(const PB_ActionAttrStateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ActionAttrStateHC* value);

void Serialize(const PB_Edu_RolesInfoHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Edu_RolesInfoHC* value);

void Serialize(const PB_ImportModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ImportModelHC* value);

void Serialize(const PB_LightningHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_LightningHC* value);

void Serialize(const PB_InteractMobPackHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_InteractMobPackHC* value);

void Serialize(const PB_UpdateMobBackpackHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_UpdateMobBackpackHC* value);

void Serialize(const PB_PlayerTransformSkinHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerTransformSkinHC* value);

void Serialize(const PB_PlayerSaveArchHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayerSaveArchHC* value);

void Serialize(const PB_PrayTreeStageHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PrayTreeStageHC* value);

void Serialize(const PB_PrayTreeReqHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PrayTreeReqHC* value);

void Serialize(const PB_PrayTreeInfoHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PrayTreeInfoHC* value);

void Serialize(const PB_PrayTreeTimeUpdateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PrayTreeTimeUpdateHC* value);

void Serialize(const PB_HomeNpcOpenHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_HomeNpcOpenHC* value);

void Serialize(const PB_PrayErrorHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PrayErrorHC* value);

void Serialize(const PB_Open_HomeCloset_HC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_Open_HomeCloset_HC* value);

void Serialize(const PB_OpenDevGoodsBuyDialogHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OpenDevGoodsBuyDialogHC* value);

void Serialize(const PB_PlayGraphicsHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_PlayGraphicsHC* value);

void Serialize(const PB_GodTempleCreateHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GodTempleCreateHC* value);

void Serialize(const PB_SFActivity_HC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SFActivity_HC* value);

void Serialize(const PB_ShapeAdditionAnimHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ShapeAdditionAnimHC* value);

void Serialize(const PB_OneHomelandRanchAnimal& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_OneHomelandRanchAnimal* value);

void Serialize(const PB_HomelandRanchInfoHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_HomelandRanchInfoHC* value);

void Serialize(const PB_UseItemByHomelandHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_UseItemByHomelandHC* value);

void Serialize(const PB_CustomBaseModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_CustomBaseModelHC* value);

void Serialize(const PB_ChangeActorModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_ChangeActorModelHC* value);

void Serialize(const PB_NotifiyActorModelHC& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_NotifiyActorModelHC* value);

void Serialize(const PB_RoomInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_RoomInfo* value);

void Serialize(const PB_LoginRoomServerReq& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_LoginRoomServerReq* value);

void Serialize(const PB_SvrUrlInfo& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_SvrUrlInfo* value);

void Serialize(const PB_LoginRoomServerRes& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_LoginRoomServerRes* value);

void Serialize(const PB_GetRoomsInfoRes& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_GetRoomsInfoRes* value);

void Serialize(const PB_JoinRoomRes& value, ProtoWriter& writer);
bool Deserialize(ProtoReader& reader, PB_JoinRoomRes* value);

inline void Serialize(const PB_HeartBeatCH& value, ProtoWriter& writer) {
    if (value.has_BeatCode) {
        writer.WriteUInt64(1, value.BeatCode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_HeartBeatCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->BeatCode)) {
                    return false;
                }
                value->has_BeatCode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SyncChunkDataCH& value, ProtoWriter& writer) {
    if (value.has_SectionCoord) {
        ProtoWriter inner;
        Serialize(value.SectionCoord, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_trunkmd5) {
        writer.WriteString(2, value.trunkmd5);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SyncChunkDataCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->SectionCoord)) {
                    return false;
                }
                value->has_SectionCoord = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->trunkmd5)) {
                    return false;
                }
                value->has_trunkmd5 = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoleEnterWorldCH& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_GeniusLv) {
        writer.WriteInt32(2, value.GeniusLv);
    }
    if (value.has_UICtrlMode) {
        writer.WriteInt32(3, value.UICtrlMode);
    }
    if (value.has_RoleInfo) {
        ProtoWriter inner;
        Serialize(value.RoleInfo, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    if (value.has_VipInfo) {
        ProtoWriter inner;
        Serialize(value.VipInfo, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
    if (value.has_lang) {
        writer.WriteInt32(6, value.lang);
    }
    if (value.has_apiid) {
        writer.WriteInt32(7, value.apiid);
    }
    if (value.has_reserved) {
        writer.WriteInt32(8, value.reserved);
    }
    if (value.has_Auth) {
        writer.WriteString(9, value.Auth);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoleEnterWorldCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->GeniusLv)) {
                    return false;
                }
                value->has_GeniusLv = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->UICtrlMode)) {
                    return false;
                }
                value->has_UICtrlMode = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RoleInfo)) {
                    return false;
                }
                value->has_RoleInfo = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->VipInfo)) {
                    return false;
                }
                value->has_VipInfo = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->lang)) {
                    return false;
                }
                value->has_lang = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->apiid)) {
                    return false;
                }
                value->has_apiid = true;
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->reserved)) {
                    return false;
                }
                value->has_reserved = true;
                break;
            }
            case 9: {
                if (!reader.ReadString(&value->Auth)) {
                    return false;
                }
                value->has_Auth = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoleLeaveWorldCH& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoleLeaveWorldCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoleMoveCH& value, ProtoWriter& writer) {
    if (value.has_MoveMotion) {
        ProtoWriter inner;
        Serialize(value.MoveMotion, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_AddMotion) {
        ProtoWriter inner;
        Serialize(value.AddMotion, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_rentToken) {
        writer.WriteInt32(3, value.rentToken);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoleMoveCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->MoveMotion)) {
                    return false;
                }
                value->has_MoveMotion = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->AddMotion)) {
                    return false;
                }
                value->has_AddMotion = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->rentToken)) {
                    return false;
                }
                value->has_rentToken = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TrainMoveCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_MapID) {
        writer.WriteUInt32(2, value.MapID);
    }
    if (value.has_CarReverse) {
        writer.WriteInt32(3, value.CarReverse);
    }
    if (value.has_OutIndex) {
        writer.WriteInt32(4, value.OutIndex);
    }
    if (value.has_CurveT) {
        writer.WriteFloat(5, value.CurveT);
    }
    if (value.has_MotionX) {
        writer.WriteFloat(6, value.MotionX);
    }
    if (value.has_MotionY) {
        writer.WriteFloat(7, value.MotionY);
    }
    if (value.has_MotionZ) {
        writer.WriteFloat(8, value.MotionZ);
    }
    if (value.has_RailKnot) {
        ProtoWriter inner;
        Serialize(value.RailKnot, inner);
        writer.WriteMessage(9, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TrainMoveCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->CarReverse)) {
                    return false;
                }
                value->has_CarReverse = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->OutIndex)) {
                    return false;
                }
                value->has_OutIndex = true;
                break;
            }
            case 5: {
                if (!reader.ReadFloat(&value->CurveT)) {
                    return false;
                }
                value->has_CurveT = true;
                break;
            }
            case 6: {
                if (!reader.ReadFloat(&value->MotionX)) {
                    return false;
                }
                value->has_MotionX = true;
                break;
            }
            case 7: {
                if (!reader.ReadFloat(&value->MotionY)) {
                    return false;
                }
                value->has_MotionY = true;
                break;
            }
            case 8: {
                if (!reader.ReadFloat(&value->MotionZ)) {
                    return false;
                }
                value->has_MotionZ = true;
                break;
            }
            case 9: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RailKnot)) {
                    return false;
                }
                value->has_RailKnot = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorTeleportCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteInt64(1, value.ObjID);
    }
    if (value.has_TargetMap) {
        writer.WriteInt32(2, value.TargetMap);
    }
    if (value.has_TargetPos) {
        ProtoWriter inner;
        Serialize(value.TargetPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorTeleportCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->TargetMap)) {
                    return false;
                }
                value->has_TargetMap = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->TargetPos)) {
                    return false;
                }
                value->has_TargetPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GunInfoCH& value, ProtoWriter& writer) {
    if (value.has_CurSpread) {
        writer.WriteFloat(1, value.CurSpread);
    }
    if (value.has_CurJaw) {
        writer.WriteFloat(2, value.CurJaw);
    }
    if (value.has_CurPitch) {
        writer.WriteFloat(3, value.CurPitch);
    }
    if (value.has_CurPos) {
        ProtoWriter inner;
        Serialize(value.CurPos, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GunInfoCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->CurSpread)) {
                    return false;
                }
                value->has_CurSpread = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->CurJaw)) {
                    return false;
                }
                value->has_CurJaw = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->CurPitch)) {
                    return false;
                }
                value->has_CurPitch = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->CurPos)) {
                    return false;
                }
                value->has_CurPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetInfoCH& value, ProtoWriter& writer) {
    if (value.has_Color) {
        writer.WriteInt32(1, value.Color);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetInfoCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Color)) {
                    return false;
                }
                value->has_Color = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemGridUserData& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_GridIndex) {
        writer.WriteInt32(2, value.GridIndex);
    }
    if (value.has_UserDataStr) {
        writer.WriteString(3, value.UserDataStr);
    }
    if (value.has_Type) {
        writer.WriteInt32(4, value.Type);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemGridUserData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->GridIndex)) {
                    return false;
                }
                value->has_GridIndex = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->UserDataStr)) {
                    return false;
                }
                value->has_UserDataStr = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BlockInteractCH& value, ProtoWriter& writer) {
    if (value.has_face) {
        writer.WriteInt32(1, value.face);
    }
    if (value.has_colptx) {
        writer.WriteInt32(2, value.colptx);
    }
    if (value.has_colpty) {
        writer.WriteInt32(3, value.colpty);
    }
    if (value.has_colptz) {
        writer.WriteInt32(4, value.colptz);
    }
    if (value.has_blockpos) {
        ProtoWriter inner;
        Serialize(value.blockpos, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BlockInteractCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->face)) {
                    return false;
                }
                value->has_face = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->colptx)) {
                    return false;
                }
                value->has_colptx = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->colpty)) {
                    return false;
                }
                value->has_colpty = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->colptz)) {
                    return false;
                }
                value->has_colptz = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockpos)) {
                    return false;
                }
                value->has_blockpos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BlockPunchCH& value, ProtoWriter& writer) {
    if (value.has_status) {
        writer.WriteInt32(1, value.status);
    }
    if (value.has_face) {
        writer.WriteInt32(2, value.face);
    }
    if (value.has_digmethod) {
        writer.WriteInt32(3, value.digmethod);
    }
    if (value.has_blockpos) {
        ProtoWriter inner;
        Serialize(value.blockpos, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BlockPunchCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->status)) {
                    return false;
                }
                value->has_status = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->face)) {
                    return false;
                }
                value->has_face = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->digmethod)) {
                    return false;
                }
                value->has_digmethod = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockpos)) {
                    return false;
                }
                value->has_blockpos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemUseCH& value, ProtoWriter& writer) {
    if (value.has_itemid) {
        writer.WriteInt32(1, value.itemid);
    }
    if (value.has_status) {
        writer.WriteInt32(2, value.status);
    }
    if (value.has_shift) {
        writer.WriteInt32(3, value.shift);
    }
    if (value.has_CurSpread) {
        writer.WriteFloat(4, value.CurSpread);
    }
    if (value.has_CurYaw) {
        writer.WriteFloat(5, value.CurYaw);
    }
    if (value.has_CurPitch) {
        writer.WriteFloat(6, value.CurPitch);
    }
    if (value.has_CurPos) {
        ProtoWriter inner;
        Serialize(value.CurPos, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemUseCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->itemid)) {
                    return false;
                }
                value->has_itemid = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->status)) {
                    return false;
                }
                value->has_status = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->shift)) {
                    return false;
                }
                value->has_shift = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->CurSpread)) {
                    return false;
                }
                value->has_CurSpread = true;
                break;
            }
            case 5: {
                if (!reader.ReadFloat(&value->CurYaw)) {
                    return false;
                }
                value->has_CurYaw = true;
                break;
            }
            case 6: {
                if (!reader.ReadFloat(&value->CurPitch)) {
                    return false;
                }
                value->has_CurPitch = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->CurPos)) {
                    return false;
                }
                value->has_CurPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SpecialSkillCH& value, ProtoWriter& writer) {
    if (value.has_status) {
        writer.WriteInt32(1, value.status);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SpecialSkillCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->status)) {
                    return false;
                }
                value->has_status = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetHookCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_hookID) {
        writer.WriteUInt64(2, value.hookID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetHookCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->hookID)) {
                    return false;
                }
                value->has_hookID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemSkillUseCH& value, ProtoWriter& writer) {
    if (value.has_itemid) {
        writer.WriteInt32(1, value.itemid);
    }
    if (value.has_status) {
        writer.WriteInt32(2, value.status);
    }
    if (value.has_skillid) {
        writer.WriteInt32(3, value.skillid);
    }
    if (value.has_CurSpread) {
        writer.WriteFloat(4, value.CurSpread);
    }
    if (value.has_CurPos) {
        ProtoWriter inner;
        Serialize(value.CurPos, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
    if (value.has_CurDirX) {
        writer.WriteFloat(6, value.CurDirX);
    }
    if (value.has_CurDirY) {
        writer.WriteFloat(7, value.CurDirY);
    }
    if (value.has_CurDirZ) {
        writer.WriteFloat(8, value.CurDirZ);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemSkillUseCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->itemid)) {
                    return false;
                }
                value->has_itemid = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->status)) {
                    return false;
                }
                value->has_status = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->skillid)) {
                    return false;
                }
                value->has_skillid = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->CurSpread)) {
                    return false;
                }
                value->has_CurSpread = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->CurPos)) {
                    return false;
                }
                value->has_CurPos = true;
                break;
            }
            case 6: {
                if (!reader.ReadFloat(&value->CurDirX)) {
                    return false;
                }
                value->has_CurDirX = true;
                break;
            }
            case 7: {
                if (!reader.ReadFloat(&value->CurDirY)) {
                    return false;
                }
                value->has_CurDirY = true;
                break;
            }
            case 8: {
                if (!reader.ReadFloat(&value->CurDirZ)) {
                    return false;
                }
                value->has_CurDirZ = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorInteractCH& value, ProtoWriter& writer) {
    if (value.has_itype) {
        writer.WriteInt32(1, value.itype);
    }
    if (value.has_target) {
        writer.WriteUInt64(2, value.target);
    }
    if (value.has_iplot) {
        writer.WriteInt32(3, value.iplot);
    }
    if (value.has_CurYaw) {
        writer.WriteFloat(4, value.CurYaw);
    }
    if (value.has_CurPitch) {
        writer.WriteFloat(5, value.CurPitch);
    }
    if (value.has_CurPos) {
        ProtoWriter inner;
        Serialize(value.CurPos, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorInteractCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->itype)) {
                    return false;
                }
                value->has_itype = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->target)) {
                    return false;
                }
                value->has_target = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->iplot)) {
                    return false;
                }
                value->has_iplot = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->CurYaw)) {
                    return false;
                }
                value->has_CurYaw = true;
                break;
            }
            case 5: {
                if (!reader.ReadFloat(&value->CurPitch)) {
                    return false;
                }
                value->has_CurPitch = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->CurPos)) {
                    return false;
                }
                value->has_CurPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorAnimCH& value, ProtoWriter& writer) {
    if (value.has_anim) {
        writer.WriteInt32(1, value.anim);
    }
    if (value.has_anim1) {
        writer.WriteInt32(2, value.anim1);
    }
    if (value.has_actid) {
        writer.WriteInt32(3, value.actid);
    }
    if (value.has_actidTrigger) {
        writer.WriteInt32(4, value.actidTrigger);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorAnimCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->anim)) {
                    return false;
                }
                value->has_anim = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->anim1)) {
                    return false;
                }
                value->has_anim1 = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->actid)) {
                    return false;
                }
                value->has_actid = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->actidTrigger)) {
                    return false;
                }
                value->has_actidTrigger = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackGridSwapCH& value, ProtoWriter& writer) {
    if (value.has_FromGridId) {
        writer.WriteInt32(1, value.FromGridId);
    }
    if (value.has_ToGridId) {
        writer.WriteInt32(2, value.ToGridId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackGridSwapCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->FromGridId)) {
                    return false;
                }
                value->has_FromGridId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToGridId)) {
                    return false;
                }
                value->has_ToGridId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackMoveItemCH& value, ProtoWriter& writer) {
    if (value.has_FromIndex) {
        writer.WriteInt32(1, value.FromIndex);
    }
    if (value.has_ToIndex) {
        writer.WriteInt32(2, value.ToIndex);
    }
    if (value.has_Num) {
        writer.WriteInt32(3, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackMoveItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->FromIndex)) {
                    return false;
                }
                value->has_FromIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToIndex)) {
                    return false;
                }
                value->has_ToIndex = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackGridDiscardCH& value, ProtoWriter& writer) {
    if (value.has_GridId) {
        writer.WriteInt32(1, value.GridId);
    }
    if (value.has_Num) {
        writer.WriteInt32(2, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackGridDiscardCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridId)) {
                    return false;
                }
                value->has_GridId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackEquipWeaponCH& value, ProtoWriter& writer) {
    if (value.has_GridId) {
        writer.WriteInt32(1, value.GridId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackEquipWeaponCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridId)) {
                    return false;
                }
                value->has_GridId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_NeedContainerPasswordCH& value, ProtoWriter& writer) {
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_Password) {
        writer.WriteInt32(2, value.Password);
    }
    if (value.has_VehicleObjID) {
        writer.WriteUInt64(3, value.VehicleObjID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_NeedContainerPasswordCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Password)) {
                    return false;
                }
                value->has_Password = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt64(&value->VehicleObjID)) {
                    return false;
                }
                value->has_VehicleObjID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloseContainerCH& value, ProtoWriter& writer) {
    if (value.has_BaseIndex) {
        writer.WriteInt32(1, value.BaseIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CloseContainerCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BaseIndex)) {
                    return false;
                }
                value->has_BaseIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetContainerTextCH& value, ProtoWriter& writer) {
    if (value.has_BaseIndex) {
        writer.WriteInt32(1, value.BaseIndex);
    }
    if (value.has_Text) {
        writer.WriteString(2, value.Text);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetContainerTextCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BaseIndex)) {
                    return false;
                }
                value->has_BaseIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->Text)) {
                    return false;
                }
                value->has_Text = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackStoreCH& value, ProtoWriter& writer) {
    if (value.has_FromIndex) {
        writer.WriteInt32(1, value.FromIndex);
    }
    if (value.has_Num) {
        writer.WriteInt32(2, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackStoreCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->FromIndex)) {
                    return false;
                }
                value->has_FromIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackLootCH& value, ProtoWriter& writer) {
    if (value.has_FromIndex) {
        writer.WriteInt32(1, value.FromIndex);
    }
    if (value.has_Num) {
        writer.WriteInt32(2, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackLootCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->FromIndex)) {
                    return false;
                }
                value->has_FromIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackSortCH& value, ProtoWriter& writer) {
    if (value.has_BaseIndex) {
        writer.WriteInt32(1, value.BaseIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackSortCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BaseIndex)) {
                    return false;
                }
                value->has_BaseIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackSetItemCH& value, ProtoWriter& writer) {
    if (value.has_ItemId) {
        writer.WriteInt32(1, value.ItemId);
    }
    if (value.has_ToIndex) {
        writer.WriteInt32(2, value.ToIndex);
    }
    if (value.has_Num) {
        writer.WriteInt32(3, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackSetItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ItemId)) {
                    return false;
                }
                value->has_ItemId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToIndex)) {
                    return false;
                }
                value->has_ToIndex = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackSetItemWithoutLimitCH& value, ProtoWriter& writer) {
    if (value.has_ItemId) {
        writer.WriteInt32(1, value.ItemId);
    }
    if (value.has_ToIndex) {
        writer.WriteInt32(2, value.ToIndex);
    }
    if (value.has_Num) {
        writer.WriteInt32(3, value.Num);
    }
    if (value.has_Userdata_Str) {
        writer.WriteString(4, value.Userdata_Str);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackSetItemWithoutLimitCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ItemId)) {
                    return false;
                }
                value->has_ItemId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToIndex)) {
                    return false;
                }
                value->has_ToIndex = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Userdata_Str)) {
                    return false;
                }
                value->has_Userdata_Str = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_StorageBoxSortCH& value, ProtoWriter& writer) {
    if (value.has_BaseIndex) {
        writer.WriteInt32(1, value.BaseIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_StorageBoxSortCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BaseIndex)) {
                    return false;
                }
                value->has_BaseIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CraftItemCH& value, ProtoWriter& writer) {
    if (value.has_CraftId) {
        writer.WriteInt32(1, value.CraftId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CraftItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->CraftId)) {
                    return false;
                }
                value->has_CraftId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EnchantItemCH& value, ProtoWriter& writer) {
    if (value.has_GridIndex) {
        writer.WriteInt32(1, value.GridIndex);
    }
    if (value.has_FrmGridIndex) {
        writer.WriteInt32(2, value.FrmGridIndex);
    }
    for (const auto& item : value.EnchantIds) {
        writer.WriteInt32(3, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EnchantItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridIndex)) {
                    return false;
                }
                value->has_GridIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->FrmGridIndex)) {
                    return false;
                }
                value->has_FrmGridIndex = true;
                break;
            }
            case 3: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->EnchantIds.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EnchantItemRandomCH& value, ProtoWriter& writer) {
    if (value.has_GridIndex) {
        writer.WriteInt32(1, value.GridIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EnchantItemRandomCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridIndex)) {
                    return false;
                }
                value->has_GridIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RepairItemCH& value, ProtoWriter& writer) {
    if (value.has_GridIndex) {
        writer.WriteInt32(1, value.GridIndex);
    }
    if (value.has_MaterialID) {
        writer.WriteInt32(2, value.MaterialID);
    }
    if (value.has_UseNum) {
        writer.WriteInt32(3, value.UseNum);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RepairItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridIndex)) {
                    return false;
                }
                value->has_GridIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MaterialID)) {
                    return false;
                }
                value->has_MaterialID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->UseNum)) {
                    return false;
                }
                value->has_UseNum = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GunDoReloadCH& value, ProtoWriter& writer) {
    if (value.has_BulletID) {
        writer.WriteInt32(1, value.BulletID);
    }
    if (value.has_Num) {
        writer.WriteInt32(2, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GunDoReloadCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BulletID)) {
                    return false;
                }
                value->has_BulletID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_AccountHorseCH& value, ProtoWriter& writer) {
    if (value.has_HorseID) {
        writer.WriteInt32(1, value.HorseID);
    }
    if (value.has_CmdType) {
        writer.WriteInt32(2, value.CmdType);
    }
    if (value.has_CmdData) {
        writer.WriteInt32(3, value.CmdData);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_AccountHorseCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->HorseID)) {
                    return false;
                }
                value->has_HorseID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->CmdType)) {
                    return false;
                }
                value->has_CmdType = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->CmdData)) {
                    return false;
                }
                value->has_CmdData = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorReviveCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_Type) {
        writer.WriteInt32(2, value.Type);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorReviveCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_JruisdicTionCH& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_JruisdicTionCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ChatCH& value, ProtoWriter& writer) {
    if (value.has_ChatType) {
        writer.WriteInt32(1, value.ChatType);
    }
    if (value.has_TargetUin) {
        writer.WriteInt32(2, value.TargetUin);
    }
    if (value.has_Content) {
        writer.WriteString(3, value.Content);
    }
    if (value.has_Language) {
        writer.WriteInt32(4, value.Language);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ChatCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ChatType)) {
                    return false;
                }
                value->has_ChatType = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->TargetUin)) {
                    return false;
                }
                value->has_TargetUin = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->Content)) {
                    return false;
                }
                value->has_Content = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Language)) {
                    return false;
                }
                value->has_Language = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerMountActorCH& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_IsShapeShift) {
        writer.WriteBool(2, value.IsShapeShift);
    }
    if (value.has_InteractBlockId) {
        writer.WriteInt32(3, value.InteractBlockId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerMountActorCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->IsShapeShift)) {
                    return false;
                }
                value->has_IsShapeShift = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->InteractBlockId)) {
                    return false;
                }
                value->has_InteractBlockId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerMoveInputCH& value, ProtoWriter& writer) {
    if (value.has_MoveForward) {
        writer.WriteFloat(1, value.MoveForward);
    }
    if (value.has_MoveStrafing) {
        writer.WriteFloat(2, value.MoveStrafing);
    }
    if (value.has_Jumping) {
        writer.WriteInt32(3, value.Jumping);
    }
    if (value.has_Sneaking) {
        writer.WriteInt32(4, value.Sneaking);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerMoveInputCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->MoveForward)) {
                    return false;
                }
                value->has_MoveForward = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->MoveStrafing)) {
                    return false;
                }
                value->has_MoveStrafing = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Jumping)) {
                    return false;
                }
                value->has_Jumping = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Sneaking)) {
                    return false;
                }
                value->has_Sneaking = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerSleepCH& value, ProtoWriter& writer) {
    if (value.has_Flags) {
        writer.WriteInt32(1, value.Flags);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerSleepCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Flags)) {
                    return false;
                }
                value->has_Flags = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_NpcTradeCH& value, ProtoWriter& writer) {
    if (value.has_OpType) {
        writer.WriteInt32(1, value.OpType);
    }
    if (value.has_Index) {
        writer.WriteInt32(2, value.Index);
    }
    if (value.has_WatchAD) {
        writer.WriteInt32(3, value.WatchAD);
    }
    if (value.has_RewardNum) {
        writer.WriteInt32(4, value.RewardNum);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_NpcTradeCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->OpType)) {
                    return false;
                }
                value->has_OpType = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Index)) {
                    return false;
                }
                value->has_Index = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->WatchAD)) {
                    return false;
                }
                value->has_WatchAD = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->RewardNum)) {
                    return false;
                }
                value->has_RewardNum = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_YMVoiceCH& value, ProtoWriter& writer) {
    if (value.has_YMMemberID) {
        writer.WriteInt32(1, value.YMMemberID);
    }
    if (value.has_YMSpeakerSwitch) {
        writer.WriteInt32(2, value.YMSpeakerSwitch);
    }
    if (value.has_YMMicSwitch) {
        writer.WriteInt32(3, value.YMMicSwitch);
    }
    if (value.has_YMMemberRole) {
        writer.WriteInt32(4, value.YMMemberRole);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_YMVoiceCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->YMMemberID)) {
                    return false;
                }
                value->has_YMMemberID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->YMSpeakerSwitch)) {
                    return false;
                }
                value->has_YMSpeakerSwitch = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->YMMicSwitch)) {
                    return false;
                }
                value->has_YMMicSwitch = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->YMMemberRole)) {
                    return false;
                }
                value->has_YMMemberRole = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GVChangeRoleCH& value, ProtoWriter& writer) {
    if (value.has_ChangeResult) {
        writer.WriteInt32(1, value.ChangeResult);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GVChangeRoleCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ChangeResult)) {
                    return false;
                }
                value->has_ChangeResult = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_YMChangeRoleCH& value, ProtoWriter& writer) {
    if (value.has_ChangeResult) {
        writer.WriteInt32(1, value.ChangeResult);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_YMChangeRoleCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ChangeResult)) {
                    return false;
                }
                value->has_ChangeResult = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GetAccountItemsCH& value, ProtoWriter& writer) {
    if (value.has_ItemId) {
        writer.WriteInt32(1, value.ItemId);
    }
    if (value.has_Num) {
        writer.WriteInt32(2, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GetAccountItemsCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ItemId)) {
                    return false;
                }
                value->has_ItemId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SpecialItemUseCH& value, ProtoWriter& writer) {
    if (value.has_GridIndex) {
        writer.WriteInt32(1, value.GridIndex);
    }
    if (value.has_ItemId) {
        writer.WriteInt32(2, value.ItemId);
    }
    if (value.has_ItemNum) {
        writer.WriteInt32(3, value.ItemNum);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SpecialItemUseCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridIndex)) {
                    return false;
                }
                value->has_GridIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ItemId)) {
                    return false;
                }
                value->has_ItemId = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ItemNum)) {
                    return false;
                }
                value->has_ItemNum = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetSpectatorModeCH& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_SpectatorMode) {
        writer.WriteInt32(2, value.SpectatorMode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetSpectatorModeCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SpectatorMode)) {
                    return false;
                }
                value->has_SpectatorMode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetSpectatorTypeCH& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_SpectatorType) {
        writer.WriteInt32(2, value.SpectatorType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetSpectatorTypeCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SpectatorType)) {
                    return false;
                }
                value->has_SpectatorType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetSpectatorPlayerCH& value, ProtoWriter& writer) {
    if (value.has_SpectatorUin) {
        writer.WriteInt32(1, value.SpectatorUin);
    }
    if (value.has_ToSpectatorUin) {
        writer.WriteInt32(2, value.ToSpectatorUin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetSpectatorPlayerCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpectatorUin)) {
                    return false;
                }
                value->has_SpectatorUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToSpectatorUin)) {
                    return false;
                }
                value->has_ToSpectatorUin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetPlayerModelAniCH& value, ProtoWriter& writer) {
    if (value.has_SpectatorUin) {
        writer.WriteInt32(1, value.SpectatorUin);
    }
    if (value.has_ToSpectatorUin) {
        writer.WriteInt32(2, value.ToSpectatorUin);
    }
    if (value.has_ModelAnimalType) {
        writer.WriteInt32(3, value.ModelAnimalType);
    }
    if (value.has_ModelAnimalExt) {
        writer.WriteInt32(4, value.ModelAnimalExt);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetPlayerModelAniCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpectatorUin)) {
                    return false;
                }
                value->has_SpectatorUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToSpectatorUin)) {
                    return false;
                }
                value->has_ToSpectatorUin = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ModelAnimalType)) {
                    return false;
                }
                value->has_ModelAnimalType = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->ModelAnimalExt)) {
                    return false;
                }
                value->has_ModelAnimalExt = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SendMyViewmodeToSpectatorCH& value, ProtoWriter& writer) {
    if (value.has_SpectatorUin) {
        writer.WriteInt32(1, value.SpectatorUin);
    }
    if (value.has_ToSpectatorUin) {
        writer.WriteInt32(2, value.ToSpectatorUin);
    }
    if (value.has_MyViewmode) {
        writer.WriteInt32(3, value.MyViewmode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SendMyViewmodeToSpectatorCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpectatorUin)) {
                    return false;
                }
                value->has_SpectatorUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToSpectatorUin)) {
                    return false;
                }
                value->has_ToSpectatorUin = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->MyViewmode)) {
                    return false;
                }
                value->has_MyViewmode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetBobbingToSpectatorCH& value, ProtoWriter& writer) {
    if (value.has_SpectatorUin) {
        writer.WriteInt32(1, value.SpectatorUin);
    }
    if (value.has_ToSpectatorUin) {
        writer.WriteInt32(2, value.ToSpectatorUin);
    }
    if (value.has_Bobbing) {
        writer.WriteInt32(3, value.Bobbing);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetBobbingToSpectatorCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpectatorUin)) {
                    return false;
                }
                value->has_SpectatorUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToSpectatorUin)) {
                    return false;
                }
                value->has_ToSpectatorUin = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Bobbing)) {
                    return false;
                }
                value->has_Bobbing = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BallOperateCH& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ActorID) {
        writer.WriteUInt64(2, value.ActorID);
    }
    if (value.has_ExtendData) {
        writer.WriteInt32(3, value.ExtendData);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BallOperateCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ExtendData)) {
                    return false;
                }
                value->has_ExtendData = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RocketTeleportCH& value, ProtoWriter& writer) {
    if (value.has_MapId) {
        writer.WriteInt32(1, value.MapId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RocketTeleportCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->MapId)) {
                    return false;
                }
                value->has_MapId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloseDialogueCH& value, ProtoWriter& writer) {
}

inline bool Deserialize(ProtoReader& reader, PB_CloseDialogueCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_AnswerTaskCH& value, ProtoWriter& writer) {
    if (value.has_TaskID) {
        writer.WriteInt32(1, value.TaskID);
    }
    if (value.has_PlotID) {
        writer.WriteInt32(2, value.PlotID);
    }
    if (value.has_Type) {
        writer.WriteInt32(3, value.Type);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_AnswerTaskCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->TaskID)) {
                    return false;
                }
                value->has_TaskID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->PlotID)) {
                    return false;
                }
                value->has_PlotID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CompleteTaskCH& value, ProtoWriter& writer) {
    if (value.has_TaskID) {
        writer.WriteInt32(1, value.TaskID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CompleteTaskCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->TaskID)) {
                    return false;
                }
                value->has_TaskID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayActCH& value, ProtoWriter& writer) {
    if (value.has_ActID) {
        writer.WriteInt32(1, value.ActID);
    }
    if (value.has_ActIDTrigger) {
        writer.WriteInt32(2, value.ActIDTrigger);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayActCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ActID)) {
                    return false;
                }
                value->has_ActID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ActIDTrigger)) {
                    return false;
                }
                value->has_ActIDTrigger = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BluePrintPreBlockCH& value, ProtoWriter& writer) {
    if (value.has_blockpos) {
        ProtoWriter inner;
        Serialize(value.blockpos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BluePrintPreBlockCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockpos)) {
                    return false;
                }
                value->has_blockpos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GravityOperateCH& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ActorID) {
        writer.WriteUInt64(2, value.ActorID);
    }
    if (value.has_ExtendData) {
        writer.WriteInt32(3, value.ExtendData);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GravityOperateCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ExtendData)) {
                    return false;
                }
                value->has_ExtendData = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MakeCustomModelCH& value, ProtoWriter& writer) {
    if (value.has_Point) {
        ProtoWriter inner;
        Serialize(value.Point, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_ModelName) {
        writer.WriteString(2, value.ModelName);
    }
    if (value.has_ModelDesc) {
        writer.WriteString(3, value.ModelDesc);
    }
    if (value.has_ModelType) {
        writer.WriteInt32(4, value.ModelType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MakeCustomModelCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Point)) {
                    return false;
                }
                value->has_Point = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->ModelName)) {
                    return false;
                }
                value->has_ModelName = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->ModelDesc)) {
                    return false;
                }
                value->has_ModelDesc = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->ModelType)) {
                    return false;
                }
                value->has_ModelType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SelectMobSpawnerCH& value, ProtoWriter& writer) {
    if (value.has_Point) {
        ProtoWriter inner;
        Serialize(value.Point, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_MobResID) {
        writer.WriteInt32(2, value.MobResID);
    }
    if (value.has_SpawnCount) {
        writer.WriteInt32(3, value.SpawnCount);
    }
    if (value.has_MaxNearbyMobs) {
        writer.WriteInt32(4, value.MaxNearbyMobs);
    }
    if (value.has_SpawnWide) {
        writer.WriteInt32(5, value.SpawnWide);
    }
    if (value.has_SpawnHigh) {
        writer.WriteInt32(6, value.SpawnHigh);
    }
    if (value.has_MinSpawnDelay) {
        writer.WriteInt32(7, value.MinSpawnDelay);
    }
    if (value.has_IsNumberDetection) {
        writer.WriteInt32(8, value.IsNumberDetection);
    }
    if (value.has_IsSpawnDelay) {
        writer.WriteInt32(9, value.IsSpawnDelay);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SelectMobSpawnerCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Point)) {
                    return false;
                }
                value->has_Point = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MobResID)) {
                    return false;
                }
                value->has_MobResID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->SpawnCount)) {
                    return false;
                }
                value->has_SpawnCount = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->MaxNearbyMobs)) {
                    return false;
                }
                value->has_MaxNearbyMobs = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->SpawnWide)) {
                    return false;
                }
                value->has_SpawnWide = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->SpawnHigh)) {
                    return false;
                }
                value->has_SpawnHigh = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->MinSpawnDelay)) {
                    return false;
                }
                value->has_MinSpawnDelay = true;
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->IsNumberDetection)) {
                    return false;
                }
                value->has_IsNumberDetection = true;
                break;
            }
            case 9: {
                if (!reader.ReadInt32(&value->IsSpawnDelay)) {
                    return false;
                }
                value->has_IsSpawnDelay = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GetNpcShopInfoCH& value, ProtoWriter& writer) {
    if (value.has_ShopID) {
        writer.WriteInt32(1, value.ShopID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GetNpcShopInfoCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ShopID)) {
                    return false;
                }
                value->has_ShopID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BuyNpcShopItemCH& value, ProtoWriter& writer) {
    if (value.has_ShopID) {
        writer.WriteInt32(1, value.ShopID);
    }
    if (value.has_SkuID) {
        writer.WriteInt32(2, value.SkuID);
    }
    if (value.has_BuyCount) {
        writer.WriteInt32(3, value.BuyCount);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BuyNpcShopItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ShopID)) {
                    return false;
                }
                value->has_ShopID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SkuID)) {
                    return false;
                }
                value->has_SkuID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->BuyCount)) {
                    return false;
                }
                value->has_BuyCount = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloseEditActorModelCH& value, ProtoWriter& writer) {
    for (const auto& item : value.BoneModels) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_OperateType) {
        writer.WriteInt32(2, value.OperateType);
    }
    if (value.has_ModelType) {
        writer.WriteInt32(3, value.ModelType);
    }
    if (value.has_ModelName) {
        writer.WriteString(4, value.ModelName);
    }
    if (value.has_SkinDisplay) {
        writer.WriteBool(5, value.SkinDisplay);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CloseEditActorModelCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ActorOneBoneModelData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->BoneModels.push_back(item);
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->OperateType)) {
                    return false;
                }
                value->has_OperateType = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ModelType)) {
                    return false;
                }
                value->has_ModelType = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->ModelName)) {
                    return false;
                }
                value->has_ModelName = true;
                break;
            }
            case 5: {
                if (!reader.ReadBool(&value->SkinDisplay)) {
                    return false;
                }
                value->has_SkinDisplay = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PackGiftNotifyItemChgCH& value, ProtoWriter& writer) {
    if (value.has_ShortCutIdx) {
        writer.WriteInt32(1, value.ShortCutIdx);
    }
    if (value.has_CostItemInfo) {
        writer.WriteInt32(2, value.CostItemInfo);
    }
    for (const auto& item : value.addlist) {
        writer.WriteInt32(3, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PackGiftNotifyItemChgCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ShortCutIdx)) {
                    return false;
                }
                value->has_ShortCutIdx = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->CostItemInfo)) {
                    return false;
                }
                value->has_CostItemInfo = true;
                break;
            }
            case 3: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->addlist.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehiclePreBlockCH& value, ProtoWriter& writer) {
    if (value.has_blockpos) {
        ProtoWriter inner;
        Serialize(value.blockpos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehiclePreBlockCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockpos)) {
                    return false;
                }
                value->has_blockpos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleItemUseCH& value, ProtoWriter& writer) {
    if (value.has_ShortCutIdx) {
        writer.WriteInt32(1, value.ShortCutIdx);
    }
    if (value.has_dir) {
        writer.WriteInt32(2, value.dir);
    }
    if (value.has_pos) {
        ProtoWriter inner;
        Serialize(value.pos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleItemUseCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ShortCutIdx)) {
                    return false;
                }
                value->has_ShortCutIdx = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->dir)) {
                    return false;
                }
                value->has_dir = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->pos)) {
                    return false;
                }
                value->has_pos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleStartBlockCH& value, ProtoWriter& writer) {
    if (value.has_blockpos) {
        ProtoWriter inner;
        Serialize(value.blockpos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleStartBlockCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockpos)) {
                    return false;
                }
                value->has_blockpos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAttribChangeCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_Fuel) {
        writer.WriteInt32(2, value.Fuel);
    }
    if (value.has_PartIndex) {
        writer.WriteInt32(3, value.PartIndex);
    }
    if (value.has_EngineState) {
        writer.WriteInt32(4, value.EngineState);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAttribChangeCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Fuel)) {
                    return false;
                }
                value->has_Fuel = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->PartIndex)) {
                    return false;
                }
                value->has_PartIndex = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->EngineState)) {
                    return false;
                }
                value->has_EngineState = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_WorkshopItemInfoCH& value, ProtoWriter& writer) {
    if (value.has_ContainerPos) {
        ProtoWriter inner;
        Serialize(value.ContainerPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_WorkshopItemInfoCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ContainerPos)) {
                    return false;
                }
                value->has_ContainerPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerVehicleMoveInputCH& value, ProtoWriter& writer) {
    if (value.has_Accel) {
        writer.WriteFloat(1, value.Accel);
    }
    if (value.has_Brake) {
        writer.WriteFloat(2, value.Brake);
    }
    if (value.has_Left) {
        writer.WriteFloat(3, value.Left);
    }
    if (value.has_Right) {
        writer.WriteFloat(4, value.Right);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerVehicleMoveInputCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->Accel)) {
                    return false;
                }
                value->has_Accel = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Brake)) {
                    return false;
                }
                value->has_Brake = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->Left)) {
                    return false;
                }
                value->has_Left = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->Right)) {
                    return false;
                }
                value->has_Right = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerResetVehicleCH& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerResetVehicleCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerMotionStateChangeCH& value, ProtoWriter& writer) {
    if (value.has_StateType) {
        writer.WriteInt32(1, value.StateType);
    }
    if (value.has_StateSwitch) {
        writer.WriteBool(2, value.StateSwitch);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerMotionStateChangeCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->StateType)) {
                    return false;
                }
                value->has_StateType = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->StateSwitch)) {
                    return false;
                }
                value->has_StateSwitch = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerClickCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_ActorID) {
        writer.WriteInt32(2, value.ActorID);
    }
    if (value.has_BlockID) {
        writer.WriteInt32(3, value.BlockID);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerClickCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->BlockID)) {
                    return false;
                }
                value->has_BlockID = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerSelectShortcutCH& value, ProtoWriter& writer) {
    if (value.has_objid) {
        writer.WriteUInt64(1, value.objid);
    }
    if (value.has_toolID) {
        writer.WriteInt32(2, value.toolID);
    }
    if (value.has_itemNum) {
        writer.WriteInt32(3, value.itemNum);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerSelectShortcutCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->objid)) {
                    return false;
                }
                value->has_objid = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->toolID)) {
                    return false;
                }
                value->has_toolID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->itemNum)) {
                    return false;
                }
                value->has_itemNum = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TRIGGERPLAYERATTRICH& value, ProtoWriter& writer) {
    for (const auto& item : value.ObActorAttrList) {
        writer.WriteInt32(1, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TRIGGERPLAYERATTRICH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->ObActorAttrList.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ReqDownLoadResUrlCH& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ExternData) {
        writer.WriteString(2, value.ExternData);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ReqDownLoadResUrlCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->ExternData)) {
                    return false;
                }
                value->has_ExternData = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloseFullyCustomModelUICH& value, ProtoWriter& writer) {
    if (value.has_OperateType) {
        writer.WriteInt32(1, value.OperateType);
    }
    if (value.has_Url) {
        writer.WriteString(2, value.Url);
    }
    if (value.has_Skey) {
        writer.WriteString(3, value.Skey);
    }
    if (value.has_version) {
        writer.WriteInt32(4, value.version);
    }
    if (value.has_Name) {
        writer.WriteString(5, value.Name);
    }
    if (value.has_Desc) {
        writer.WriteString(6, value.Desc);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CloseFullyCustomModelUICH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->OperateType)) {
                    return false;
                }
                value->has_OperateType = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->Url)) {
                    return false;
                }
                value->has_Url = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->Skey)) {
                    return false;
                }
                value->has_Skey = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->version)) {
                    return false;
                }
                value->has_version = true;
                break;
            }
            case 5: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 6: {
                if (!reader.ReadString(&value->Desc)) {
                    return false;
                }
                value->has_Desc = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerNavFinishedCH& value, ProtoWriter& writer) {
    if (value.has_objid) {
        writer.WriteInt32(1, value.objid);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerNavFinishedCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->objid)) {
                    return false;
                }
                value->has_objid = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAssembleLineCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_from) {
        writer.WriteInt32(2, value.from);
    }
    if (value.has_to) {
        writer.WriteInt32(3, value.to);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->from)) {
                    return false;
                }
                value->has_from = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->to)) {
                    return false;
                }
                value->has_to = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAssembleLineOperateCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_Blockid) {
        writer.WriteInt32(2, value.Blockid);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_IsClicked) {
        writer.WriteBool(4, value.IsClicked);
    }
    if (value.has_Type) {
        writer.WriteInt32(5, value.Type);
    }
    if (value.has_KeyId) {
        writer.WriteInt32(6, value.KeyId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineOperateCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Blockid)) {
                    return false;
                }
                value->has_Blockid = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            case 4: {
                if (!reader.ReadBool(&value->IsClicked)) {
                    return false;
                }
                value->has_IsClicked = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->KeyId)) {
                    return false;
                }
                value->has_KeyId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_UpdateActionerDataCH& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_datastr) {
        writer.WriteString(3, value.datastr);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_UpdateActionerDataCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->datastr)) {
                    return false;
                }
                value->has_datastr = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CSPlayerPermitCH& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_Bit) {
        writer.WriteInt32(2, value.Bit);
    }
    if (value.has_BitVal) {
        writer.WriteBool(3, value.BitVal);
    }
    if (value.has_BanItem) {
        writer.WriteInt32(4, value.BanItem);
    }
    if (value.has_IsBan) {
        writer.WriteBool(5, value.IsBan);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CSPlayerPermitCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Bit)) {
                    return false;
                }
                value->has_Bit = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->BitVal)) {
                    return false;
                }
                value->has_BitVal = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->BanItem)) {
                    return false;
                }
                value->has_BanItem = true;
                break;
            }
            case 5: {
                if (!reader.ReadBool(&value->IsBan)) {
                    return false;
                }
                value->has_IsBan = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleWorkshopLineCH& value, ProtoWriter& writer) {
    if (value.has_ContainerPos) {
        ProtoWriter inner;
        Serialize(value.ContainerPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_FromPos) {
        ProtoWriter inner;
        Serialize(value.FromPos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_ToPos) {
        ProtoWriter inner;
        Serialize(value.ToPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleWorkshopLineCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ContainerPos)) {
                    return false;
                }
                value->has_ContainerPos = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->FromPos)) {
                    return false;
                }
                value->has_FromPos = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ToPos)) {
                    return false;
                }
                value->has_ToPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CSChangePlayerTeamCH& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_TeamId) {
        writer.WriteInt32(2, value.TeamId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CSChangePlayerTeamCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->TeamId)) {
                    return false;
                }
                value->has_TeamId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CSRentRoomAutoMuteCH& value, ProtoWriter& writer) {
    if (value.has_SpamPreventionMinutes) {
        writer.WriteUInt32(1, value.SpamPreventionMinutes);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CSRentRoomAutoMuteCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->SpamPreventionMinutes)) {
                    return false;
                }
                value->has_SpamPreventionMinutes = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAssembleLineUpdateCH& value, ProtoWriter& writer) {
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineUpdateCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MapEditHandleCH& value, ProtoWriter& writer) {
    if (value.has_EditMode) {
        writer.WriteInt32(1, value.EditMode);
    }
    if (value.has_ShapeType) {
        writer.WriteInt32(2, value.ShapeType);
    }
    if (value.has_HandleType) {
        writer.WriteInt32(3, value.HandleType);
    }
    if (value.has_ExtraParamLength) {
        writer.WriteInt32(4, value.ExtraParamLength);
    }
    if (value.has_ExtraParamWidth) {
        writer.WriteInt32(5, value.ExtraParamWidth);
    }
    if (value.has_ExtraParamHeight) {
        writer.WriteInt32(6, value.ExtraParamHeight);
    }
    if (value.has_ExtraParamRadius) {
        writer.WriteInt32(7, value.ExtraParamRadius);
    }
    if (value.has_ExtraParamIsSolid) {
        writer.WriteBool(8, value.ExtraParamIsSolid);
    }
    if (value.has_ExtraParamScale) {
        writer.WriteInt32(9, value.ExtraParamScale);
    }
    if (value.has_ExtraParamIsContinue) {
        writer.WriteBool(10, value.ExtraParamIsContinue);
    }
    if (value.has_ExtraParamDir) {
        writer.WriteInt32(11, value.ExtraParamDir);
    }
    if (value.has_ExtraParamFillerId) {
        writer.WriteInt32(12, value.ExtraParamFillerId);
    }
    if (value.has_ExtraParamReplaceId) {
        writer.WriteInt32(13, value.ExtraParamReplaceId);
    }
    if (value.has_ExtraParamDistance) {
        writer.WriteInt32(14, value.ExtraParamDistance);
    }
    if (value.has_ExtraParamOffsetX) {
        writer.WriteInt32(15, value.ExtraParamOffsetX);
    }
    if (value.has_ExtraParamOffsetY) {
        writer.WriteInt32(16, value.ExtraParamOffsetY);
    }
    if (value.has_ExtraParamOffsetZ) {
        writer.WriteInt32(17, value.ExtraParamOffsetZ);
    }
    if (value.has_ExtraParamRotateAngle) {
        writer.WriteInt32(18, value.ExtraParamRotateAngle);
    }
    if (value.has_ExtraParamTwoPointCPDir) {
        writer.WriteInt32(19, value.ExtraParamTwoPointCPDir);
    }
    if (value.has_ExtraParamIsOnlyFillAtmosphere) {
        writer.WriteBool(20, value.ExtraParamIsOnlyFillAtmosphere);
    }
    if (value.has_CenterPos) {
        ProtoWriter inner;
        Serialize(value.CenterPos, inner);
        writer.WriteMessage(21, inner.Bytes());
    }
    if (value.has_BeginPos) {
        ProtoWriter inner;
        Serialize(value.BeginPos, inner);
        writer.WriteMessage(22, inner.Bytes());
    }
    if (value.has_EndPos) {
        ProtoWriter inner;
        Serialize(value.EndPos, inner);
        writer.WriteMessage(23, inner.Bytes());
    }
    if (value.has_SaveCmd) {
        writer.WriteInt32(24, value.SaveCmd);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MapEditHandleCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->EditMode)) {
                    return false;
                }
                value->has_EditMode = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ShapeType)) {
                    return false;
                }
                value->has_ShapeType = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->HandleType)) {
                    return false;
                }
                value->has_HandleType = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->ExtraParamLength)) {
                    return false;
                }
                value->has_ExtraParamLength = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->ExtraParamWidth)) {
                    return false;
                }
                value->has_ExtraParamWidth = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->ExtraParamHeight)) {
                    return false;
                }
                value->has_ExtraParamHeight = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->ExtraParamRadius)) {
                    return false;
                }
                value->has_ExtraParamRadius = true;
                break;
            }
            case 8: {
                if (!reader.ReadBool(&value->ExtraParamIsSolid)) {
                    return false;
                }
                value->has_ExtraParamIsSolid = true;
                break;
            }
            case 9: {
                if (!reader.ReadInt32(&value->ExtraParamScale)) {
                    return false;
                }
                value->has_ExtraParamScale = true;
                break;
            }
            case 10: {
                if (!reader.ReadBool(&value->ExtraParamIsContinue)) {
                    return false;
                }
                value->has_ExtraParamIsContinue = true;
                break;
            }
            case 11: {
                if (!reader.ReadInt32(&value->ExtraParamDir)) {
                    return false;
                }
                value->has_ExtraParamDir = true;
                break;
            }
            case 12: {
                if (!reader.ReadInt32(&value->ExtraParamFillerId)) {
                    return false;
                }
                value->has_ExtraParamFillerId = true;
                break;
            }
            case 13: {
                if (!reader.ReadInt32(&value->ExtraParamReplaceId)) {
                    return false;
                }
                value->has_ExtraParamReplaceId = true;
                break;
            }
            case 14: {
                if (!reader.ReadInt32(&value->ExtraParamDistance)) {
                    return false;
                }
                value->has_ExtraParamDistance = true;
                break;
            }
            case 15: {
                if (!reader.ReadInt32(&value->ExtraParamOffsetX)) {
                    return false;
                }
                value->has_ExtraParamOffsetX = true;
                break;
            }
            case 16: {
                if (!reader.ReadInt32(&value->ExtraParamOffsetY)) {
                    return false;
                }
                value->has_ExtraParamOffsetY = true;
                break;
            }
            case 17: {
                if (!reader.ReadInt32(&value->ExtraParamOffsetZ)) {
                    return false;
                }
                value->has_ExtraParamOffsetZ = true;
                break;
            }
            case 18: {
                if (!reader.ReadInt32(&value->ExtraParamRotateAngle)) {
                    return false;
                }
                value->has_ExtraParamRotateAngle = true;
                break;
            }
            case 19: {
                if (!reader.ReadInt32(&value->ExtraParamTwoPointCPDir)) {
                    return false;
                }
                value->has_ExtraParamTwoPointCPDir = true;
                break;
            }
            case 20: {
                if (!reader.ReadBool(&value->ExtraParamIsOnlyFillAtmosphere)) {
                    return false;
                }
                value->has_ExtraParamIsOnlyFillAtmosphere = true;
                break;
            }
            case 21: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->CenterPos)) {
                    return false;
                }
                value->has_CenterPos = true;
                break;
            }
            case 22: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BeginPos)) {
                    return false;
                }
                value->has_BeginPos = true;
                break;
            }
            case 23: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->EndPos)) {
                    return false;
                }
                value->has_EndPos = true;
                break;
            }
            case 24: {
                if (!reader.ReadInt32(&value->SaveCmd)) {
                    return false;
                }
                value->has_SaveCmd = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MapEditRevokeCH& value, ProtoWriter& writer) {
    if (value.has_Revoke) {
        writer.WriteBool(1, value.Revoke);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MapEditRevokeCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadBool(&value->Revoke)) {
                    return false;
                }
                value->has_Revoke = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloudRoomOwnerStartGameCH& value, ProtoWriter& writer) {
}

inline bool Deserialize(ProtoReader& reader, PB_CloudRoomOwnerStartGameCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CSKickOffDataCH& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_KickerType) {
        writer.WriteInt32(2, value.KickerType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CSKickOffDataCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->KickerType)) {
                    return false;
                }
                value->has_KickerType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_UsePackingFCMItemCH& value, ProtoWriter& writer) {
    if (value.has_ItemId) {
        writer.WriteInt32(1, value.ItemId);
    }
    if (value.has_UsePos) {
        ProtoWriter inner;
        Serialize(value.UsePos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_UsePackingFCMItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ItemId)) {
                    return false;
                }
                value->has_ItemId = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->UsePos)) {
                    return false;
                }
                value->has_UsePos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CreatePackingCMCH& value, ProtoWriter& writer) {
    if (value.has_OffsetPos) {
        ProtoWriter inner;
        Serialize(value.OffsetPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_RotateType) {
        writer.WriteInt32(2, value.RotateType);
    }
    if (value.has_Name) {
        writer.WriteString(3, value.Name);
    }
    if (value.has_Desc) {
        writer.WriteString(4, value.Desc);
    }
    if (value.has_CreateType) {
        writer.WriteInt32(5, value.CreateType);
    }
    if (value.has_StartPos) {
        ProtoWriter inner;
        Serialize(value.StartPos, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
    if (value.has_EndPos) {
        ProtoWriter inner;
        Serialize(value.EndPos, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CreatePackingCMCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->OffsetPos)) {
                    return false;
                }
                value->has_OffsetPos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->RotateType)) {
                    return false;
                }
                value->has_RotateType = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Desc)) {
                    return false;
                }
                value->has_Desc = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->CreateType)) {
                    return false;
                }
                value->has_CreateType = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->StartPos)) {
                    return false;
                }
                value->has_StartPos = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->EndPos)) {
                    return false;
                }
                value->has_EndPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_InputContentCH& value, ProtoWriter& writer) {
    if (value.has_objId) {
        writer.WriteUInt64(1, value.objId);
    }
    if (value.has_content) {
        writer.WriteString(2, value.content);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_InputContentCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->objId)) {
                    return false;
                }
                value->has_objId = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->content)) {
                    return false;
                }
                value->has_content = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_InputKeyCH& value, ProtoWriter& writer) {
    if (value.has_objId) {
        writer.WriteUInt64(1, value.objId);
    }
    if (value.has_keyType) {
        writer.WriteInt32(2, value.keyType);
    }
    if (value.has_eventType) {
        writer.WriteString(3, value.eventType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_InputKeyCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->objId)) {
                    return false;
                }
                value->has_objId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->keyType)) {
                    return false;
                }
                value->has_keyType = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->eventType)) {
                    return false;
                }
                value->has_eventType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SensorContainerDataCH& value, ProtoWriter& writer) {
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_SensorValue) {
        writer.WriteInt32(2, value.SensorValue);
    }
    if (value.has_IsBreverse) {
        writer.WriteBool(3, value.IsBreverse);
    }
    if (value.has_ObjID) {
        writer.WriteUInt64(4, value.ObjID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SensorContainerDataCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SensorValue)) {
                    return false;
                }
                value->has_SensorValue = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->IsBreverse)) {
                    return false;
                }
                value->has_IsBreverse = true;
                break;
            }
            case 4: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerCarryActorCH& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerCarryActorCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VillagerModifyName& value, ProtoWriter& writer) {
    if (value.has_ObjId) {
        writer.WriteUInt64(1, value.ObjId);
    }
    if (value.has_Name) {
        writer.WriteString(2, value.Name);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VillagerModifyName* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerGotoPosCH& value, ProtoWriter& writer) {
    if (value.has_ObjId) {
        writer.WriteUInt64(1, value.ObjId);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerGotoPosCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CustomModelPrepareCH& value, ProtoWriter& writer) {
    if (value.has_Index) {
        writer.WriteInt32(2, value.Index);
    }
    if (value.has_HaveFile) {
        writer.WriteInt32(3, value.HaveFile);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CustomModelPrepareCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 2: {
                if (!reader.ReadInt32(&value->Index)) {
                    return false;
                }
                value->has_Index = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->HaveFile)) {
                    return false;
                }
                value->has_HaveFile = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MoveMobBackpackItemCH& value, ProtoWriter& writer) {
    if (value.has_gridIndex) {
        writer.WriteInt32(1, value.gridIndex);
    }
    if (value.has_moveType) {
        writer.WriteInt32(2, value.moveType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MoveMobBackpackItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->gridIndex)) {
                    return false;
                }
                value->has_gridIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->moveType)) {
                    return false;
                }
                value->has_moveType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_InteractMobBackpackItemCH& value, ProtoWriter& writer) {
    if (value.has_fromIndex) {
        writer.WriteInt32(1, value.fromIndex);
    }
    if (value.has_toIndex) {
        writer.WriteInt32(2, value.toIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_InteractMobBackpackItemCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->fromIndex)) {
                    return false;
                }
                value->has_fromIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->toIndex)) {
                    return false;
                }
                value->has_toIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_AltarLuckyDrawCH& value, ProtoWriter& writer) {
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_AltarLuckyDrawCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerRestoreTransformSkinCH& value, ProtoWriter& writer) {
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerRestoreTransformSkinCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerDeformationSkinCH& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_IsShapeShift) {
        writer.WriteBool(2, value.IsShapeShift);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerDeformationSkinCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->IsShapeShift)) {
                    return false;
                }
                value->has_IsShapeShift = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerResetDeformationCH& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_IsShapeShift) {
        writer.WriteBool(2, value.IsShapeShift);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerResetDeformationCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->IsShapeShift)) {
                    return false;
                }
                value->has_IsShapeShift = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerBaseAttrCH& value, ProtoWriter& writer) {
    if (value.has_CurHP) {
        writer.WriteFloat(1, value.CurHP);
    }
    if (value.has_MaxHP) {
        writer.WriteFloat(2, value.MaxHP);
    }
    if (value.has_CurFoodLv) {
        writer.WriteFloat(3, value.CurFoodLv);
    }
    if (value.has_MaxFoodLv) {
        writer.WriteFloat(4, value.MaxFoodLv);
    }
    if (value.has_BaseSpeed) {
        writer.WriteFloat(5, value.BaseSpeed);
    }
    if (value.has_AttackPunch) {
        writer.WriteFloat(6, value.AttackPunch);
    }
    if (value.has_AttackRange) {
        writer.WriteFloat(7, value.AttackRange);
    }
    if (value.has_DefensePunch) {
        writer.WriteFloat(8, value.DefensePunch);
    }
    if (value.has_DefenseRange) {
        writer.WriteFloat(9, value.DefenseRange);
    }
    if (value.has_CurExp) {
        writer.WriteInt32(10, value.CurExp);
    }
    if (value.has_CurSexp) {
        writer.WriteInt32(11, value.CurSexp);
    }
    if (value.has_CurLevel) {
        writer.WriteInt32(12, value.CurLevel);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerBaseAttrCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->CurHP)) {
                    return false;
                }
                value->has_CurHP = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->MaxHP)) {
                    return false;
                }
                value->has_MaxHP = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->CurFoodLv)) {
                    return false;
                }
                value->has_CurFoodLv = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->MaxFoodLv)) {
                    return false;
                }
                value->has_MaxFoodLv = true;
                break;
            }
            case 5: {
                if (!reader.ReadFloat(&value->BaseSpeed)) {
                    return false;
                }
                value->has_BaseSpeed = true;
                break;
            }
            case 6: {
                if (!reader.ReadFloat(&value->AttackPunch)) {
                    return false;
                }
                value->has_AttackPunch = true;
                break;
            }
            case 7: {
                if (!reader.ReadFloat(&value->AttackRange)) {
                    return false;
                }
                value->has_AttackRange = true;
                break;
            }
            case 8: {
                if (!reader.ReadFloat(&value->DefensePunch)) {
                    return false;
                }
                value->has_DefensePunch = true;
                break;
            }
            case 9: {
                if (!reader.ReadFloat(&value->DefenseRange)) {
                    return false;
                }
                value->has_DefenseRange = true;
                break;
            }
            case 10: {
                if (!reader.ReadInt32(&value->CurExp)) {
                    return false;
                }
                value->has_CurExp = true;
                break;
            }
            case 11: {
                if (!reader.ReadInt32(&value->CurSexp)) {
                    return false;
                }
                value->has_CurSexp = true;
                break;
            }
            case 12: {
                if (!reader.ReadInt32(&value->CurLevel)) {
                    return false;
                }
                value->has_CurLevel = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerArchEntityCH& value, ProtoWriter& writer) {
    if (value.has_BaseAttr) {
        ProtoWriter inner;
        Serialize(value.BaseAttr, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    for (const auto& item : value.RoleBuffs) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    for (const auto& item : value.RoleEquips) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    for (const auto& item : value.RoleBackpack) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    for (const auto& item : value.RoleShortcut) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
    if (value.has_CloudVars) {
        writer.WriteString(6, value.CloudVars);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerArchEntityCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BaseAttr)) {
                    return false;
                }
                value->has_BaseAttr = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ActorBuff item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->RoleBuffs.push_back(item);
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->RoleEquips.push_back(item);
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->RoleBackpack.push_back(item);
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->RoleShortcut.push_back(item);
                break;
            }
            case 6: {
                if (!reader.ReadString(&value->CloudVars)) {
                    return false;
                }
                value->has_CloudVars = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PrayTreeTimeCH& value, ProtoWriter& writer) {
    if (value.has_uin) {
        writer.WriteInt32(1, value.uin);
    }
    if (value.has_stage) {
        writer.WriteInt32(2, value.stage);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PrayTreeTimeCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->uin)) {
                    return false;
                }
                value->has_uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->stage)) {
                    return false;
                }
                value->has_stage = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SUMMONPETCH& value, ProtoWriter& writer) {
    if (value.has_monsterid) {
        writer.WriteInt32(1, value.monsterid);
    }
    if (value.has_serverid) {
        writer.WriteString(2, value.serverid);
    }
    if (value.has_petid) {
        writer.WriteInt32(3, value.petid);
    }
    if (value.has_stage) {
        writer.WriteInt32(4, value.stage);
    }
    if (value.has_quality) {
        writer.WriteInt32(5, value.quality);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SUMMONPETCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->monsterid)) {
                    return false;
                }
                value->has_monsterid = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->serverid)) {
                    return false;
                }
                value->has_serverid = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->petid)) {
                    return false;
                }
                value->has_petid = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->stage)) {
                    return false;
                }
                value->has_stage = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->quality)) {
                    return false;
                }
                value->has_quality = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_HomeLandRanchUpdateAnimalStateCH& value, ProtoWriter& writer) {
    if (value.has_objid) {
        writer.WriteUInt64(1, value.objid);
    }
    if (value.has_enterstate) {
        writer.WriteInt32(2, value.enterstate);
    }
    if (value.has_serverid) {
        writer.WriteString(3, value.serverid);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_HomeLandRanchUpdateAnimalStateCH* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->objid)) {
                    return false;
                }
                value->has_objid = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->enterstate)) {
                    return false;
                }
                value->has_enterstate = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->serverid)) {
                    return false;
                }
                value->has_serverid = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RequestModelName& value, ProtoWriter& writer) {
    if (value.has_modelID) {
        writer.WriteString(1, value.modelID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RequestModelName* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->modelID)) {
                    return false;
                }
                value->has_modelID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Vector3& value, ProtoWriter& writer) {
    if (value.has_X) {
        writer.WriteInt32(1, value.X);
    }
    if (value.has_Y) {
        writer.WriteInt32(2, value.Y);
    }
    if (value.has_Z) {
        writer.WriteInt32(3, value.Z);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Vector3* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->X)) {
                    return false;
                }
                value->has_X = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Y)) {
                    return false;
                }
                value->has_Y = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Z)) {
                    return false;
                }
                value->has_Z = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Quaternion& value, ProtoWriter& writer) {
    if (value.has_X) {
        writer.WriteFloat(1, value.X);
    }
    if (value.has_Y) {
        writer.WriteFloat(2, value.Y);
    }
    if (value.has_Z) {
        writer.WriteFloat(3, value.Z);
    }
    if (value.has_W) {
        writer.WriteFloat(4, value.W);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Quaternion* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->X)) {
                    return false;
                }
                value->has_X = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Y)) {
                    return false;
                }
                value->has_Y = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->Z)) {
                    return false;
                }
                value->has_Z = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->W)) {
                    return false;
                }
                value->has_W = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Pos& value, ProtoWriter& writer) {
    if (value.has_X) {
        writer.WriteInt32(1, value.X);
    }
    if (value.has_Y) {
        writer.WriteInt32(2, value.Y);
    }
    if (value.has_Z) {
        writer.WriteInt32(3, value.Z);
    }
    if (value.has_Map) {
        writer.WriteUInt32(4, value.Map);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Pos* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->X)) {
                    return false;
                }
                value->has_X = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Y)) {
                    return false;
                }
                value->has_Y = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Z)) {
                    return false;
                }
                value->has_Z = true;
                break;
            }
            case 4: {
                if (!reader.ReadUInt32(&value->Map)) {
                    return false;
                }
                value->has_Map = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BodyDir& value, ProtoWriter& writer) {
    if (value.has_RotationYaw) {
        writer.WriteFloat(1, value.RotationYaw);
    }
    if (value.has_RotationPitch) {
        writer.WriteFloat(2, value.RotationPitch);
    }
    if (value.has_Motion) {
        ProtoWriter inner;
        Serialize(value.Motion, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BodyDir* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->RotationYaw)) {
                    return false;
                }
                value->has_RotationYaw = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->RotationPitch)) {
                    return false;
                }
                value->has_RotationPitch = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Motion)) {
                    return false;
                }
                value->has_Motion = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Item& value, ProtoWriter& writer) {
    if (value.has_DefID) {
        writer.WriteUInt32(1, value.DefID);
    }
    if (value.has_Idx) {
        writer.WriteUInt32(2, value.Idx);
    }
    if (value.has_Pile) {
        writer.WriteUInt32(3, value.Pile);
    }
    if (value.has_UserDataStr) {
        writer.WriteString(4, value.UserDataStr);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Item* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->DefID)) {
                    return false;
                }
                value->has_DefID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->Idx)) {
                    return false;
                }
                value->has_Idx = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt32(&value->Pile)) {
                    return false;
                }
                value->has_Pile = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->UserDataStr)) {
                    return false;
                }
                value->has_UserDataStr = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ArmFumo& value, ProtoWriter& writer) {
    for (const auto& item : value.FumoIDs) {
        writer.WriteInt32(1, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ArmFumo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->FumoIDs.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Arm& value, ProtoWriter& writer) {
    if (value.has_DefID) {
        writer.WriteUInt32(1, value.DefID);
    }
    if (value.has_Idx) {
        writer.WriteUInt32(2, value.Idx);
    }
    if (value.has_Color) {
        writer.WriteUInt32(3, value.Color);
    }
    if (value.has_Dur) {
        writer.WriteUInt32(4, value.Dur);
    }
    if (value.has_RepairNum) {
        writer.WriteUInt32(5, value.RepairNum);
    }
    if (value.has_Name) {
        writer.WriteString(6, value.Name);
    }
    if (value.has_Fumo) {
        ProtoWriter inner;
        Serialize(value.Fumo, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Arm* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->DefID)) {
                    return false;
                }
                value->has_DefID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->Idx)) {
                    return false;
                }
                value->has_Idx = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt32(&value->Color)) {
                    return false;
                }
                value->has_Color = true;
                break;
            }
            case 4: {
                if (!reader.ReadUInt32(&value->Dur)) {
                    return false;
                }
                value->has_Dur = true;
                break;
            }
            case 5: {
                if (!reader.ReadUInt32(&value->RepairNum)) {
                    return false;
                }
                value->has_RepairNum = true;
                break;
            }
            case 6: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Fumo)) {
                    return false;
                }
                value->has_Fumo = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemSpecial& value, ProtoWriter& writer) {
    if (value.has_SpecailTypeID) {
        writer.WriteInt32(1, value.SpecailTypeID);
    }
    if (value.has_Buff) {
        writer.WriteString(2, value.Buff);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemSpecial* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpecailTypeID)) {
                    return false;
                }
                value->has_SpecailTypeID = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->Buff)) {
                    return false;
                }
                value->has_Buff = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemGridData& value, ProtoWriter& writer) {
    if (value.has_Item) {
        ProtoWriter inner;
        Serialize(value.Item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_Arm) {
        ProtoWriter inner;
        Serialize(value.Arm, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_ItemSpecial) {
        ProtoWriter inner;
        Serialize(value.ItemSpecial, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemGridData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Item)) {
                    return false;
                }
                value->has_Item = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Arm)) {
                    return false;
                }
                value->has_Arm = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ItemSpecial)) {
                    return false;
                }
                value->has_ItemSpecial = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemGrid& value, ProtoWriter& writer) {
    if (value.has_Dir) {
        ProtoWriter inner;
        Serialize(value.Dir, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_FallDistance) {
        writer.WriteFloat(2, value.FallDistance);
    }
    if (value.has_SpecialFlag) {
        writer.WriteInt32(3, value.SpecialFlag);
    }
    if (value.has_Type) {
        writer.WriteInt32(4, value.Type);
    }
    if (value.has_ItemGridData) {
        ProtoWriter inner;
        Serialize(value.ItemGridData, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemGrid* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Dir)) {
                    return false;
                }
                value->has_Dir = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->FallDistance)) {
                    return false;
                }
                value->has_FallDistance = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->SpecialFlag)) {
                    return false;
                }
                value->has_SpecialFlag = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ItemGridData)) {
                    return false;
                }
                value->has_ItemGridData = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoleBackpack& value, ProtoWriter& writer) {
    if (value.has_PackTmp) {
        writer.WriteInt32(1, value.PackTmp);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoleBackpack* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->PackTmp)) {
                    return false;
                }
                value->has_PackTmp = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerVipInfo& value, ProtoWriter& writer) {
    if (value.has_VipType) {
        writer.WriteInt32(1, value.VipType);
    }
    if (value.has_VipLevel) {
        writer.WriteInt32(2, value.VipLevel);
    }
    if (value.has_VipExp) {
        writer.WriteInt32(3, value.VipExp);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerVipInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->VipType)) {
                    return false;
                }
                value->has_VipType = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->VipLevel)) {
                    return false;
                }
                value->has_VipLevel = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->VipExp)) {
                    return false;
                }
                value->has_VipExp = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ChunkBlob& value, ProtoWriter& writer) {
    if (value.has_UnzipLen) {
        writer.WriteInt32(1, value.UnzipLen);
    }
    if (value.has_BlobLen) {
        writer.WriteInt32(2, value.BlobLen);
    }
    if (value.has_BlobDetail) {
        writer.WriteString(3, value.BlobDetail);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ChunkBlob* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->UnzipLen)) {
                    return false;
                }
                value->has_UnzipLen = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->BlobLen)) {
                    return false;
                }
                value->has_BlobLen = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->BlobDetail)) {
                    return false;
                }
                value->has_BlobDetail = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ChunkSaveDB& value, ProtoWriter& writer) {
    if (value.has_OWID) {
        writer.WriteUInt64(1, value.OWID);
    }
    if (value.has_MapID) {
        writer.WriteUInt32(2, value.MapID);
    }
    if (value.has_x) {
        writer.WriteInt32(3, value.x);
    }
    if (value.has_z) {
        writer.WriteInt32(4, value.z);
    }
    if (value.has_Version) {
        writer.WriteInt32(5, value.Version);
    }
    if (value.has_ShareFlag) {
        writer.WriteInt32(6, value.ShareFlag);
    }
    if (value.has_ChunkBlob) {
        ProtoWriter inner;
        Serialize(value.ChunkBlob, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ChunkSaveDB* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->OWID)) {
                    return false;
                }
                value->has_OWID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->x)) {
                    return false;
                }
                value->has_x = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->z)) {
                    return false;
                }
                value->has_z = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Version)) {
                    return false;
                }
                value->has_Version = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->ShareFlag)) {
                    return false;
                }
                value->has_ShareFlag = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ChunkBlob)) {
                    return false;
                }
                value->has_ChunkBlob = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GridPak& value, ProtoWriter& writer) {
    for (const auto& item : value.Grids) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GridPak* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemGrid item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Grids.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ShortcutPak& value, ProtoWriter& writer) {
    if (value.has_HandIdx) {
        writer.WriteInt32(1, value.HandIdx);
    }
    for (const auto& item : value.Grids) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ShortcutPak* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->HandIdx)) {
                    return false;
                }
                value->has_HandIdx = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemGrid item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Grids.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ArmPak& value, ProtoWriter& writer) {
    for (const auto& item : value.Grids) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ArmPak* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemGrid item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Grids.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorBuff& value, ProtoWriter& writer) {
    if (value.has_BuffID) {
        writer.WriteUInt32(1, value.BuffID);
    }
    if (value.has_BuffLV) {
        writer.WriteUInt32(2, value.BuffLV);
    }
    if (value.has_Ticks) {
        writer.WriteInt32(3, value.Ticks);
    }
    if (value.has_BuffInstanceId) {
        writer.WriteInt32(4, value.BuffInstanceId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorBuff* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->BuffID)) {
                    return false;
                }
                value->has_BuffID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->BuffLV)) {
                    return false;
                }
                value->has_BuffLV = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Ticks)) {
                    return false;
                }
                value->has_Ticks = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->BuffInstanceId)) {
                    return false;
                }
                value->has_BuffInstanceId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BuffAttr& value, ProtoWriter& writer) {
    if (value.has_AttrID) {
        writer.WriteUInt32(1, value.AttrID);
    }
    if (value.has_Val) {
        writer.WriteFloat(2, value.Val);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BuffAttr* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->AttrID)) {
                    return false;
                }
                value->has_AttrID = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Val)) {
                    return false;
                }
                value->has_Val = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RolePackage& value, ProtoWriter& writer) {
    if (value.has_GridPak) {
        ProtoWriter inner;
        Serialize(value.GridPak, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_ShortcutPak) {
        ProtoWriter inner;
        Serialize(value.ShortcutPak, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_ArmPak) {
        ProtoWriter inner;
        Serialize(value.ArmPak, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_RevicePos) {
        ProtoWriter inner;
        Serialize(value.RevicePos, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RolePackage* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->GridPak)) {
                    return false;
                }
                value->has_GridPak = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ShortcutPak)) {
                    return false;
                }
                value->has_ShortcutPak = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ArmPak)) {
                    return false;
                }
                value->has_ArmPak = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RevicePos)) {
                    return false;
                }
                value->has_RevicePos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorBuffList& value, ProtoWriter& writer) {
    for (const auto& item : value.Buffs) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    for (const auto& item : value.RelBuffAttrs) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    for (const auto& item : value.AbsBuffAttrs) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorBuffList* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ActorBuff item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Buffs.push_back(item);
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_BuffAttr item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->RelBuffAttrs.push_back(item);
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_BuffAttr item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->AbsBuffAttrs.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoleData& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_OWID) {
        writer.WriteUInt64(2, value.OWID);
    }
    if (value.has_HP) {
        writer.WriteInt32(3, value.HP);
    }
    if (value.has_Oxygen) {
        writer.WriteInt32(4, value.Oxygen);
    }
    if (value.has_FoodLevel) {
        writer.WriteInt32(5, value.FoodLevel);
    }
    if (value.has_FoodSatLevel) {
        writer.WriteInt32(6, value.FoodSatLevel);
    }
    if (value.has_UsedStamina) {
        writer.WriteInt32(7, value.UsedStamina);
    }
    if (value.has_Exp) {
        writer.WriteInt32(8, value.Exp);
    }
    if (value.has_Level) {
        writer.WriteInt32(9, value.Level);
    }
    if (value.has_LastLoginTime) {
        writer.WriteInt32(10, value.LastLoginTime);
    }
    if (value.has_LoginNum) {
        writer.WriteInt32(11, value.LoginNum);
    }
    if (value.has_FallDist) {
        writer.WriteFloat(12, value.FallDist);
    }
    if (value.has_Flags) {
        writer.WriteInt32(13, value.Flags);
    }
    if (value.has_LiveTicks) {
        writer.WriteInt32(14, value.LiveTicks);
    }
    if (value.has_RideActorID) {
        writer.WriteUInt64(15, value.RideActorID);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(16, inner.Bytes());
    }
    if (value.has_Dir) {
        ProtoWriter inner;
        Serialize(value.Dir, inner);
        writer.WriteMessage(17, inner.Bytes());
    }
    if (value.has_Package) {
        ProtoWriter inner;
        Serialize(value.Package, inner);
        writer.WriteMessage(18, inner.Bytes());
    }
    if (value.has_Buff) {
        ProtoWriter inner;
        Serialize(value.Buff, inner);
        writer.WriteMessage(19, inner.Bytes());
    }
    if (value.has_CarringActorID) {
        writer.WriteUInt64(20, value.CarringActorID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoleData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->OWID)) {
                    return false;
                }
                value->has_OWID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->HP)) {
                    return false;
                }
                value->has_HP = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Oxygen)) {
                    return false;
                }
                value->has_Oxygen = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->FoodLevel)) {
                    return false;
                }
                value->has_FoodLevel = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->FoodSatLevel)) {
                    return false;
                }
                value->has_FoodSatLevel = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->UsedStamina)) {
                    return false;
                }
                value->has_UsedStamina = true;
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->Exp)) {
                    return false;
                }
                value->has_Exp = true;
                break;
            }
            case 9: {
                if (!reader.ReadInt32(&value->Level)) {
                    return false;
                }
                value->has_Level = true;
                break;
            }
            case 10: {
                if (!reader.ReadInt32(&value->LastLoginTime)) {
                    return false;
                }
                value->has_LastLoginTime = true;
                break;
            }
            case 11: {
                if (!reader.ReadInt32(&value->LoginNum)) {
                    return false;
                }
                value->has_LoginNum = true;
                break;
            }
            case 12: {
                if (!reader.ReadFloat(&value->FallDist)) {
                    return false;
                }
                value->has_FallDist = true;
                break;
            }
            case 13: {
                if (!reader.ReadInt32(&value->Flags)) {
                    return false;
                }
                value->has_Flags = true;
                break;
            }
            case 14: {
                if (!reader.ReadInt32(&value->LiveTicks)) {
                    return false;
                }
                value->has_LiveTicks = true;
                break;
            }
            case 15: {
                if (!reader.ReadUInt64(&value->RideActorID)) {
                    return false;
                }
                value->has_RideActorID = true;
                break;
            }
            case 16: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            case 17: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Dir)) {
                    return false;
                }
                value->has_Dir = true;
                break;
            }
            case 18: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Package)) {
                    return false;
                }
                value->has_Package = true;
                break;
            }
            case 19: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Buff)) {
                    return false;
                }
                value->has_Buff = true;
                break;
            }
            case 20: {
                if (!reader.ReadUInt64(&value->CarringActorID)) {
                    return false;
                }
                value->has_CarringActorID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BodyEffectBrief& value, ProtoWriter& writer) {
    if (value.has_effectID) {
        writer.WriteInt32(1, value.effectID);
    }
    if (value.has_effectScale) {
        writer.WriteFloat(2, value.effectScale);
    }
    if (value.has_effectClass) {
        writer.WriteInt32(3, value.effectClass);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BodyEffectBrief* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->effectID)) {
                    return false;
                }
                value->has_effectID = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->effectScale)) {
                    return false;
                }
                value->has_effectScale = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->effectClass)) {
                    return false;
                }
                value->has_effectClass = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerInfo& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_anim) {
        writer.WriteInt32(2, value.anim);
    }
    if (value.has_anim1) {
        writer.WriteInt32(3, value.anim1);
    }
    if (value.has_RoleData) {
        ProtoWriter inner;
        Serialize(value.RoleData, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    if (value.has_BodyColor) {
        writer.WriteUInt32(5, value.BodyColor);
    }
    if (value.has_bodyscale_invalid) {
        writer.WriteFloat(6, value.bodyscale_invalid);
    }
    for (const auto& item : value.effectList) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
    if (value.has_customscale) {
        writer.WriteFloat(8, value.customscale);
    }
    for (const auto& item : value.soundList) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(9, inner.Bytes());
    }
    if (value.has_actSeqId) {
        writer.WriteInt32(10, value.actSeqId);
    }
    if (value.has_accountSkinID) {
        writer.WriteInt32(11, value.accountSkinID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->anim)) {
                    return false;
                }
                value->has_anim = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->anim1)) {
                    return false;
                }
                value->has_anim1 = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RoleData)) {
                    return false;
                }
                value->has_RoleData = true;
                break;
            }
            case 5: {
                if (!reader.ReadUInt32(&value->BodyColor)) {
                    return false;
                }
                value->has_BodyColor = true;
                break;
            }
            case 6: {
                if (!reader.ReadFloat(&value->bodyscale_invalid)) {
                    return false;
                }
                value->has_bodyscale_invalid = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_BodyEffectBrief item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->effectList.push_back(item);
                break;
            }
            case 8: {
                if (!reader.ReadFloat(&value->customscale)) {
                    return false;
                }
                value->has_customscale = true;
                break;
            }
            case 9: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_EffectTriggerSound item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->soundList.push_back(item);
                break;
            }
            case 10: {
                if (!reader.ReadInt32(&value->actSeqId)) {
                    return false;
                }
                value->has_actSeqId = true;
                break;
            }
            case 11: {
                if (!reader.ReadInt32(&value->accountSkinID)) {
                    return false;
                }
                value->has_accountSkinID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RedStoneIdx& value, ProtoWriter& writer) {
    if (value.has_x) {
        writer.WriteInt32(1, value.x);
    }
    if (value.has_z) {
        writer.WriteInt32(2, value.z);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RedStoneIdx* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->x)) {
                    return false;
                }
                value->has_x = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->z)) {
                    return false;
                }
                value->has_z = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RedStoneData& value, ProtoWriter& writer) {
    for (const auto& item : value.RedStoneIdxList) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RedStoneData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_RedStoneIdx item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->RedStoneIdxList.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GlobalBin& value, ProtoWriter& writer) {
    if (value.has_BinLen) {
        writer.WriteInt32(1, value.BinLen);
    }
    if (value.has_BinContent) {
        writer.WriteString(2, value.BinContent);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GlobalBin* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BinLen)) {
                    return false;
                }
                value->has_BinLen = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->BinContent)) {
                    return false;
                }
                value->has_BinContent = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OWGlobalMisc& value, ProtoWriter& writer) {
    if (value.has_GlobalFlag) {
        writer.WriteInt32(1, value.GlobalFlag);
    }
    if (value.has_ChunkVer) {
        writer.WriteInt32(2, value.ChunkVer);
    }
    if (value.has_ChunkVerBroadCast) {
        writer.WriteInt32(3, value.ChunkVerBroadCast);
    }
    if (value.has_InitPos) {
        ProtoWriter inner;
        Serialize(value.InitPos, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    if (value.has_RevicePos) {
        ProtoWriter inner;
        Serialize(value.RevicePos, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
    if (value.has_RedStoneData) {
        ProtoWriter inner;
        Serialize(value.RedStoneData, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
    if (value.has_GlobalBin) {
        ProtoWriter inner;
        Serialize(value.GlobalBin, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OWGlobalMisc* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GlobalFlag)) {
                    return false;
                }
                value->has_GlobalFlag = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ChunkVer)) {
                    return false;
                }
                value->has_ChunkVer = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ChunkVerBroadCast)) {
                    return false;
                }
                value->has_ChunkVerBroadCast = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->InitPos)) {
                    return false;
                }
                value->has_InitPos = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RevicePos)) {
                    return false;
                }
                value->has_RevicePos = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RedStoneData)) {
                    return false;
                }
                value->has_RedStoneData = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->GlobalBin)) {
                    return false;
                }
                value->has_GlobalBin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OWGlobal& value, ProtoWriter& writer) {
    if (value.has_OWID) {
        writer.WriteUInt64(1, value.OWID);
    }
    if (value.has_ID) {
        writer.WriteInt32(2, value.ID);
    }
    if (value.has_Uin) {
        writer.WriteInt32(3, value.Uin);
    }
    if (value.has_SvrStart) {
        writer.WriteInt32(4, value.SvrStart);
    }
    if (value.has_GridChgNum) {
        writer.WriteInt32(5, value.GridChgNum);
    }
    if (value.has_Misc) {
        ProtoWriter inner;
        Serialize(value.Misc, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OWGlobal* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->OWID)) {
                    return false;
                }
                value->has_OWID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->SvrStart)) {
                    return false;
                }
                value->has_SvrStart = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->GridChgNum)) {
                    return false;
                }
                value->has_GridChgNum = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Misc)) {
                    return false;
                }
                value->has_Misc = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_WorldCreateData& value, ProtoWriter& writer) {
    if (value.has_TerrType) {
        writer.WriteInt32(1, value.TerrType);
    }
    if (value.has_RandSeed1) {
        writer.WriteUInt32(2, value.RandSeed1);
    }
    if (value.has_RandSeed2) {
        writer.WriteUInt32(3, value.RandSeed2);
    }
    if (value.has_RoleModel) {
        writer.WriteInt32(4, value.RoleModel);
    }
    if (value.has_SeedStr) {
        writer.WriteString(5, value.SeedStr);
    }
    if (value.has_TilesX) {
        writer.WriteUInt32(6, value.TilesX);
    }
    if (value.has_TilesZ) {
        writer.WriteUInt32(7, value.TilesZ);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_WorldCreateData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->TerrType)) {
                    return false;
                }
                value->has_TerrType = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->RandSeed1)) {
                    return false;
                }
                value->has_RandSeed1 = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt32(&value->RandSeed2)) {
                    return false;
                }
                value->has_RandSeed2 = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->RoleModel)) {
                    return false;
                }
                value->has_RoleModel = true;
                break;
            }
            case 5: {
                if (!reader.ReadString(&value->SeedStr)) {
                    return false;
                }
                value->has_SeedStr = true;
                break;
            }
            case 6: {
                if (!reader.ReadUInt32(&value->TilesX)) {
                    return false;
                }
                value->has_TilesX = true;
                break;
            }
            case 7: {
                if (!reader.ReadUInt32(&value->TilesZ)) {
                    return false;
                }
                value->has_TilesZ = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_WorldDesc& value, ProtoWriter& writer) {
    if (value.has_WorldId) {
        writer.WriteUInt64(1, value.WorldId);
    }
    if (value.has_WorldType) {
        writer.WriteInt32(2, value.WorldType);
    }
    if (value.has_OwnerUin) {
        writer.WriteInt32(3, value.OwnerUin);
    }
    if (value.has_CreateData) {
        ProtoWriter inner;
        Serialize(value.CreateData, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    if (value.has_DeveloperFlag) {
        writer.WriteInt32(5, value.DeveloperFlag);
    }
    if (value.has_FromOWID) {
        writer.WriteUInt64(6, value.FromOWID);
    }
    if (value.has_RealOwnerUin) {
        writer.WriteInt32(7, value.RealOwnerUin);
    }
    if (value.has_WorldOpen) {
        writer.WriteInt32(8, value.WorldOpen);
    }
    if (value.has_WorldName) {
        writer.WriteString(9, value.WorldName);
    }
    if (value.has_TempType) {
        writer.WriteInt32(10, value.TempType);
    }
    if (value.has_pwid) {
        writer.WriteInt64(11, value.pwid);
    }
    if (value.has_SpecialType) {
        writer.WriteUInt32(12, value.SpecialType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_WorldDesc* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->WorldId)) {
                    return false;
                }
                value->has_WorldId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->WorldType)) {
                    return false;
                }
                value->has_WorldType = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->OwnerUin)) {
                    return false;
                }
                value->has_OwnerUin = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->CreateData)) {
                    return false;
                }
                value->has_CreateData = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->DeveloperFlag)) {
                    return false;
                }
                value->has_DeveloperFlag = true;
                break;
            }
            case 6: {
                if (!reader.ReadUInt64(&value->FromOWID)) {
                    return false;
                }
                value->has_FromOWID = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->RealOwnerUin)) {
                    return false;
                }
                value->has_RealOwnerUin = true;
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->WorldOpen)) {
                    return false;
                }
                value->has_WorldOpen = true;
                break;
            }
            case 9: {
                if (!reader.ReadString(&value->WorldName)) {
                    return false;
                }
                value->has_WorldName = true;
                break;
            }
            case 10: {
                if (!reader.ReadInt32(&value->TempType)) {
                    return false;
                }
                value->has_TempType = true;
                break;
            }
            case 11: {
                if (!reader.ReadInt64(&value->pwid)) {
                    return false;
                }
                value->has_pwid = true;
                break;
            }
            case 12: {
                if (!reader.ReadUInt32(&value->SpecialType)) {
                    return false;
                }
                value->has_SpecialType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SkillCDData& value, ProtoWriter& writer) {
    if (value.has_NumSkillCD) {
        writer.WriteInt32(1, value.NumSkillCD);
    }
    for (const auto& item : value.ItemID) {
        writer.WriteInt32(2, item);
    }
    for (const auto& item : value.CD) {
        writer.WriteFloat(3, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SkillCDData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->NumSkillCD)) {
                    return false;
                }
                value->has_NumSkillCD = true;
                break;
            }
            case 2: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->ItemID.push_back(item);
                break;
            }
            case 3: {
                float item{};
                if (!reader.ReadFloat(&item)) {
                    return false;
                }
                value->CD.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoleInfo& value, ProtoWriter& writer) {
    if (value.has_Model) {
        writer.WriteInt32(1, value.Model);
    }
    if (value.has_NickName) {
        writer.WriteString(2, value.NickName);
    }
    if (value.has_SkinID) {
        writer.WriteInt32(3, value.SkinID);
    }
    if (value.has_CustomJson) {
        writer.WriteString(4, value.CustomJson);
    }
    if (value.has_FrameID) {
        writer.WriteInt32(5, value.FrameID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoleInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Model)) {
                    return false;
                }
                value->has_Model = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->NickName)) {
                    return false;
                }
                value->has_NickName = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->SkinID)) {
                    return false;
                }
                value->has_SkinID = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->CustomJson)) {
                    return false;
                }
                value->has_CustomJson = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->FrameID)) {
                    return false;
                }
                value->has_FrameID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorRoleInfo& value, ProtoWriter& writer) {
    if (value.has_Info) {
        ProtoWriter inner;
        Serialize(value.Info, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_Player) {
        ProtoWriter inner;
        Serialize(value.Player, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorRoleInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Info)) {
                    return false;
                }
                value->has_Info = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Player)) {
                    return false;
                }
                value->has_Player = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorGeneral& value, ProtoWriter& writer) {
    if (value.has_MapId) {
        writer.WriteInt32(1, value.MapId);
    }
    if (value.has_InfoLen) {
        writer.WriteInt32(2, value.InfoLen);
    }
    if (value.has_ActorDetail) {
        writer.WriteString(3, value.ActorDetail);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorGeneral* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->MapId)) {
                    return false;
                }
                value->has_MapId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->InfoLen)) {
                    return false;
                }
                value->has_InfoLen = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->ActorDetail)) {
                    return false;
                }
                value->has_ActorDetail = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorInfo& value, ProtoWriter& writer) {
    if (value.has_ActorGeneral) {
        ProtoWriter inner;
        Serialize(value.ActorGeneral, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_RoleInfo) {
        ProtoWriter inner;
        Serialize(value.RoleInfo, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ActorGeneral)) {
                    return false;
                }
                value->has_ActorGeneral = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RoleInfo)) {
                    return false;
                }
                value->has_RoleInfo = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MoveMotion& value, ProtoWriter& writer) {
    if (value.has_Position) {
        ProtoWriter inner;
        Serialize(value.Position, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_Yaw) {
        writer.WriteUInt32(2, value.Yaw);
    }
    if (value.has_Pitch) {
        writer.WriteUInt32(3, value.Pitch);
    }
    if (value.has_MapID) {
        writer.WriteUInt32(4, value.MapID);
    }
    if (value.has_ChangeFlags) {
        writer.WriteInt32(5, value.ChangeFlags);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MoveMotion* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Position)) {
                    return false;
                }
                value->has_Position = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->Yaw)) {
                    return false;
                }
                value->has_Yaw = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt32(&value->Pitch)) {
                    return false;
                }
                value->has_Pitch = true;
                break;
            }
            case 4: {
                if (!reader.ReadUInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->ChangeFlags)) {
                    return false;
                }
                value->has_ChangeFlags = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemData& value, ProtoWriter& writer) {
    if (value.has_Index) {
        writer.WriteInt32(1, value.Index);
    }
    if (value.has_ItemID) {
        writer.WriteInt32(2, value.ItemID);
    }
    if (value.has_Durable) {
        writer.WriteInt32(3, value.Durable);
    }
    if (value.has_Num) {
        writer.WriteUInt32(4, value.Num);
    }
    if (value.has_UserData) {
        writer.WriteInt32(5, value.UserData);
    }
    for (const auto& item : value.Enchs) {
        writer.WriteInt32(6, item);
    }
    if (value.has_UserDataStr) {
        writer.WriteString(7, value.UserDataStr);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Index)) {
                    return false;
                }
                value->has_Index = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ItemID)) {
                    return false;
                }
                value->has_ItemID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Durable)) {
                    return false;
                }
                value->has_Durable = true;
                break;
            }
            case 4: {
                if (!reader.ReadUInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->UserData)) {
                    return false;
                }
                value->has_UserData = true;
                break;
            }
            case 6: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->Enchs.push_back(item);
                break;
            }
            case 7: {
                if (!reader.ReadString(&value->UserDataStr)) {
                    return false;
                }
                value->has_UserDataStr = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerBriefInfo& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    if (value.has_HP) {
        writer.WriteInt32(3, value.HP);
    }
    if (value.has_NickName) {
        writer.WriteString(4, value.NickName);
    }
    if (value.has_PlayerIndex) {
        writer.WriteInt32(5, value.PlayerIndex);
    }
    if (value.has_TeamID) {
        writer.WriteInt32(6, value.TeamID);
    }
    for (const auto& item : value.CGVars) {
        writer.WriteInt32(7, item);
    }
    if (value.has_InSpectator) {
        writer.WriteInt32(8, value.InSpectator);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(9, inner.Bytes());
    }
    if (value.has_VipInfo) {
        ProtoWriter inner;
        Serialize(value.VipInfo, inner);
        writer.WriteMessage(10, inner.Bytes());
    }
    if (value.has_CustomJson) {
        writer.WriteString(11, value.CustomJson);
    }
    if (value.has_SkinID) {
        writer.WriteInt32(12, value.SkinID);
    }
    if (value.has_FrameID) {
        writer.WriteInt32(13, value.FrameID);
    }
    if (value.has_CustomModel) {
        writer.WriteString(14, value.CustomModel);
    }
    if (value.has_AcctountSkinID) {
        writer.WriteInt32(15, value.AcctountSkinID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerBriefInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->HP)) {
                    return false;
                }
                value->has_HP = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->NickName)) {
                    return false;
                }
                value->has_NickName = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->PlayerIndex)) {
                    return false;
                }
                value->has_PlayerIndex = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->TeamID)) {
                    return false;
                }
                value->has_TeamID = true;
                break;
            }
            case 7: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->CGVars.push_back(item);
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->InSpectator)) {
                    return false;
                }
                value->has_InSpectator = true;
                break;
            }
            case 9: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            case 10: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->VipInfo)) {
                    return false;
                }
                value->has_VipInfo = true;
                break;
            }
            case 11: {
                if (!reader.ReadString(&value->CustomJson)) {
                    return false;
                }
                value->has_CustomJson = true;
                break;
            }
            case 12: {
                if (!reader.ReadInt32(&value->SkinID)) {
                    return false;
                }
                value->has_SkinID = true;
                break;
            }
            case 13: {
                if (!reader.ReadInt32(&value->FrameID)) {
                    return false;
                }
                value->has_FrameID = true;
                break;
            }
            case 14: {
                if (!reader.ReadString(&value->CustomModel)) {
                    return false;
                }
                value->has_CustomModel = true;
                break;
            }
            case 15: {
                if (!reader.ReadInt32(&value->AcctountSkinID)) {
                    return false;
                }
                value->has_AcctountSkinID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectParticle& value, ProtoWriter& writer) {
    if (value.has_Name) {
        writer.WriteString(1, value.Name);
    }
    if (value.has_Age) {
        writer.WriteInt32(2, value.Age);
    }
    if (value.has_Color) {
        writer.WriteInt32(3, value.Color);
    }
    if (value.has_Yaw) {
        writer.WriteFloat(4, value.Yaw);
    }
    if (value.has_Pitch) {
        writer.WriteFloat(5, value.Pitch);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectParticle* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Age)) {
                    return false;
                }
                value->has_Age = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Color)) {
                    return false;
                }
                value->has_Color = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->Yaw)) {
                    return false;
                }
                value->has_Yaw = true;
                break;
            }
            case 5: {
                if (!reader.ReadFloat(&value->Pitch)) {
                    return false;
                }
                value->has_Pitch = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectPickItem& value, ProtoWriter& writer) {
    if (value.has_PickerObj) {
        writer.WriteUInt64(1, value.PickerObj);
    }
    if (value.has_ItemObj) {
        writer.WriteUInt64(2, value.ItemObj);
    }
    if (value.has_YOffset) {
        writer.WriteInt32(3, value.YOffset);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectPickItem* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->PickerObj)) {
                    return false;
                }
                value->has_PickerObj = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ItemObj)) {
                    return false;
                }
                value->has_ItemObj = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->YOffset)) {
                    return false;
                }
                value->has_YOffset = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectSound& value, ProtoWriter& writer) {
    if (value.has_Name) {
        writer.WriteString(1, value.Name);
    }
    if (value.has_Volume) {
        writer.WriteFloat(2, value.Volume);
    }
    if (value.has_Pitch) {
        writer.WriteFloat(3, value.Pitch);
    }
    if (value.has_Flags) {
        writer.WriteInt32(4, value.Flags);
    }
    if (value.has_Segment) {
        writer.WriteInt32(5, value.Segment);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectSound* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Volume)) {
                    return false;
                }
                value->has_Volume = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->Pitch)) {
                    return false;
                }
                value->has_Pitch = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Flags)) {
                    return false;
                }
                value->has_Flags = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Segment)) {
                    return false;
                }
                value->has_Segment = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectTriggerSound& value, ProtoWriter& writer) {
    if (value.has_Name) {
        writer.WriteString(1, value.Name);
    }
    if (value.has_Volume) {
        writer.WriteFloat(2, value.Volume);
    }
    if (value.has_Pitch) {
        writer.WriteFloat(3, value.Pitch);
    }
    if (value.has_IsLoop) {
        writer.WriteBool(4, value.IsLoop);
    }
    if (value.has_PlayState) {
        writer.WriteInt32(5, value.PlayState);
    }
    if (value.has_ObjId) {
        writer.WriteUInt64(6, value.ObjId);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
    if (value.has_position) {
        writer.WriteInt32(8, value.position);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectTriggerSound* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Volume)) {
                    return false;
                }
                value->has_Volume = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->Pitch)) {
                    return false;
                }
                value->has_Pitch = true;
                break;
            }
            case 4: {
                if (!reader.ReadBool(&value->IsLoop)) {
                    return false;
                }
                value->has_IsLoop = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->PlayState)) {
                    return false;
                }
                value->has_PlayState = true;
                break;
            }
            case 6: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->position)) {
                    return false;
                }
                value->has_position = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectActorBody& value, ProtoWriter& writer) {
    if (value.has_ObjId) {
        writer.WriteUInt64(1, value.ObjId);
    }
    if (value.has_BodyEffect) {
        writer.WriteInt32(2, value.BodyEffect);
    }
    if (value.has_Status) {
        writer.WriteInt32(3, value.Status);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectActorBody* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->BodyEffect)) {
                    return false;
                }
                value->has_BodyEffect = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Status)) {
                    return false;
                }
                value->has_Status = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectStringActorBody& value, ProtoWriter& writer) {
    if (value.has_ObjId) {
        writer.WriteUInt64(1, value.ObjId);
    }
    if (value.has_EffectName) {
        writer.WriteString(2, value.EffectName);
    }
    if (value.has_Status) {
        writer.WriteInt32(3, value.Status);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectStringActorBody* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->EffectName)) {
                    return false;
                }
                value->has_EffectName = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Status)) {
                    return false;
                }
                value->has_Status = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectDestroyBlock& value, ProtoWriter& writer) {
    if (value.has_Face) {
        writer.WriteInt32(1, value.Face);
    }
    if (value.has_SubType) {
        writer.WriteInt32(2, value.SubType);
    }
    if (value.has_Age) {
        writer.WriteInt32(3, value.Age);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    if (value.has_ID) {
        writer.WriteInt32(5, value.ID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectDestroyBlock* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Face)) {
                    return false;
                }
                value->has_Face = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SubType)) {
                    return false;
                }
                value->has_SubType = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Age)) {
                    return false;
                }
                value->has_Age = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectCrackBlock& value, ProtoWriter& writer) {
    if (value.has_Stage) {
        writer.WriteInt32(1, value.Stage);
    }
    if (value.has_ActorId) {
        writer.WriteUInt64(2, value.ActorId);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectCrackBlock* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Stage)) {
                    return false;
                }
                value->has_Stage = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorId)) {
                    return false;
                }
                value->has_ActorId = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectVehicle& value, ProtoWriter& writer) {
    if (value.has_Name) {
        writer.WriteString(1, value.Name);
    }
    if (value.has_ActorId) {
        writer.WriteUInt64(2, value.ActorId);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_Age) {
        writer.WriteInt32(4, value.Age);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectVehicle* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorId)) {
                    return false;
                }
                value->has_ActorId = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Age)) {
                    return false;
                }
                value->has_Age = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectPlayMusicGrid& value, ProtoWriter& writer) {
    if (value.has_Name) {
        writer.WriteString(1, value.Name);
    }
    if (value.has_Pitch) {
        writer.WriteFloat(2, value.Pitch);
    }
    if (value.has_Flags) {
        writer.WriteInt32(3, value.Flags);
    }
    if (value.has_Segment) {
        writer.WriteInt32(4, value.Segment);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectPlayMusicGrid* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Pitch)) {
                    return false;
                }
                value->has_Pitch = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Flags)) {
                    return false;
                }
                value->has_Flags = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Segment)) {
                    return false;
                }
                value->has_Segment = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectStopMusicGrid& value, ProtoWriter& writer) {
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectStopMusicGrid* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_IntertactData& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ID) {
        writer.WriteInt32(2, value.ID);
    }
    if (value.has_Show) {
        writer.WriteInt32(3, value.Show);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_IntertactData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Show)) {
                    return false;
                }
                value->has_Show = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TaskContentData& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ID) {
        writer.WriteInt32(2, value.ID);
    }
    if (value.has_Num) {
        writer.WriteInt32(3, value.Num);
    }
    if (value.has_CompletedNum) {
        writer.WriteInt32(4, value.CompletedNum);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TaskContentData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->CompletedNum)) {
                    return false;
                }
                value->has_CompletedNum = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TaskInfoData& value, ProtoWriter& writer) {
    if (value.has_ID) {
        writer.WriteInt32(1, value.ID);
    }
    if (value.has_State) {
        writer.WriteInt32(2, value.State);
    }
    if (value.has_PlotID) {
        writer.WriteInt32(3, value.PlotID);
    }
    for (const auto& item : value.Contents) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TaskInfoData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->State)) {
                    return false;
                }
                value->has_State = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->PlotID)) {
                    return false;
                }
                value->has_PlotID = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_TaskContentData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Contents.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PreBlockData& value, ProtoWriter& writer) {
    if (value.has_ID) {
        writer.WriteInt32(1, value.ID);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PreBlockData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CustomModelClassData& value, ProtoWriter& writer) {
    if (value.has_ClassName) {
        writer.WriteString(1, value.ClassName);
    }
    for (const auto& item : value.ModelNames) {
        writer.WriteString(2, item);
    }
    if (value.has_folderindex) {
        writer.WriteInt32(3, value.folderindex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CustomModelClassData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->ClassName)) {
                    return false;
                }
                value->has_ClassName = true;
                break;
            }
            case 2: {
                std::string item{};
                if (!reader.ReadString(&item)) {
                    return false;
                }
                value->ModelNames.push_back(item);
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->folderindex)) {
                    return false;
                }
                value->has_folderindex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MobDisplayData& value, ProtoWriter& writer) {
    if (value.has_mobId) {
        writer.WriteInt32(1, value.mobId);
    }
    if (value.has_itemId) {
        writer.WriteInt32(2, value.itemId);
    }
    if (value.has_itemCount) {
        writer.WriteInt32(3, value.itemCount);
    }
    if (value.has_animId) {
        writer.WriteInt32(4, value.animId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MobDisplayData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->mobId)) {
                    return false;
                }
                value->has_mobId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->itemId)) {
                    return false;
                }
                value->has_itemId = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->itemCount)) {
                    return false;
                }
                value->has_itemCount = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->animId)) {
                    return false;
                }
                value->has_animId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_NpcShopItemData& value, ProtoWriter& writer) {
    if (value.has_SkuID) {
        writer.WriteInt32(1, value.SkuID);
    }
    if (value.has_ItemID) {
        writer.WriteInt32(2, value.ItemID);
    }
    if (value.has_OnceBuyNum) {
        writer.WriteInt32(3, value.OnceBuyNum);
    }
    if (value.has_MaxCanBuyCount) {
        writer.WriteInt32(4, value.MaxCanBuyCount);
    }
    if (value.has_RefreshDuration) {
        writer.WriteInt32(5, value.RefreshDuration);
    }
    if (value.has_StarNum) {
        writer.WriteInt32(6, value.StarNum);
    }
    if (value.has_CostItemInfo1) {
        writer.WriteInt32(7, value.CostItemInfo1);
    }
    if (value.has_CostItemInfo2) {
        writer.WriteInt32(8, value.CostItemInfo2);
    }
    if (value.has_LeftCount) {
        writer.WriteInt32(9, value.LeftCount);
    }
    if (value.has_EndTime) {
        writer.WriteInt32(10, value.EndTime);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_NpcShopItemData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SkuID)) {
                    return false;
                }
                value->has_SkuID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ItemID)) {
                    return false;
                }
                value->has_ItemID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->OnceBuyNum)) {
                    return false;
                }
                value->has_OnceBuyNum = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->MaxCanBuyCount)) {
                    return false;
                }
                value->has_MaxCanBuyCount = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->RefreshDuration)) {
                    return false;
                }
                value->has_RefreshDuration = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->StarNum)) {
                    return false;
                }
                value->has_StarNum = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->CostItemInfo1)) {
                    return false;
                }
                value->has_CostItemInfo1 = true;
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->CostItemInfo2)) {
                    return false;
                }
                value->has_CostItemInfo2 = true;
                break;
            }
            case 9: {
                if (!reader.ReadInt32(&value->LeftCount)) {
                    return false;
                }
                value->has_LeftCount = true;
                break;
            }
            case 10: {
                if (!reader.ReadInt32(&value->EndTime)) {
                    return false;
                }
                value->has_EndTime = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_NpcShopData& value, ProtoWriter& writer) {
    if (value.has_ShopID) {
        writer.WriteInt32(1, value.ShopID);
    }
    if (value.has_ShopName) {
        writer.WriteString(2, value.ShopName);
    }
    if (value.has_ShopDesc) {
        writer.WriteString(3, value.ShopDesc);
    }
    if (value.has_InnerKey) {
        writer.WriteString(4, value.InnerKey);
    }
    for (const auto& item : value.ShopItemData) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_NpcShopData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ShopID)) {
                    return false;
                }
                value->has_ShopID = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->ShopName)) {
                    return false;
                }
                value->has_ShopName = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->ShopDesc)) {
                    return false;
                }
                value->has_ShopDesc = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->InnerKey)) {
                    return false;
                }
                value->has_InnerKey = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_NpcShopItemData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->ShopItemData.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorOneAvatarModelData& value, ProtoWriter& writer) {
    if (value.has_ModelFilename) {
        writer.WriteString(1, value.ModelFilename);
    }
    if (value.has_Scale) {
        writer.WriteFloat(2, value.Scale);
    }
    if (value.has_Yaw) {
        writer.WriteInt32(3, value.Yaw);
    }
    if (value.has_Pitch) {
        writer.WriteInt32(4, value.Pitch);
    }
    if (value.has_OffsetPos) {
        ProtoWriter inner;
        Serialize(value.OffsetPos, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
    if (value.has_Roll) {
        writer.WriteInt32(6, value.Roll);
    }
    if (value.has_NewRotateMode) {
        writer.WriteBool(7, value.NewRotateMode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorOneAvatarModelData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->ModelFilename)) {
                    return false;
                }
                value->has_ModelFilename = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Scale)) {
                    return false;
                }
                value->has_Scale = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Yaw)) {
                    return false;
                }
                value->has_Yaw = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Pitch)) {
                    return false;
                }
                value->has_Pitch = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->OffsetPos)) {
                    return false;
                }
                value->has_OffsetPos = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->Roll)) {
                    return false;
                }
                value->has_Roll = true;
                break;
            }
            case 7: {
                if (!reader.ReadBool(&value->NewRotateMode)) {
                    return false;
                }
                value->has_NewRotateMode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorOneBoneModelData& value, ProtoWriter& writer) {
    if (value.has_BoneName) {
        writer.WriteString(1, value.BoneName);
    }
    for (const auto& item : value.AvatarModels) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorOneBoneModelData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->BoneName)) {
                    return false;
                }
                value->has_BoneName = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ActorOneAvatarModelData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->AvatarModels.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BasketBallOperate& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ActorID) {
        writer.WriteUInt64(2, value.ActorID);
    }
    if (value.has_IsSelectedTarget) {
        writer.WriteBool(3, value.IsSelectedTarget);
    }
    if (value.has_FallResult) {
        writer.WriteInt32(4, value.FallResult);
    }
    if (value.has_ExtendData) {
        writer.WriteInt32(5, value.ExtendData);
    }
    if (value.has_Uin) {
        writer.WriteInt32(6, value.Uin);
    }
    if (value.has_pos) {
        ProtoWriter inner;
        Serialize(value.pos, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
    if (value.has_yaw) {
        writer.WriteFloat(8, value.yaw);
    }
    if (value.has_SelectedActorID) {
        writer.WriteInt32(9, value.SelectedActorID);
    }
    if (value.has_pitch) {
        writer.WriteFloat(10, value.pitch);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BasketBallOperate* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->IsSelectedTarget)) {
                    return false;
                }
                value->has_IsSelectedTarget = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->FallResult)) {
                    return false;
                }
                value->has_FallResult = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->ExtendData)) {
                    return false;
                }
                value->has_ExtendData = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->pos)) {
                    return false;
                }
                value->has_pos = true;
                break;
            }
            case 8: {
                if (!reader.ReadFloat(&value->yaw)) {
                    return false;
                }
                value->has_yaw = true;
                break;
            }
            case 9: {
                if (!reader.ReadInt32(&value->SelectedActorID)) {
                    return false;
                }
                value->has_SelectedActorID = true;
                break;
            }
            case 10: {
                if (!reader.ReadFloat(&value->pitch)) {
                    return false;
                }
                value->has_pitch = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CSAuthorityData& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_Type) {
        writer.WriteInt32(2, value.Type);
    }
    if (value.has_Flag) {
        writer.WriteUInt32(3, value.Flag);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CSAuthorityData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt32(&value->Flag)) {
                    return false;
                }
                value->has_Flag = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CSPermitData& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_Type) {
        writer.WriteInt32(2, value.Type);
    }
    if (value.has_Flag) {
        writer.WriteUInt32(3, value.Flag);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CSPermitData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt32(&value->Flag)) {
                    return false;
                }
                value->has_Flag = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Edu_RoleInfo& value, ProtoWriter& writer) {
    if (value.has_objId) {
        writer.WriteUInt32(1, value.objId);
    }
    if (value.has_yaw) {
        writer.WriteFloat(2, value.yaw);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Edu_RoleInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->objId)) {
                    return false;
                }
                value->has_objId = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->yaw)) {
                    return false;
                }
                value->has_yaw = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ImportModelData& value, ProtoWriter& writer) {
    if (value.has_Key) {
        writer.WriteString(1, value.Key);
    }
    if (value.has_Name) {
        writer.WriteString(2, value.Name);
    }
    if (value.has_Desc) {
        writer.WriteString(3, value.Desc);
    }
    if (value.has_Type) {
        writer.WriteInt32(4, value.Type);
    }
    if (value.has_AuthUin) {
        writer.WriteInt32(5, value.AuthUin);
    }
    if (value.has_AuthName) {
        writer.WriteString(6, value.AuthName);
    }
    if (value.has_FileName) {
        writer.WriteString(7, value.FileName);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ImportModelData* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->Key)) {
                    return false;
                }
                value->has_Key = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->Desc)) {
                    return false;
                }
                value->has_Desc = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->AuthUin)) {
                    return false;
                }
                value->has_AuthUin = true;
                break;
            }
            case 6: {
                if (!reader.ReadString(&value->AuthName)) {
                    return false;
                }
                value->has_AuthName = true;
                break;
            }
            case 7: {
                if (!reader.ReadString(&value->FileName)) {
                    return false;
                }
                value->has_FileName = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GraphicsAttr& value, ProtoWriter& writer) {
    if (value.has_grapicsid) {
        writer.WriteInt32(1, value.grapicsid);
    }
    if (value.has_Type) {
        writer.WriteInt32(2, value.Type);
    }
    if (value.has_Groupid) {
        writer.WriteInt32(3, value.Groupid);
    }
    if (value.has_Title) {
        writer.WriteString(4, value.Title);
    }
    if (value.has_Apha) {
        writer.WriteInt32(5, value.Apha);
    }
    if (value.has_CurVal) {
        writer.WriteInt32(6, value.CurVal);
    }
    if (value.has_MaxVal) {
        writer.WriteInt32(7, value.MaxVal);
    }
    if (value.has_Color) {
        writer.WriteInt32(8, value.Color);
    }
    if (value.has_Fontsize) {
        writer.WriteInt32(9, value.Fontsize);
    }
    if (value.has_WordPos) {
        ProtoWriter inner;
        Serialize(value.WordPos, inner);
        writer.WriteMessage(10, inner.Bytes());
    }
    if (value.has_bindObjID) {
        writer.WriteUInt64(11, value.bindObjID);
    }
    if (value.has_x2) {
        writer.WriteInt32(12, value.x2);
    }
    if (value.has_y2) {
        writer.WriteInt32(13, value.y2);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GraphicsAttr* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->grapicsid)) {
                    return false;
                }
                value->has_grapicsid = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Groupid)) {
                    return false;
                }
                value->has_Groupid = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Title)) {
                    return false;
                }
                value->has_Title = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Apha)) {
                    return false;
                }
                value->has_Apha = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->CurVal)) {
                    return false;
                }
                value->has_CurVal = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->MaxVal)) {
                    return false;
                }
                value->has_MaxVal = true;
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->Color)) {
                    return false;
                }
                value->has_Color = true;
                break;
            }
            case 9: {
                if (!reader.ReadInt32(&value->Fontsize)) {
                    return false;
                }
                value->has_Fontsize = true;
                break;
            }
            case 10: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->WordPos)) {
                    return false;
                }
                value->has_WordPos = true;
                break;
            }
            case 11: {
                if (!reader.ReadUInt64(&value->bindObjID)) {
                    return false;
                }
                value->has_bindObjID = true;
                break;
            }
            case 12: {
                if (!reader.ReadInt32(&value->x2)) {
                    return false;
                }
                value->has_x2 = true;
                break;
            }
            case 13: {
                if (!reader.ReadInt32(&value->y2)) {
                    return false;
                }
                value->has_y2 = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MsgErrorHC& value, ProtoWriter& writer) {
    if (value.has_ErrCode) {
        writer.WriteInt32(1, value.ErrCode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MsgErrorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ErrCode)) {
                    return false;
                }
                value->has_ErrCode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_HeartBeatHC& value, ProtoWriter& writer) {
    if (value.has_BeatCode) {
        writer.WriteInt32(1, value.BeatCode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_HeartBeatHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BeatCode)) {
                    return false;
                }
                value->has_BeatCode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SyncChunkDataHC& value, ProtoWriter& writer) {
    if (value.has_SectionFlags) {
        writer.WriteInt32(1, value.SectionFlags);
    }
    if (value.has_Initialize) {
        writer.WriteInt32(2, value.Initialize);
    }
    if (value.has_ChunkData) {
        ProtoWriter inner;
        Serialize(value.ChunkData, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SyncChunkDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SectionFlags)) {
                    return false;
                }
                value->has_SectionFlags = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Initialize)) {
                    return false;
                }
                value->has_Initialize = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ChunkData)) {
                    return false;
                }
                value->has_ChunkData = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BlockUpdateHC& value, ProtoWriter& writer) {
    if (value.has_ChunkX) {
        writer.WriteInt32(1, value.ChunkX);
    }
    if (value.has_ChunkZ) {
        writer.WriteInt32(2, value.ChunkZ);
    }
    if (value.has_MapID) {
        writer.WriteInt32(3, value.MapID);
    }
    for (const auto& item : value.Blocks) {
        writer.WriteUInt32(4, item);
    }
    if (value.has_ContainerBuf) {
        writer.WriteString(5, value.ContainerBuf);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BlockUpdateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ChunkX)) {
                    return false;
                }
                value->has_ChunkX = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ChunkZ)) {
                    return false;
                }
                value->has_ChunkZ = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 4: {
                uint32_t item{};
                if (!reader.ReadUInt32(&item)) {
                    return false;
                }
                value->Blocks.push_back(item);
                break;
            }
            case 5: {
                if (!reader.ReadString(&value->ContainerBuf)) {
                    return false;
                }
                value->has_ContainerBuf = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoleEnterWorldHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_PlayerInfo) {
        ProtoWriter inner;
        Serialize(value.PlayerInfo, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_GlobalInfo) {
        ProtoWriter inner;
        Serialize(value.GlobalInfo, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_WorldDesc) {
        ProtoWriter inner;
        Serialize(value.WorldDesc, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    if (value.has_SkillCDData) {
        ProtoWriter inner;
        Serialize(value.SkillCDData, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
    for (const auto& item : value.UnlockItems) {
        writer.WriteInt32(6, item);
    }
    if (value.has_Url) {
        writer.WriteString(7, value.Url);
    }
    if (value.has_HasRole) {
        writer.WriteBool(8, value.HasRole);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoleEnterWorldHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->PlayerInfo)) {
                    return false;
                }
                value->has_PlayerInfo = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->GlobalInfo)) {
                    return false;
                }
                value->has_GlobalInfo = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->WorldDesc)) {
                    return false;
                }
                value->has_WorldDesc = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->SkillCDData)) {
                    return false;
                }
                value->has_SkillCDData = true;
                break;
            }
            case 6: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->UnlockItems.push_back(item);
                break;
            }
            case 7: {
                if (!reader.ReadString(&value->Url)) {
                    return false;
                }
                value->has_Url = true;
                break;
            }
            case 8: {
                if (!reader.ReadBool(&value->HasRole)) {
                    return false;
                }
                value->has_HasRole = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoleLeaveWorldHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoleLeaveWorldHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorEnterAOIHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_ActorType) {
        writer.WriteInt32(2, value.ActorType);
    }
    if (value.has_ActorInfo) {
        ProtoWriter inner;
        Serialize(value.ActorInfo, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_Spectator_Mode) {
        writer.WriteInt32(4, value.Spectator_Mode);
    }
    if (value.has_Spectator_Type) {
        writer.WriteInt32(5, value.Spectator_Type);
    }
    if (value.has_HookID) {
        writer.WriteUInt64(6, value.HookID);
    }
    if (value.has_PlayMode) {
        writer.WriteUInt32(7, value.PlayMode);
    }
    if (value.has_PlayOperate) {
        writer.WriteUInt32(8, value.PlayOperate);
    }
    if (value.has_childUUID) {
        writer.WriteUInt64(9, value.childUUID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorEnterAOIHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ActorType)) {
                    return false;
                }
                value->has_ActorType = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ActorInfo)) {
                    return false;
                }
                value->has_ActorInfo = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Spectator_Mode)) {
                    return false;
                }
                value->has_Spectator_Mode = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Spectator_Type)) {
                    return false;
                }
                value->has_Spectator_Type = true;
                break;
            }
            case 6: {
                if (!reader.ReadUInt64(&value->HookID)) {
                    return false;
                }
                value->has_HookID = true;
                break;
            }
            case 7: {
                if (!reader.ReadUInt32(&value->PlayMode)) {
                    return false;
                }
                value->has_PlayMode = true;
                break;
            }
            case 8: {
                if (!reader.ReadUInt32(&value->PlayOperate)) {
                    return false;
                }
                value->has_PlayOperate = true;
                break;
            }
            case 9: {
                if (!reader.ReadUInt64(&value->childUUID)) {
                    return false;
                }
                value->has_childUUID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const AssembleBlockInfo& value, ProtoWriter& writer) {
    if (value.has_Block) {
        writer.WriteUInt32(1, value.Block);
    }
    if (value.has_data) {
        writer.WriteUInt32(2, value.data);
    }
    if (value.has_Info) {
        writer.WriteUInt32(3, value.Info);
    }
}

inline bool Deserialize(ProtoReader& reader, AssembleBlockInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->Block)) {
                    return false;
                }
                value->has_Block = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->data)) {
                    return false;
                }
                value->has_data = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt32(&value->Info)) {
                    return false;
                }
                value->has_Info = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAssembleBlockUpdateHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    for (const auto& item : value.BlockInfo) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_ContainerBuf) {
        writer.WriteString(3, value.ContainerBuf);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAssembleBlockUpdateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                AssembleBlockInfo item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->BlockInfo.push_back(item);
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->ContainerBuf)) {
                    return false;
                }
                value->has_ContainerBuf = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAssembleBlockAllHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_UnzipLen) {
        writer.WriteInt32(2, value.UnzipLen);
    }
    if (value.has_BlobLen) {
        writer.WriteInt32(3, value.BlobLen);
    }
    if (value.has_BlobDetail) {
        writer.WriteString(4, value.BlobDetail);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAssembleBlockAllHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->UnzipLen)) {
                    return false;
                }
                value->has_UnzipLen = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->BlobLen)) {
                    return false;
                }
                value->has_BlobLen = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->BlobDetail)) {
                    return false;
                }
                value->has_BlobDetail = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorLeaveAOIHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorLeaveAOIHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorMoveHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_MoveMotion) {
        ProtoWriter inner;
        Serialize(value.MoveMotion, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorMoveHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->MoveMotion)) {
                    return false;
                }
                value->has_MoveMotion = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TrainMoveHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_MapID) {
        writer.WriteUInt32(2, value.MapID);
    }
    if (value.has_CurveT) {
        writer.WriteFloat(3, value.CurveT);
    }
    if (value.has_CarReverse) {
        writer.WriteInt32(4, value.CarReverse);
    }
    if (value.has_HeadCar) {
        writer.WriteUInt64(5, value.HeadCar);
    }
    if (value.has_TailCar) {
        writer.WriteUInt64(6, value.TailCar);
    }
    if (value.has_OutIndex) {
        writer.WriteUInt32(7, value.OutIndex);
    }
    if (value.has_RailKnot) {
        ProtoWriter inner;
        Serialize(value.RailKnot, inner);
        writer.WriteMessage(8, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TrainMoveHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->CurveT)) {
                    return false;
                }
                value->has_CurveT = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->CarReverse)) {
                    return false;
                }
                value->has_CarReverse = true;
                break;
            }
            case 5: {
                if (!reader.ReadUInt64(&value->HeadCar)) {
                    return false;
                }
                value->has_HeadCar = true;
                break;
            }
            case 6: {
                if (!reader.ReadUInt64(&value->TailCar)) {
                    return false;
                }
                value->has_TailCar = true;
                break;
            }
            case 7: {
                if (!reader.ReadUInt32(&value->OutIndex)) {
                    return false;
                }
                value->has_OutIndex = true;
                break;
            }
            case 8: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RailKnot)) {
                    return false;
                }
                value->has_RailKnot = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorTeleportHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_TargetMap) {
        writer.WriteInt32(2, value.TargetMap);
    }
    if (value.has_TargetPos) {
        ProtoWriter inner;
        Serialize(value.TargetPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorTeleportHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->TargetMap)) {
                    return false;
                }
                value->has_TargetMap = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->TargetPos)) {
                    return false;
                }
                value->has_TargetPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorMotionHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_x) {
        writer.WriteFloat(2, value.x);
    }
    if (value.has_y) {
        writer.WriteFloat(3, value.y);
    }
    if (value.has_z) {
        writer.WriteFloat(4, value.z);
    }
    if (value.has_isChangePos) {
        writer.WriteBool(5, value.isChangePos);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorMotionHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->x)) {
                    return false;
                }
                value->has_x = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->y)) {
                    return false;
                }
                value->has_y = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->z)) {
                    return false;
                }
                value->has_z = true;
                break;
            }
            case 5: {
                if (!reader.ReadBool(&value->isChangePos)) {
                    return false;
                }
                value->has_isChangePos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MechaMotionHC& value, ProtoWriter& writer) {
    if (value.has_ObjId) {
        writer.WriteUInt64(1, value.ObjId);
    }
    if (value.has_MotionType) {
        writer.WriteInt32(2, value.MotionType);
    }
    if (value.has_MotionParam) {
        writer.WriteFloat(3, value.MotionParam);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MechaMotionHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MotionType)) {
                    return false;
                }
                value->has_MotionType = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->MotionParam)) {
                    return false;
                }
                value->has_MotionParam = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SyncTriggerBlock& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SyncTriggerBlock* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BlockInteractHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_face) {
        writer.WriteInt32(2, value.face);
    }
    if (value.has_blockpos) {
        ProtoWriter inner;
        Serialize(value.blockpos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BlockInteractHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->face)) {
                    return false;
                }
                value->has_face = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockpos)) {
                    return false;
                }
                value->has_blockpos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BlockPunchHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_status) {
        writer.WriteInt32(2, value.status);
    }
    if (value.has_face) {
        writer.WriteInt32(3, value.face);
    }
    if (value.has_digmethod) {
        writer.WriteInt32(4, value.digmethod);
    }
    if (value.has_blockpos) {
        ProtoWriter inner;
        Serialize(value.blockpos, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BlockPunchHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->status)) {
                    return false;
                }
                value->has_status = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->face)) {
                    return false;
                }
                value->has_face = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->digmethod)) {
                    return false;
                }
                value->has_digmethod = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockpos)) {
                    return false;
                }
                value->has_blockpos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemUseHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_itemid) {
        writer.WriteInt32(2, value.itemid);
    }
    if (value.has_status) {
        writer.WriteInt32(3, value.status);
    }
    if (value.has_shift) {
        writer.WriteInt32(4, value.shift);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemUseHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->itemid)) {
                    return false;
                }
                value->has_itemid = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->status)) {
                    return false;
                }
                value->has_status = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->shift)) {
                    return false;
                }
                value->has_shift = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetHookHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_hookID) {
        writer.WriteUInt64(2, value.hookID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetHookHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->hookID)) {
                    return false;
                }
                value->has_hookID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ItemSkillUseHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_itemid) {
        writer.WriteInt32(2, value.itemid);
    }
    if (value.has_status) {
        writer.WriteInt32(3, value.status);
    }
    if (value.has_skillid) {
        writer.WriteInt32(4, value.skillid);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ItemSkillUseHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->itemid)) {
                    return false;
                }
                value->has_itemid = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->status)) {
                    return false;
                }
                value->has_status = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->skillid)) {
                    return false;
                }
                value->has_skillid = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorInteractHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_target) {
        writer.WriteUInt64(2, value.target);
    }
    if (value.has_itype) {
        writer.WriteInt32(3, value.itype);
    }
    if (value.has_iplot) {
        writer.WriteInt32(4, value.iplot);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorInteractHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->target)) {
                    return false;
                }
                value->has_target = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->itype)) {
                    return false;
                }
                value->has_itype = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->iplot)) {
                    return false;
                }
                value->has_iplot = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorAnimHC& value, ProtoWriter& writer) {
    if (value.has_actorid) {
        writer.WriteUInt64(1, value.actorid);
    }
    if (value.has_anim) {
        writer.WriteInt32(2, value.anim);
    }
    if (value.has_anim1) {
        writer.WriteInt32(3, value.anim1);
    }
    if (value.has_actid) {
        writer.WriteInt32(4, value.actid);
    }
    if (value.has_bodyscale_invalid) {
        writer.WriteFloat(5, value.bodyscale_invalid);
    }
    if (value.has_customscale) {
        writer.WriteFloat(6, value.customscale);
    }
    if (value.has_actidTrigger) {
        writer.WriteInt32(7, value.actidTrigger);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorAnimHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->actorid)) {
                    return false;
                }
                value->has_actorid = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->anim)) {
                    return false;
                }
                value->has_anim = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->anim1)) {
                    return false;
                }
                value->has_anim1 = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->actid)) {
                    return false;
                }
                value->has_actid = true;
                break;
            }
            case 5: {
                if (!reader.ReadFloat(&value->bodyscale_invalid)) {
                    return false;
                }
                value->has_bodyscale_invalid = true;
                break;
            }
            case 6: {
                if (!reader.ReadFloat(&value->customscale)) {
                    return false;
                }
                value->has_customscale = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->actidTrigger)) {
                    return false;
                }
                value->has_actidTrigger = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackGridUpdateHC& value, ProtoWriter& writer) {
    for (const auto& item : value.ItemInfo) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackGridUpdateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->ItemInfo.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BackPackEquipWeaponHC& value, ProtoWriter& writer) {
    if (value.has_GridId) {
        writer.WriteInt32(1, value.GridId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BackPackEquipWeaponHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridId)) {
                    return false;
                }
                value->has_GridId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloseContainerHC& value, ProtoWriter& writer) {
    if (value.has_BaseIndex) {
        writer.WriteInt32(1, value.BaseIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CloseContainerHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BaseIndex)) {
                    return false;
                }
                value->has_BaseIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OpenContainerHC& value, ProtoWriter& writer) {
    if (value.has_BaseIndex) {
        writer.WriteInt32(1, value.BaseIndex);
    }
    if (value.has_TotalItemGrids) {
        writer.WriteInt32(2, value.TotalItemGrids);
    }
    for (const auto& item : value.AttribInfo) {
        writer.WriteFloat(3, item);
    }
    if (value.has_Text) {
        writer.WriteString(4, value.Text);
    }
    if (value.has_NpcID) {
        writer.WriteUInt64(5, value.NpcID);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
    for (const auto& item : value.ItemInfo) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
    if (value.has_VehicleObjID) {
        writer.WriteUInt64(8, value.VehicleObjID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OpenContainerHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BaseIndex)) {
                    return false;
                }
                value->has_BaseIndex = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->TotalItemGrids)) {
                    return false;
                }
                value->has_TotalItemGrids = true;
                break;
            }
            case 3: {
                float item{};
                if (!reader.ReadFloat(&item)) {
                    return false;
                }
                value->AttribInfo.push_back(item);
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Text)) {
                    return false;
                }
                value->has_Text = true;
                break;
            }
            case 5: {
                if (!reader.ReadUInt64(&value->NpcID)) {
                    return false;
                }
                value->has_NpcID = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->ItemInfo.push_back(item);
                break;
            }
            case 8: {
                if (!reader.ReadUInt64(&value->VehicleObjID)) {
                    return false;
                }
                value->has_VehicleObjID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_NeedContainerPasswordHC& value, ProtoWriter& writer) {
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_state) {
        writer.WriteInt32(2, value.state);
    }
    if (value.has_VehicleObjID) {
        writer.WriteUInt64(3, value.VehicleObjID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_NeedContainerPasswordHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->state)) {
                    return false;
                }
                value->has_state = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt64(&value->VehicleObjID)) {
                    return false;
                }
                value->has_VehicleObjID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_UpdateContainerHC& value, ProtoWriter& writer) {
    if (value.has_BaseIndex) {
        writer.WriteInt32(1, value.BaseIndex);
    }
    for (const auto& item : value.AttribInfo) {
        writer.WriteFloat(2, item);
    }
    if (value.has_Text) {
        writer.WriteString(3, value.Text);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    for (const auto& item : value.ItemInfo) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_UpdateContainerHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BaseIndex)) {
                    return false;
                }
                value->has_BaseIndex = true;
                break;
            }
            case 2: {
                float item{};
                if (!reader.ReadFloat(&item)) {
                    return false;
                }
                value->AttribInfo.push_back(item);
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->Text)) {
                    return false;
                }
                value->has_Text = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->ItemInfo.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorEquipItemHC& value, ProtoWriter& writer) {
    if (value.has_ObjId) {
        writer.WriteUInt64(1, value.ObjId);
    }
    if (value.has_SlotType) {
        writer.WriteInt32(2, value.SlotType);
    }
    if (value.has_ItemInfo) {
        ProtoWriter inner;
        Serialize(value.ItemInfo, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorEquipItemHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SlotType)) {
                    return false;
                }
                value->has_SlotType = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ItemInfo)) {
                    return false;
                }
                value->has_ItemInfo = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EnchantItemSuccessHC& value, ProtoWriter& writer) {
    if (value.has_GridIndex) {
        writer.WriteInt32(1, value.GridIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EnchantItemSuccessHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridIndex)) {
                    return false;
                }
                value->has_GridIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RepairItemSuccessHC& value, ProtoWriter& writer) {
    if (value.has_GridIndex) {
        writer.WriteInt32(1, value.GridIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RepairItemSuccessHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GridIndex)) {
                    return false;
                }
                value->has_GridIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GunDoReloadHC& value, ProtoWriter& writer) {
    if (value.has_BulletID) {
        writer.WriteInt32(1, value.BulletID);
    }
    if (value.has_Num) {
        writer.WriteInt32(2, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GunDoReloadHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->BulletID)) {
                    return false;
                }
                value->has_BulletID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_AccountHorseHC& value, ProtoWriter& writer) {
    if (value.has_HorseID) {
        writer.WriteInt32(1, value.HorseID);
    }
    if (value.has_SyncType) {
        writer.WriteInt32(2, value.SyncType);
    }
    if (value.has_SyncData) {
        writer.WriteInt32(3, value.SyncData);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_AccountHorseHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->HorseID)) {
                    return false;
                }
                value->has_HorseID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SyncType)) {
                    return false;
                }
                value->has_SyncType = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->SyncData)) {
                    return false;
                }
                value->has_SyncData = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorAttrChangeHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_HP) {
        writer.WriteFloat(2, value.HP);
    }
    if (value.has_BeHurtTarget) {
        writer.WriteUInt64(3, value.BeHurtTarget);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorAttrChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->HP)) {
                    return false;
                }
                value->has_HP = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt64(&value->BeHurtTarget)) {
                    return false;
                }
                value->has_BeHurtTarget = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorBuffChangeHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    for (const auto& item : value.Buffs) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorBuffChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ActorBuff item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Buffs.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorReviveHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    if (value.has_ReviveType) {
        writer.WriteInt32(3, value.ReviveType);
    }
    if (value.has_ReviveYaw) {
        writer.WriteFloat(4, value.ReviveYaw);
    }
    if (value.has_RevivePitch) {
        writer.WriteFloat(5, value.RevivePitch);
    }
    if (value.has_RevivePosition) {
        ProtoWriter inner;
        Serialize(value.RevivePosition, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorReviveHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ReviveType)) {
                    return false;
                }
                value->has_ReviveType = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->ReviveYaw)) {
                    return false;
                }
                value->has_ReviveYaw = true;
                break;
            }
            case 5: {
                if (!reader.ReadFloat(&value->RevivePitch)) {
                    return false;
                }
                value->has_RevivePitch = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RevivePosition)) {
                    return false;
                }
                value->has_RevivePosition = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerAttrChangeHC& value, ProtoWriter& writer) {
    if (value.has_HP) {
        writer.WriteFloat(1, value.HP);
    }
    if (value.has_Oxygen) {
        writer.WriteFloat(2, value.Oxygen);
    }
    if (value.has_Exp) {
        writer.WriteInt32(3, value.Exp);
    }
    if (value.has_FoodLevel) {
        writer.WriteInt32(4, value.FoodLevel);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerAttrChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->HP)) {
                    return false;
                }
                value->has_HP = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Oxygen)) {
                    return false;
                }
                value->has_Oxygen = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Exp)) {
                    return false;
                }
                value->has_Exp = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->FoodLevel)) {
                    return false;
                }
                value->has_FoodLevel = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MobBodyChangeHC& value, ProtoWriter& writer) {
    if (value.has_ObjId) {
        writer.WriteUInt64(1, value.ObjId);
    }
    if (value.has_BodyColor) {
        writer.WriteInt32(2, value.BodyColor);
    }
    if (value.has_Sheared) {
        writer.WriteInt32(3, value.Sheared);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MobBodyChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->BodyColor)) {
                    return false;
                }
                value->has_BodyColor = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Sheared)) {
                    return false;
                }
                value->has_Sheared = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_JruisdicTionHC& value, ProtoWriter& writer) {
    if (value.has_Ret) {
        writer.WriteInt32(1, value.Ret);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_JruisdicTionHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Ret)) {
                    return false;
                }
                value->has_Ret = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ChatHC& value, ProtoWriter& writer) {
    if (value.has_ChatType) {
        writer.WriteInt32(1, value.ChatType);
    }
    if (value.has_Uin) {
        writer.WriteInt32(2, value.Uin);
    }
    if (value.has_Speaker) {
        writer.WriteString(3, value.Speaker);
    }
    if (value.has_Content) {
        writer.WriteString(4, value.Content);
    }
    if (value.has_Language) {
        writer.WriteInt32(5, value.Language);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ChatHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ChatType)) {
                    return false;
                }
                value->has_ChatType = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->Speaker)) {
                    return false;
                }
                value->has_Speaker = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Content)) {
                    return false;
                }
                value->has_Content = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Language)) {
                    return false;
                }
                value->has_Language = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_WGlobalUpdateHC& value, ProtoWriter& writer) {
    if (value.has_WorldTime) {
        writer.WriteInt32(1, value.WorldTime);
    }
    if (value.has_ViewRange) {
        writer.WriteInt32(2, value.ViewRange);
    }
    if (value.has_MapID) {
        writer.WriteInt32(3, value.MapID);
    }
    if (value.has_Raining) {
        writer.WriteInt32(4, value.Raining);
    }
    if (value.has_DayNightTime) {
        writer.WriteInt32(5, value.DayNightTime);
    }
    if (value.has_Darking) {
        writer.WriteInt32(6, value.Darking);
    }
    if (value.has_CurWeather) {
        writer.WriteInt32(7, value.CurWeather);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_WGlobalUpdateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->WorldTime)) {
                    return false;
                }
                value->has_WorldTime = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ViewRange)) {
                    return false;
                }
                value->has_ViewRange = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Raining)) {
                    return false;
                }
                value->has_Raining = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->DayNightTime)) {
                    return false;
                }
                value->has_DayNightTime = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->Darking)) {
                    return false;
                }
                value->has_Darking = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->CurWeather)) {
                    return false;
                }
                value->has_CurWeather = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayersUpdateInfoHC& value, ProtoWriter& writer) {
    for (const auto& item : value.TeamScores) {
        writer.WriteInt32(1, item);
    }
    for (const auto& item : value.TeamFlags) {
        writer.WriteInt32(2, item);
    }
    for (const auto& item : value.Players) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayersUpdateInfoHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->TeamScores.push_back(item);
                break;
            }
            case 2: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->TeamFlags.push_back(item);
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_PlayerBriefInfo item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Players.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GameTipsHC& value, ProtoWriter& writer) {
    if (value.has_TipsType) {
        writer.WriteInt32(1, value.TipsType);
    }
    if (value.has_Id) {
        writer.WriteInt32(2, value.Id);
    }
    if (value.has_Num) {
        writer.WriteInt32(3, value.Num);
    }
    if (value.has_OtherName) {
        writer.WriteString(4, value.OtherName);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GameTipsHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->TipsType)) {
                    return false;
                }
                value->has_TipsType = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Id)) {
                    return false;
                }
                value->has_Id = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->OtherName)) {
                    return false;
                }
                value->has_OtherName = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayEffectHC& value, ProtoWriter& writer) {
    if (value.has_EffectType) {
        writer.WriteInt32(1, value.EffectType);
    }
    if (value.has_Particle) {
        ProtoWriter inner;
        Serialize(value.Particle, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_PickItem) {
        ProtoWriter inner;
        Serialize(value.PickItem, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_Sound) {
        ProtoWriter inner;
        Serialize(value.Sound, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    if (value.has_ActorBody) {
        ProtoWriter inner;
        Serialize(value.ActorBody, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
    if (value.has_DestroyBlock) {
        ProtoWriter inner;
        Serialize(value.DestroyBlock, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
    if (value.has_PlayMusicGrid) {
        ProtoWriter inner;
        Serialize(value.PlayMusicGrid, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
    if (value.has_StopMusicGrid) {
        ProtoWriter inner;
        Serialize(value.StopMusicGrid, inner);
        writer.WriteMessage(8, inner.Bytes());
    }
    if (value.has_StringActorBody) {
        ProtoWriter inner;
        Serialize(value.StringActorBody, inner);
        writer.WriteMessage(9, inner.Bytes());
    }
    if (value.has_CrackBlock) {
        ProtoWriter inner;
        Serialize(value.CrackBlock, inner);
        writer.WriteMessage(10, inner.Bytes());
    }
    if (value.has_effectScale) {
        writer.WriteFloat(11, value.effectScale);
    }
    if (value.has_effectClass) {
        writer.WriteInt32(12, value.effectClass);
    }
    if (value.has_TriggerSound) {
        ProtoWriter inner;
        Serialize(value.TriggerSound, inner);
        writer.WriteMessage(13, inner.Bytes());
    }
    if (value.has_EffectVehicle) {
        ProtoWriter inner;
        Serialize(value.EffectVehicle, inner);
        writer.WriteMessage(14, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayEffectHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->EffectType)) {
                    return false;
                }
                value->has_EffectType = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Particle)) {
                    return false;
                }
                value->has_Particle = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->PickItem)) {
                    return false;
                }
                value->has_PickItem = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Sound)) {
                    return false;
                }
                value->has_Sound = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ActorBody)) {
                    return false;
                }
                value->has_ActorBody = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->DestroyBlock)) {
                    return false;
                }
                value->has_DestroyBlock = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->PlayMusicGrid)) {
                    return false;
                }
                value->has_PlayMusicGrid = true;
                break;
            }
            case 8: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->StopMusicGrid)) {
                    return false;
                }
                value->has_StopMusicGrid = true;
                break;
            }
            case 9: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->StringActorBody)) {
                    return false;
                }
                value->has_StringActorBody = true;
                break;
            }
            case 10: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->CrackBlock)) {
                    return false;
                }
                value->has_CrackBlock = true;
                break;
            }
            case 11: {
                if (!reader.ReadFloat(&value->effectScale)) {
                    return false;
                }
                value->has_effectScale = true;
                break;
            }
            case 12: {
                if (!reader.ReadInt32(&value->effectClass)) {
                    return false;
                }
                value->has_effectClass = true;
                break;
            }
            case 13: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->TriggerSound)) {
                    return false;
                }
                value->has_TriggerSound = true;
                break;
            }
            case 14: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->EffectVehicle)) {
                    return false;
                }
                value->has_EffectVehicle = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_EffectScaleHC& value, ProtoWriter& writer) {
    if (value.has_EffectType) {
        writer.WriteInt32(1, value.EffectType);
    }
    if (value.has_effectScale) {
        writer.WriteFloat(2, value.effectScale);
    }
    if (value.has_objID) {
        writer.WriteUInt64(3, value.objID);
    }
    if (value.has_effectName) {
        writer.WriteString(4, value.effectName);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_EffectScaleHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->EffectType)) {
                    return false;
                }
                value->has_EffectType = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->effectScale)) {
                    return false;
                }
                value->has_effectScale = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt64(&value->objID)) {
                    return false;
                }
                value->has_objID = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->effectName)) {
                    return false;
                }
                value->has_effectName = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerMountActorHC& value, ProtoWriter& writer) {
    if (value.has_PlayerUIN) {
        writer.WriteInt32(1, value.PlayerUIN);
    }
    if (value.has_RidePosIndex) {
        writer.WriteInt32(2, value.RidePosIndex);
    }
    if (value.has_ActorID) {
        writer.WriteUInt64(3, value.ActorID);
    }
    if (value.has_force) {
        writer.WriteInt32(4, value.force);
    }
    if (value.has_InteractBlockId) {
        writer.WriteInt32(5, value.InteractBlockId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerMountActorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->PlayerUIN)) {
                    return false;
                }
                value->has_PlayerUIN = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->RidePosIndex)) {
                    return false;
                }
                value->has_RidePosIndex = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->force)) {
                    return false;
                }
                value->has_force = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->InteractBlockId)) {
                    return false;
                }
                value->has_InteractBlockId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerSleepHC& value, ProtoWriter& writer) {
    if (value.has_Flags) {
        writer.WriteInt32(1, value.Flags);
    }
    if (value.has_Pos) {
        ProtoWriter inner;
        Serialize(value.Pos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerSleepHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Flags)) {
                    return false;
                }
                value->has_Flags = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Pos)) {
                    return false;
                }
                value->has_Pos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OpenWindowHC& value, ProtoWriter& writer) {
    if (value.has_id) {
        writer.WriteInt32(1, value.id);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OpenWindowHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->id)) {
                    return false;
                }
                value->has_id = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_LastPingHC& value, ProtoWriter& writer) {
    for (const auto& item : value.MPLastPingOne) {
        writer.WriteInt32(1, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_LastPingHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->MPLastPingOne.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CGameStageHC& value, ProtoWriter& writer) {
    if (value.has_Stage) {
        writer.WriteInt32(1, value.Stage);
    }
    if (value.has_GameTime) {
        writer.WriteInt32(2, value.GameTime);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CGameStageHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Stage)) {
                    return false;
                }
                value->has_Stage = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->GameTime)) {
                    return false;
                }
                value->has_GameTime = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerPermitHC& value, ProtoWriter& writer) {
    if (value.has_RoomFlags) {
        writer.WriteUInt32(1, value.RoomFlags);
    }
    if (value.has_PlayerFlags) {
        writer.WriteUInt32(2, value.PlayerFlags);
    }
    if (value.has_SpamPreventionMinutes) {
        writer.WriteUInt32(3, value.SpamPreventionMinutes);
    }
    for (const auto& item : value.BanItems) {
        writer.WriteInt32(4, item);
    }
    for (const auto& item : value.MutePlayerlist) {
        writer.WriteUInt32(5, item);
    }
    for (const auto& item : value.PlayerSpeakingWhitelist) {
        writer.WriteUInt32(6, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerPermitHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->RoomFlags)) {
                    return false;
                }
                value->has_RoomFlags = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->PlayerFlags)) {
                    return false;
                }
                value->has_PlayerFlags = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt32(&value->SpamPreventionMinutes)) {
                    return false;
                }
                value->has_SpamPreventionMinutes = true;
                break;
            }
            case 4: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->BanItems.push_back(item);
                break;
            }
            case 5: {
                uint32_t item{};
                if (!reader.ReadUInt32(&item)) {
                    return false;
                }
                value->MutePlayerlist.push_back(item);
                break;
            }
            case 6: {
                uint32_t item{};
                if (!reader.ReadUInt32(&item)) {
                    return false;
                }
                value->PlayerSpeakingWhitelist.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_YMVoiceHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_YMMemberID) {
        writer.WriteInt32(2, value.YMMemberID);
    }
    if (value.has_SpeakerSwitch) {
        writer.WriteInt32(3, value.SpeakerSwitch);
    }
    if (value.has_MicSwitch) {
        writer.WriteInt32(4, value.MicSwitch);
    }
    if (value.has_YMMemberRole) {
        writer.WriteInt32(5, value.YMMemberRole);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_YMVoiceHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->YMMemberID)) {
                    return false;
                }
                value->has_YMMemberID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->SpeakerSwitch)) {
                    return false;
                }
                value->has_SpeakerSwitch = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->MicSwitch)) {
                    return false;
                }
                value->has_MicSwitch = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->YMMemberRole)) {
                    return false;
                }
                value->has_YMMemberRole = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SkillCDHC& value, ProtoWriter& writer) {
    if (value.has_ItemID) {
        writer.WriteInt32(1, value.ItemID);
    }
    if (value.has_CD) {
        writer.WriteFloat(2, value.CD);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SkillCDHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ItemID)) {
                    return false;
                }
                value->has_ItemID = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->CD)) {
                    return false;
                }
                value->has_CD = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Horse_SkillCDHC& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_Index) {
        writer.WriteInt32(2, value.Index);
    }
    if (value.has_CD) {
        writer.WriteFloat(3, value.CD);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Horse_SkillCDHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Index)) {
                    return false;
                }
                value->has_Index = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->CD)) {
                    return false;
                }
                value->has_CD = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorMountActorHC& value, ProtoWriter& writer) {
    if (value.has_ActorIDOwner) {
        writer.WriteUInt64(1, value.ActorIDOwner);
    }
    if (value.has_ActorID) {
        writer.WriteUInt64(2, value.ActorID);
    }
    if (value.has_RidePosIndex) {
        writer.WriteInt32(3, value.RidePosIndex);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorMountActorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorIDOwner)) {
                    return false;
                }
                value->has_ActorIDOwner = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->RidePosIndex)) {
                    return false;
                }
                value->has_RidePosIndex = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorReverseHC& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_Reverse) {
        writer.WriteInt32(2, value.Reverse);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorReverseHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Reverse)) {
                    return false;
                }
                value->has_Reverse = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorBindHC& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_ActorIDBind) {
        writer.WriteUInt64(2, value.ActorIDBind);
    }
    if (value.has_OffetBind) {
        ProtoWriter inner;
        Serialize(value.OffetBind, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorBindHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorIDBind)) {
                    return false;
                }
                value->has_ActorIDBind = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->OffetBind)) {
                    return false;
                }
                value->has_OffetBind = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayWeaponEffectHC& value, ProtoWriter& writer) {
    if (value.has_EffectName) {
        writer.WriteString(1, value.EffectName);
    }
    if (value.has_EffectID) {
        writer.WriteInt32(2, value.EffectID);
    }
    if (value.has_EffectStatus) {
        writer.WriteInt32(3, value.EffectStatus);
    }
    if (value.has_EffectScale) {
        writer.WriteInt32(4, value.EffectScale);
    }
    if (value.has_ObjId) {
        writer.WriteUInt64(5, value.ObjId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayWeaponEffectHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->EffectName)) {
                    return false;
                }
                value->has_EffectName = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->EffectID)) {
                    return false;
                }
                value->has_EffectID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->EffectStatus)) {
                    return false;
                }
                value->has_EffectStatus = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->EffectScale)) {
                    return false;
                }
                value->has_EffectScale = true;
                break;
            }
            case 5: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ScriptVarHC& value, ProtoWriter& writer) {
    for (const auto& item : value.ScriptVars) {
        writer.WriteFloat(1, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ScriptVarHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                float item{};
                if (!reader.ReadFloat(&item)) {
                    return false;
                }
                value->ScriptVars.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GVChangeRoleHC& value, ProtoWriter& writer) {
    if (value.has_GVMemberRole) {
        writer.WriteInt32(1, value.GVMemberRole);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GVChangeRoleHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->GVMemberRole)) {
                    return false;
                }
                value->has_GVMemberRole = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_YMChangeRoleHC& value, ProtoWriter& writer) {
    if (value.has_YMMemberRole) {
        writer.WriteInt32(1, value.YMMemberRole);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_YMChangeRoleHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->YMMemberRole)) {
                    return false;
                }
                value->has_YMMemberRole = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SpecialItemUseHC& value, ProtoWriter& writer) {
    if (value.has_ItemId) {
        writer.WriteInt32(1, value.ItemId);
    }
    if (value.has_ItemNum) {
        writer.WriteInt32(2, value.ItemNum);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SpecialItemUseHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ItemId)) {
                    return false;
                }
                value->has_ItemId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ItemNum)) {
                    return false;
                }
                value->has_ItemNum = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_LeaveRoomInfoHC& value, ProtoWriter& writer) {
    if (value.has_Cause) {
        writer.WriteInt32(1, value.Cause);
    }
    if (value.has_KickerType) {
        writer.WriteInt32(2, value.KickerType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_LeaveRoomInfoHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Cause)) {
                    return false;
                }
                value->has_Cause = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->KickerType)) {
                    return false;
                }
                value->has_KickerType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_InviteJoinRoomHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_RoomState) {
        writer.WriteString(2, value.RoomState);
    }
    if (value.has_PassWorld) {
        writer.WriteString(3, value.PassWorld);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_InviteJoinRoomHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->RoomState)) {
                    return false;
                }
                value->has_RoomState = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->PassWorld)) {
                    return false;
                }
                value->has_PassWorld = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetSpectatorModeHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_SpectatorMode) {
        writer.WriteInt32(2, value.SpectatorMode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetSpectatorModeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SpectatorMode)) {
                    return false;
                }
                value->has_SpectatorMode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetSpectatorTypeHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_SpectatorType) {
        writer.WriteInt32(2, value.SpectatorType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetSpectatorTypeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SpectatorType)) {
                    return false;
                }
                value->has_SpectatorType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetSpectatorPlayerHC& value, ProtoWriter& writer) {
    if (value.has_SpectatorUin) {
        writer.WriteInt32(1, value.SpectatorUin);
    }
    if (value.has_ToSpectatorUin) {
        writer.WriteInt32(2, value.ToSpectatorUin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetSpectatorPlayerHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpectatorUin)) {
                    return false;
                }
                value->has_SpectatorUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToSpectatorUin)) {
                    return false;
                }
                value->has_ToSpectatorUin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetPlayerModelAniHC& value, ProtoWriter& writer) {
    if (value.has_SpectatorUin) {
        writer.WriteInt32(1, value.SpectatorUin);
    }
    if (value.has_ToSpectatorUin) {
        writer.WriteInt32(2, value.ToSpectatorUin);
    }
    if (value.has_ModelAnimalType) {
        writer.WriteInt32(3, value.ModelAnimalType);
    }
    if (value.has_ModelAnimalExt) {
        writer.WriteInt32(4, value.ModelAnimalExt);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetPlayerModelAniHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpectatorUin)) {
                    return false;
                }
                value->has_SpectatorUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToSpectatorUin)) {
                    return false;
                }
                value->has_ToSpectatorUin = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ModelAnimalType)) {
                    return false;
                }
                value->has_ModelAnimalType = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->ModelAnimalExt)) {
                    return false;
                }
                value->has_ModelAnimalExt = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SendMyViewmodeToSpectatorHC& value, ProtoWriter& writer) {
    if (value.has_SpectatorUin) {
        writer.WriteInt32(1, value.SpectatorUin);
    }
    if (value.has_ToSpectatorUin) {
        writer.WriteInt32(2, value.ToSpectatorUin);
    }
    if (value.has_MyViewmode) {
        writer.WriteInt32(3, value.MyViewmode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SendMyViewmodeToSpectatorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpectatorUin)) {
                    return false;
                }
                value->has_SpectatorUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToSpectatorUin)) {
                    return false;
                }
                value->has_ToSpectatorUin = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->MyViewmode)) {
                    return false;
                }
                value->has_MyViewmode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SetBobbingToSpectatorHC& value, ProtoWriter& writer) {
    if (value.has_SpectatorUin) {
        writer.WriteInt32(1, value.SpectatorUin);
    }
    if (value.has_ToSpectatorUin) {
        writer.WriteInt32(2, value.ToSpectatorUin);
    }
    if (value.has_Bobbing) {
        writer.WriteInt32(3, value.Bobbing);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SetBobbingToSpectatorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SpectatorUin)) {
                    return false;
                }
                value->has_SpectatorUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ToSpectatorUin)) {
                    return false;
                }
                value->has_ToSpectatorUin = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Bobbing)) {
                    return false;
                }
                value->has_Bobbing = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BallOperateHC& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ActorID) {
        writer.WriteUInt64(2, value.ActorID);
    }
    if (value.has_ExtendData) {
        writer.WriteInt32(3, value.ExtendData);
    }
    if (value.has_Uin) {
        writer.WriteInt32(4, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BallOperateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ExtendData)) {
                    return false;
                }
                value->has_ExtendData = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BasketBallOperateHC& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ActorID) {
        writer.WriteUInt64(2, value.ActorID);
    }
    if (value.has_IsSelectedTarget) {
        writer.WriteBool(3, value.IsSelectedTarget);
    }
    if (value.has_FallResult) {
        writer.WriteInt32(4, value.FallResult);
    }
    if (value.has_ExtendData) {
        writer.WriteInt32(5, value.ExtendData);
    }
    if (value.has_Uin) {
        writer.WriteInt32(6, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BasketBallOperateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->IsSelectedTarget)) {
                    return false;
                }
                value->has_IsSelectedTarget = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->FallResult)) {
                    return false;
                }
                value->has_FallResult = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->ExtendData)) {
                    return false;
                }
                value->has_ExtendData = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ResetRoundHC& value, ProtoWriter& writer) {
}

inline bool Deserialize(ProtoReader& reader, PB_ResetRoundHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RocketAttribChangeHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_State) {
        writer.WriteInt32(2, value.State);
    }
    if (value.has_Fuel) {
        writer.WriteInt32(3, value.Fuel);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RocketAttribChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->State)) {
                    return false;
                }
                value->has_State = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Fuel)) {
                    return false;
                }
                value->has_Fuel = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorBodyTextureHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_TexName) {
        writer.WriteString(2, value.TexName);
    }
    if (value.has_MeshName) {
        writer.WriteString(3, value.MeshName);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorBodyTextureHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->TexName)) {
                    return false;
                }
                value->has_TexName = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->MeshName)) {
                    return false;
                }
                value->has_MeshName = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_AttractAttribChangeHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_State) {
        writer.WriteInt32(2, value.State);
    }
    if (value.has_blockID) {
        writer.WriteInt32(3, value.blockID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_AttractAttribChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->State)) {
                    return false;
                }
                value->has_State = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->blockID)) {
                    return false;
                }
                value->has_blockID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_WorldTimesHC& value, ProtoWriter& writer) {
    for (const auto& item : value.Times) {
        writer.WriteInt32(1, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_WorldTimesHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->Times.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_StatisticHC& value, ProtoWriter& writer) {
    if (value.has_EventID) {
        writer.WriteInt32(1, value.EventID);
    }
    if (value.has_WorldType) {
        writer.WriteInt32(2, value.WorldType);
    }
    if (value.has_FristName) {
        writer.WriteString(3, value.FristName);
    }
    if (value.has_Param1) {
        writer.WriteString(4, value.Param1);
    }
    if (value.has_Param2) {
        writer.WriteString(5, value.Param2);
    }
    if (value.has_Param3) {
        writer.WriteString(6, value.Param3);
    }
    if (value.has_Param4) {
        writer.WriteString(7, value.Param4);
    }
    if (value.has_Param5) {
        writer.WriteString(8, value.Param5);
    }
    if (value.has_Param6) {
        writer.WriteString(9, value.Param6);
    }
    if (value.has_Param7) {
        writer.WriteString(10, value.Param7);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_StatisticHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->EventID)) {
                    return false;
                }
                value->has_EventID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->WorldType)) {
                    return false;
                }
                value->has_WorldType = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->FristName)) {
                    return false;
                }
                value->has_FristName = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Param1)) {
                    return false;
                }
                value->has_Param1 = true;
                break;
            }
            case 5: {
                if (!reader.ReadString(&value->Param2)) {
                    return false;
                }
                value->has_Param2 = true;
                break;
            }
            case 6: {
                if (!reader.ReadString(&value->Param3)) {
                    return false;
                }
                value->has_Param3 = true;
                break;
            }
            case 7: {
                if (!reader.ReadString(&value->Param4)) {
                    return false;
                }
                value->has_Param4 = true;
                break;
            }
            case 8: {
                if (!reader.ReadString(&value->Param5)) {
                    return false;
                }
                value->has_Param5 = true;
                break;
            }
            case 9: {
                if (!reader.ReadString(&value->Param6)) {
                    return false;
                }
                value->has_Param6 = true;
                break;
            }
            case 10: {
                if (!reader.ReadString(&value->Param7)) {
                    return false;
                }
                value->has_Param7 = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TotemPointHC& value, ProtoWriter& writer) {
    if (value.has_Op) {
        writer.WriteInt32(1, value.Op);
    }
    if (value.has_Point) {
        ProtoWriter inner;
        Serialize(value.Point, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_MapID) {
        writer.WriteInt32(3, value.MapID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TotemPointHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Op)) {
                    return false;
                }
                value->has_Op = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Point)) {
                    return false;
                }
                value->has_Point = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_HorseFlyStateHC& value, ProtoWriter& writer) {
    if (value.has_m_fEnergy) {
        writer.WriteFloat(1, value.m_fEnergy);
    }
    if (value.has_m_bTired) {
        writer.WriteInt32(2, value.m_bTired);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_HorseFlyStateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->m_fEnergy)) {
                    return false;
                }
                value->has_m_fEnergy = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->m_bTired)) {
                    return false;
                }
                value->has_m_bTired = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OpenDialogueHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    for (const auto& item : value.InteractData) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_ItemID) {
        writer.WriteInt32(3, value.ItemID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OpenDialogueHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_IntertactData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->InteractData.push_back(item);
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ItemID)) {
                    return false;
                }
                value->has_ItemID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloseDialogueHC& value, ProtoWriter& writer) {
}

inline bool Deserialize(ProtoReader& reader, PB_CloseDialogueHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_UpdateTaskHC& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ID) {
        writer.WriteInt32(2, value.ID);
    }
    if (value.has_Num) {
        writer.WriteInt32(3, value.Num);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_UpdateTaskHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Num)) {
                    return false;
                }
                value->has_Num = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SyncTaskEnterWorldHC& value, ProtoWriter& writer) {
    for (const auto& item : value.TaskInfoData) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SyncTaskEnterWorldHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_TaskInfoData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->TaskInfoData.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CompleteTaskHC& value, ProtoWriter& writer) {
    if (value.has_TaskID) {
        writer.WriteInt32(1, value.TaskID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CompleteTaskHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->TaskID)) {
                    return false;
                }
                value->has_TaskID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerAddAvartarHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_avatarmodel) {
        writer.WriteInt32(2, value.avatarmodel);
    }
    if (value.has_index) {
        writer.WriteInt32(3, value.index);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerAddAvartarHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->avatarmodel)) {
                    return false;
                }
                value->has_avatarmodel = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->index)) {
                    return false;
                }
                value->has_index = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerChangeModelHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_playerindex) {
        writer.WriteInt32(2, value.playerindex);
    }
    if (value.has_customskins) {
        writer.WriteString(3, value.customskins);
    }
    if (value.has_Reason) {
        writer.WriteInt32(4, value.Reason);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerChangeModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->playerindex)) {
                    return false;
                }
                value->has_playerindex = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->customskins)) {
                    return false;
                }
                value->has_customskins = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Reason)) {
                    return false;
                }
                value->has_Reason = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerAvartarColorHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_r) {
        writer.WriteFloat(2, value.r);
    }
    if (value.has_g) {
        writer.WriteFloat(3, value.g);
    }
    if (value.has_b) {
        writer.WriteFloat(4, value.b);
    }
    if (value.has_partID) {
        writer.WriteFloat(5, value.partID);
    }
    if (value.has_modelID) {
        writer.WriteFloat(6, value.modelID);
    }
    if (value.has_block) {
        writer.WriteFloat(7, value.block);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerAvartarColorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->r)) {
                    return false;
                }
                value->has_r = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->g)) {
                    return false;
                }
                value->has_g = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->b)) {
                    return false;
                }
                value->has_b = true;
                break;
            }
            case 5: {
                if (!reader.ReadFloat(&value->partID)) {
                    return false;
                }
                value->has_partID = true;
                break;
            }
            case 6: {
                if (!reader.ReadFloat(&value->modelID)) {
                    return false;
                }
                value->has_modelID = true;
                break;
            }
            case 7: {
                if (!reader.ReadFloat(&value->block)) {
                    return false;
                }
                value->has_block = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayActHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_ActID) {
        writer.WriteInt32(2, value.ActID);
    }
    if (value.has_ActIDTrigger) {
        writer.WriteInt32(3, value.ActIDTrigger);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayActHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ActID)) {
                    return false;
                }
                value->has_ActID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ActIDTrigger)) {
                    return false;
                }
                value->has_ActIDTrigger = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CreateBlueprintHC& value, ProtoWriter& writer) {
    if (value.has_Point) {
        ProtoWriter inner;
        Serialize(value.Point, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_sheetname) {
        writer.WriteString(2, value.sheetname);
    }
    if (value.has_authorname) {
        writer.WriteString(3, value.authorname);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CreateBlueprintHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Point)) {
                    return false;
                }
                value->has_Point = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->sheetname)) {
                    return false;
                }
                value->has_sheetname = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->authorname)) {
                    return false;
                }
                value->has_authorname = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_MeasureDistanceHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    if (value.has_blockpos) {
        ProtoWriter inner;
        Serialize(value.blockpos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    for (const auto& item : value.findpos) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_MeasureDistanceHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockpos)) {
                    return false;
                }
                value->has_blockpos = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_Vector3 item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->findpos.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_BluePrintPreBlockHC& value, ProtoWriter& writer) {
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    for (const auto& item : value.PreBlocks) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_BluePrintPreBlockHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_PreBlockData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->PreBlocks.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GravityOperateHC& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ActorID) {
        writer.WriteUInt64(2, value.ActorID);
    }
    if (value.has_ExtendData) {
        writer.WriteInt32(3, value.ExtendData);
    }
    if (value.has_Uin) {
        writer.WriteInt32(4, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GravityOperateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ExtendData)) {
                    return false;
                }
                value->has_ExtendData = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerBodyColorHC& value, ProtoWriter& writer) {
    if (value.has_DestColor) {
        writer.WriteUInt32(1, value.DestColor);
    }
    if (value.has_CurColor) {
        writer.WriteUInt32(2, value.CurColor);
    }
    if (value.has_Uin) {
        writer.WriteInt32(3, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerBodyColorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->DestColor)) {
                    return false;
                }
                value->has_DestColor = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->CurColor)) {
                    return false;
                }
                value->has_CurColor = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CustomModelHC& value, ProtoWriter& writer) {
    if (value.has_UnzipLen) {
        writer.WriteInt32(1, value.UnzipLen);
    }
    if (value.has_BlobLen) {
        writer.WriteInt32(2, value.BlobLen);
    }
    if (value.has_BlobDetail) {
        writer.WriteString(3, value.BlobDetail);
    }
    if (value.has_FileName) {
        writer.WriteString(4, value.FileName);
    }
    if (value.has_ItemID) {
        writer.WriteInt32(5, value.ItemID);
    }
    if (value.has_ModelName) {
        writer.WriteString(6, value.ModelName);
    }
    if (value.has_ModelDesc) {
        writer.WriteString(7, value.ModelDesc);
    }
    if (value.has_ClassName) {
        writer.WriteString(8, value.ClassName);
    }
    if (value.has_Type) {
        writer.WriteInt32(9, value.Type);
    }
    if (value.has_Box) {
        ProtoWriter inner;
        Serialize(value.Box, inner);
        writer.WriteMessage(10, inner.Bytes());
    }
    if (value.has_folderIndex) {
        writer.WriteInt32(11, value.folderIndex);
    }
    if (value.has_IsDownload) {
        writer.WriteBool(12, value.IsDownload);
    }
    if (value.has_AuthUin) {
        writer.WriteInt32(13, value.AuthUin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CustomModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->UnzipLen)) {
                    return false;
                }
                value->has_UnzipLen = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->BlobLen)) {
                    return false;
                }
                value->has_BlobLen = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->BlobDetail)) {
                    return false;
                }
                value->has_BlobDetail = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->FileName)) {
                    return false;
                }
                value->has_FileName = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->ItemID)) {
                    return false;
                }
                value->has_ItemID = true;
                break;
            }
            case 6: {
                if (!reader.ReadString(&value->ModelName)) {
                    return false;
                }
                value->has_ModelName = true;
                break;
            }
            case 7: {
                if (!reader.ReadString(&value->ModelDesc)) {
                    return false;
                }
                value->has_ModelDesc = true;
                break;
            }
            case 8: {
                if (!reader.ReadString(&value->ClassName)) {
                    return false;
                }
                value->has_ClassName = true;
                break;
            }
            case 9: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 10: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Box)) {
                    return false;
                }
                value->has_Box = true;
                break;
            }
            case 11: {
                if (!reader.ReadInt32(&value->folderIndex)) {
                    return false;
                }
                value->has_folderIndex = true;
                break;
            }
            case 12: {
                if (!reader.ReadBool(&value->IsDownload)) {
                    return false;
                }
                value->has_IsDownload = true;
                break;
            }
            case 13: {
                if (!reader.ReadInt32(&value->AuthUin)) {
                    return false;
                }
                value->has_AuthUin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CustomModelPrepareHC& value, ProtoWriter& writer) {
    if (value.has_FileName) {
        writer.WriteString(1, value.FileName);
    }
    if (value.has_Index) {
        writer.WriteInt32(2, value.Index);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CustomModelPrepareHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->FileName)) {
                    return false;
                }
                value->has_FileName = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Index)) {
                    return false;
                }
                value->has_Index = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CustomItemIDsHC& value, ProtoWriter& writer) {
    for (const auto& item : value.CustomItemIDs) {
        writer.WriteInt32(1, item);
    }
    for (const auto& item : value.CustomModelFileNames) {
        writer.WriteString(2, item);
    }
    for (const auto& item : value.CustomModelClassNames) {
        writer.WriteString(3, item);
    }
    for (const auto& item : value.CustomTypes) {
        writer.WriteInt32(4, item);
    }
    for (const auto& item : value.InvolvedIds) {
        writer.WriteInt32(5, item);
    }
    for (const auto& item : value.CustomModelFolderIndexs) {
        writer.WriteInt32(6, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CustomItemIDsHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->CustomItemIDs.push_back(item);
                break;
            }
            case 2: {
                std::string item{};
                if (!reader.ReadString(&item)) {
                    return false;
                }
                value->CustomModelFileNames.push_back(item);
                break;
            }
            case 3: {
                std::string item{};
                if (!reader.ReadString(&item)) {
                    return false;
                }
                value->CustomModelClassNames.push_back(item);
                break;
            }
            case 4: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->CustomTypes.push_back(item);
                break;
            }
            case 5: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->InvolvedIds.push_back(item);
                break;
            }
            case 6: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->CustomModelFolderIndexs.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerSpawnPointHC& value, ProtoWriter& writer) {
    if (value.has_x) {
        writer.WriteInt32(1, value.x);
    }
    if (value.has_y) {
        writer.WriteInt32(2, value.y);
    }
    if (value.has_z) {
        writer.WriteInt32(3, value.z);
    }
    if (value.has_Uin) {
        writer.WriteInt32(4, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerSpawnPointHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->x)) {
                    return false;
                }
                value->has_x = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->y)) {
                    return false;
                }
                value->has_y = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->z)) {
                    return false;
                }
                value->has_z = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CustomModelClassHC& value, ProtoWriter& writer) {
    for (const auto& item : value.Data) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CustomModelClassHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_CustomModelClassData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Data.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TransferOneRecordHC& value, ProtoWriter& writer) {
    if (value.has_DesID) {
        writer.WriteInt32(1, value.DesID);
    }
    if (value.has_TeamColor) {
        writer.WriteInt32(2, value.TeamColor);
    }
    if (value.has_PassItemID) {
        writer.WriteInt32(3, value.PassItemID);
    }
    if (value.has_PassItemNum) {
        writer.WriteInt32(4, value.PassItemNum);
    }
    if (value.has_ForbidItemID) {
        writer.WriteInt32(5, value.ForbidItemID);
    }
    if (value.has_IsExpendable) {
        writer.WriteBool(6, value.IsExpendable);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TransferOneRecordHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->DesID)) {
                    return false;
                }
                value->has_DesID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->TeamColor)) {
                    return false;
                }
                value->has_TeamColor = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->PassItemID)) {
                    return false;
                }
                value->has_PassItemID = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->PassItemNum)) {
                    return false;
                }
                value->has_PassItemNum = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->ForbidItemID)) {
                    return false;
                }
                value->has_ForbidItemID = true;
                break;
            }
            case 6: {
                if (!reader.ReadBool(&value->IsExpendable)) {
                    return false;
                }
                value->has_IsExpendable = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TransferRecordHC& value, ProtoWriter& writer) {
    if (value.has_SrcID) {
        writer.WriteInt32(1, value.SrcID);
    }
    if (value.has_IsEdit) {
        writer.WriteBool(2, value.IsEdit);
    }
    if (value.has_OneRecord) {
        ProtoWriter inner;
        Serialize(value.OneRecord, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TransferRecordHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SrcID)) {
                    return false;
                }
                value->has_SrcID = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->IsEdit)) {
                    return false;
                }
                value->has_IsEdit = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->OneRecord)) {
                    return false;
                }
                value->has_OneRecord = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TransferNameTipHC& value, ProtoWriter& writer) {
    if (value.has_SrcID) {
        writer.WriteInt32(1, value.SrcID);
    }
    if (value.has_TransferName) {
        writer.WriteString(2, value.TransferName);
    }
    if (value.has_TransferTip) {
        writer.WriteString(3, value.TransferTip);
    }
    if (value.has_ShowName) {
        writer.WriteBool(4, value.ShowName);
    }
    if (value.has_Status) {
        writer.WriteInt32(5, value.Status);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TransferNameTipHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SrcID)) {
                    return false;
                }
                value->has_SrcID = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->TransferName)) {
                    return false;
                }
                value->has_TransferName = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->TransferTip)) {
                    return false;
                }
                value->has_TransferTip = true;
                break;
            }
            case 4: {
                if (!reader.ReadBool(&value->ShowName)) {
                    return false;
                }
                value->has_ShowName = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Status)) {
                    return false;
                }
                value->has_Status = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TransferAddDelHC& value, ProtoWriter& writer) {
    if (value.has_AddDel) {
        writer.WriteBool(1, value.AddDel);
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    if (value.has_postion) {
        ProtoWriter inner;
        Serialize(value.postion, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_Status) {
        writer.WriteInt32(4, value.Status);
    }
    if (value.has_TransferID) {
        writer.WriteInt32(5, value.TransferID);
    }
    if (value.has_vehicleObj) {
        writer.WriteUInt64(6, value.vehicleObj);
    }
    if (value.has_postion_v) {
        ProtoWriter inner;
        Serialize(value.postion_v, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TransferAddDelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadBool(&value->AddDel)) {
                    return false;
                }
                value->has_AddDel = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->postion)) {
                    return false;
                }
                value->has_postion = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Status)) {
                    return false;
                }
                value->has_Status = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->TransferID)) {
                    return false;
                }
                value->has_TransferID = true;
                break;
            }
            case 6: {
                if (!reader.ReadUInt64(&value->vehicleObj)) {
                    return false;
                }
                value->has_vehicleObj = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->postion_v)) {
                    return false;
                }
                value->has_postion_v = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TransferDataHC& value, ProtoWriter& writer) {
    if (value.has_ID) {
        writer.WriteInt32(1, value.ID);
    }
    if (value.has_TransferName) {
        writer.WriteString(2, value.TransferName);
    }
    if (value.has_TransferTip) {
        writer.WriteString(3, value.TransferTip);
    }
    if (value.has_ShowName) {
        writer.WriteBool(4, value.ShowName);
    }
    if (value.has_MapID) {
        writer.WriteInt32(5, value.MapID);
    }
    if (value.has_postion) {
        ProtoWriter inner;
        Serialize(value.postion, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
    if (value.has_Status) {
        writer.WriteInt32(7, value.Status);
    }
    for (const auto& item : value.OneRecord) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(8, inner.Bytes());
    }
    if (value.has_vehicleObj) {
        writer.WriteUInt64(9, value.vehicleObj);
    }
    if (value.has_postion_v) {
        ProtoWriter inner;
        Serialize(value.postion_v, inner);
        writer.WriteMessage(10, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TransferDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->TransferName)) {
                    return false;
                }
                value->has_TransferName = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->TransferTip)) {
                    return false;
                }
                value->has_TransferTip = true;
                break;
            }
            case 4: {
                if (!reader.ReadBool(&value->ShowName)) {
                    return false;
                }
                value->has_ShowName = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->postion)) {
                    return false;
                }
                value->has_postion = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->Status)) {
                    return false;
                }
                value->has_Status = true;
                break;
            }
            case 8: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_TransferOneRecordHC item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->OneRecord.push_back(item);
                break;
            }
            case 9: {
                if (!reader.ReadUInt64(&value->vehicleObj)) {
                    return false;
                }
                value->has_vehicleObj = true;
                break;
            }
            case 10: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->postion_v)) {
                    return false;
                }
                value->has_postion_v = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TransferTargetHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_DesID) {
        writer.WriteInt32(2, value.DesID);
    }
    if (value.has_SrcID) {
        writer.WriteInt32(3, value.SrcID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TransferTargetHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->DesID)) {
                    return false;
                }
                value->has_DesID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->SrcID)) {
                    return false;
                }
                value->has_SrcID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OpenUIHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_Type) {
        writer.WriteInt32(2, value.Type);
    }
    if (value.has_ID) {
        writer.WriteInt32(3, value.ID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OpenUIHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->ID)) {
                    return false;
                }
                value->has_ID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RespNpcShopInfoHC& value, ProtoWriter& writer) {
    if (value.has_NpcShopInfo) {
        ProtoWriter inner;
        Serialize(value.NpcShopInfo, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RespNpcShopInfoHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->NpcShopInfo)) {
                    return false;
                }
                value->has_NpcShopInfo = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_NotifyNpcShopBuySkuHC& value, ProtoWriter& writer) {
    if (value.has_Ret) {
        writer.WriteInt32(1, value.Ret);
    }
    if (value.has_ShopID) {
        writer.WriteInt32(2, value.ShopID);
    }
    if (value.has_SkuID) {
        writer.WriteInt32(3, value.SkuID);
    }
    if (value.has_LeftNum) {
        writer.WriteInt32(4, value.LeftNum);
    }
    if (value.has_EndTime) {
        writer.WriteInt32(5, value.EndTime);
    }
    if (value.has_Uin) {
        writer.WriteInt32(6, value.Uin);
    }
    if (value.has_BuyCount) {
        writer.WriteInt32(7, value.BuyCount);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_NotifyNpcShopBuySkuHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Ret)) {
                    return false;
                }
                value->has_Ret = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ShopID)) {
                    return false;
                }
                value->has_ShopID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->SkuID)) {
                    return false;
                }
                value->has_SkuID = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->LeftNum)) {
                    return false;
                }
                value->has_LeftNum = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->EndTime)) {
                    return false;
                }
                value->has_EndTime = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->BuyCount)) {
                    return false;
                }
                value->has_BuyCount = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehiclePosDesc& value, ProtoWriter& writer) {
    if (value.has_Position) {
        ProtoWriter inner;
        Serialize(value.Position, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_RotateQuat) {
        ProtoWriter inner;
        Serialize(value.RotateQuat, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehiclePosDesc* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Position)) {
                    return false;
                }
                value->has_Position = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->RotateQuat)) {
                    return false;
                }
                value->has_RotateQuat = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleSTrustersPowerLevel& value, ProtoWriter& writer) {
    if (value.has_Position) {
        ProtoWriter inner;
        Serialize(value.Position, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_PowerLevel) {
        writer.WriteUInt32(2, value.PowerLevel);
    }
    if (value.has_CurPower) {
        writer.WriteInt64(3, value.CurPower);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleSTrustersPowerLevel* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Position)) {
                    return false;
                }
                value->has_Position = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->PowerLevel)) {
                    return false;
                }
                value->has_PowerLevel = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt64(&value->CurPower)) {
                    return false;
                }
                value->has_CurPower = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleMoveHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    for (const auto& item : value.ChassisPos) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    for (const auto& item : value.WheelPos) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    for (const auto& item : value.STrustersPower) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleMoveHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_VehiclePosDesc item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->ChassisPos.push_back(item);
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_VehiclePosDesc item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->WheelPos.push_back(item);
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_VehicleSTrustersPowerLevel item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->STrustersPower.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OpenEditActorModelHC& value, ProtoWriter& writer) {
    if (value.has_ContainerPos) {
        ProtoWriter inner;
        Serialize(value.ContainerPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OpenEditActorModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ContainerPos)) {
                    return false;
                }
                value->has_ContainerPos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloseEditActorModelHC& value, ProtoWriter& writer) {
    for (const auto& item : value.BoneModels) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    if (value.has_ContainerPos) {
        ProtoWriter inner;
        Serialize(value.ContainerPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_ModelType) {
        writer.WriteInt32(4, value.ModelType);
    }
    if (value.has_ModelMark) {
        writer.WriteString(5, value.ModelMark);
    }
    if (value.has_ModelName) {
        writer.WriteString(6, value.ModelName);
    }
    if (value.has_SkinDisplay) {
        writer.WriteBool(7, value.SkinDisplay);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CloseEditActorModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ActorOneBoneModelData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->BoneModels.push_back(item);
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ContainerPos)) {
                    return false;
                }
                value->has_ContainerPos = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->ModelType)) {
                    return false;
                }
                value->has_ModelType = true;
                break;
            }
            case 5: {
                if (!reader.ReadString(&value->ModelMark)) {
                    return false;
                }
                value->has_ModelMark = true;
                break;
            }
            case 6: {
                if (!reader.ReadString(&value->ModelName)) {
                    return false;
                }
                value->has_ModelName = true;
                break;
            }
            case 7: {
                if (!reader.ReadBool(&value->SkinDisplay)) {
                    return false;
                }
                value->has_SkinDisplay = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OneCustomActorModelDataHC& value, ProtoWriter& writer) {
    for (const auto& item : value.BoneModels) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_ModelMark) {
        writer.WriteString(2, value.ModelMark);
    }
    if (value.has_Type) {
        writer.WriteInt32(3, value.Type);
    }
    if (value.has_ModelName) {
        writer.WriteString(4, value.ModelName);
    }
    if (value.has_SkinDisplay) {
        writer.WriteBool(5, value.SkinDisplay);
    }
    if (value.has_AuthUin) {
        writer.WriteInt32(6, value.AuthUin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OneCustomActorModelDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ActorOneBoneModelData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->BoneModels.push_back(item);
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->ModelMark)) {
                    return false;
                }
                value->has_ModelMark = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->ModelName)) {
                    return false;
                }
                value->has_ModelName = true;
                break;
            }
            case 5: {
                if (!reader.ReadBool(&value->SkinDisplay)) {
                    return false;
                }
                value->has_SkinDisplay = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->AuthUin)) {
                    return false;
                }
                value->has_AuthUin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CustomActorModelDataHC& value, ProtoWriter& writer) {
    for (const auto& item : value.ModelDatas) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CustomActorModelDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_OneCustomActorModelDataHC item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->ModelDatas.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehiclePreBlockHC& value, ProtoWriter& writer) {
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    if (value.has_AttrInfo) {
        writer.WriteString(3, value.AttrInfo);
    }
    for (const auto& item : value.PreBlocks) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehiclePreBlockHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->AttrInfo)) {
                    return false;
                }
                value->has_AttrInfo = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_PreBlockData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->PreBlocks.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleItemIdHC& value, ProtoWriter& writer) {
    for (const auto& item : value.ItemID) {
        writer.WriteInt32(1, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleItemIdHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->ItemID.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAttribChangeHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_Fuel) {
        writer.WriteInt32(2, value.Fuel);
    }
    if (value.has_PartIndex) {
        writer.WriteInt32(3, value.PartIndex);
    }
    if (value.has_ActualSpeed) {
        writer.WriteInt32(4, value.ActualSpeed);
    }
    if (value.has_ShowSpeed) {
        writer.WriteInt32(5, value.ShowSpeed);
    }
    if (value.has_EngineRotationSpeed) {
        writer.WriteFloat(6, value.EngineRotationSpeed);
    }
    if (value.has_EngineState) {
        writer.WriteInt32(7, value.EngineState);
    }
    for (const auto& item : value.STrustersPower) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(8, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAttribChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Fuel)) {
                    return false;
                }
                value->has_Fuel = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->PartIndex)) {
                    return false;
                }
                value->has_PartIndex = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->ActualSpeed)) {
                    return false;
                }
                value->has_ActualSpeed = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->ShowSpeed)) {
                    return false;
                }
                value->has_ShowSpeed = true;
                break;
            }
            case 6: {
                if (!reader.ReadFloat(&value->EngineRotationSpeed)) {
                    return false;
                }
                value->has_EngineRotationSpeed = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->EngineState)) {
                    return false;
                }
                value->has_EngineState = true;
                break;
            }
            case 8: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_VehicleSTrustersPowerLevel item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->STrustersPower.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_WorkshopItemInfoHC& value, ProtoWriter& writer) {
    if (value.has_ItemID) {
        writer.WriteInt32(1, value.ItemID);
    }
    if (value.has_ItemName) {
        writer.WriteString(2, value.ItemName);
    }
    if (value.has_ItemDesc) {
        writer.WriteString(3, value.ItemDesc);
    }
    if (value.has_IsStart) {
        writer.WriteBool(4, value.IsStart);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_WorkshopItemInfoHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ItemID)) {
                    return false;
                }
                value->has_ItemID = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->ItemName)) {
                    return false;
                }
                value->has_ItemName = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->ItemDesc)) {
                    return false;
                }
                value->has_ItemDesc = true;
                break;
            }
            case 4: {
                if (!reader.ReadBool(&value->IsStart)) {
                    return false;
                }
                value->has_IsStart = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerCameraRotateHC& value, ProtoWriter& writer) {
    if (value.has_Pitch) {
        writer.WriteFloat(1, value.Pitch);
    }
    if (value.has_Yaw) {
        writer.WriteFloat(2, value.Yaw);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerCameraRotateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadFloat(&value->Pitch)) {
                    return false;
                }
                value->has_Pitch = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Yaw)) {
                    return false;
                }
                value->has_Yaw = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerChangeViewModeHC& value, ProtoWriter& writer) {
    if (value.has_ViewMode) {
        writer.WriteInt32(1, value.ViewMode);
    }
    if (value.has_Lock) {
        writer.WriteBool(2, value.Lock);
    }
    if (value.has_ClientMode) {
        writer.WriteBool(3, value.ClientMode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerChangeViewModeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ViewMode)) {
                    return false;
                }
                value->has_ViewMode = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->Lock)) {
                    return false;
                }
                value->has_Lock = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->ClientMode)) {
                    return false;
                }
                value->has_ClientMode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerCanMoveHC& value, ProtoWriter& writer) {
    if (value.has_CanMove) {
        writer.WriteBool(1, value.CanMove);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerCanMoveHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadBool(&value->CanMove)) {
                    return false;
                }
                value->has_CanMove = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerCanControlHC& value, ProtoWriter& writer) {
    if (value.has_CanControl) {
        writer.WriteBool(1, value.CanControl);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerCanControlHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadBool(&value->CanControl)) {
                    return false;
                }
                value->has_CanControl = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerSetAttrHC& value, ProtoWriter& writer) {
    if (value.has_AttrType) {
        writer.WriteInt32(1, value.AttrType);
    }
    if (value.has_Val) {
        writer.WriteFloat(2, value.Val);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerSetAttrHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->AttrType)) {
                    return false;
                }
                value->has_AttrType = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Val)) {
                    return false;
                }
                value->has_Val = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TriggerTimerDataHC& value, ProtoWriter& writer) {
    if (value.has_TimerID) {
        writer.WriteInt32(1, value.TimerID);
    }
    if (value.has_Time) {
        writer.WriteInt32(2, value.Time);
    }
    if (value.has_Type) {
        writer.WriteInt32(3, value.Type);
    }
    if (value.has_Title) {
        writer.WriteString(4, value.Title);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TriggerTimerDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->TimerID)) {
                    return false;
                }
                value->has_TimerID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Time)) {
                    return false;
                }
                value->has_Time = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Title)) {
                    return false;
                }
                value->has_Title = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerFreezingHC& value, ProtoWriter& writer) {
    if (value.has_freezingflag) {
        writer.WriteInt32(1, value.freezingflag);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerFreezingHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->freezingflag)) {
                    return false;
                }
                value->has_freezingflag = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_WorkshopBuildHC& value, ProtoWriter& writer) {
    if (value.has_Isbuild) {
        writer.WriteBool(1, value.Isbuild);
    }
    if (value.has_Mapid) {
        writer.WriteInt32(2, value.Mapid);
    }
    if (value.has_ContainerPos) {
        ProtoWriter inner;
        Serialize(value.ContainerPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_WorkshopBuildHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadBool(&value->Isbuild)) {
                    return false;
                }
                value->has_Isbuild = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Mapid)) {
                    return false;
                }
                value->has_Mapid = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ContainerPos)) {
                    return false;
                }
                value->has_ContainerPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GameRuleHC& value, ProtoWriter& writer) {
    if (value.has_ruleid) {
        writer.WriteInt32(1, value.ruleid);
    }
    if (value.has_optionid) {
        writer.WriteInt32(2, value.optionid);
    }
    if (value.has_value) {
        writer.WriteFloat(3, value.value);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GameRuleHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ruleid)) {
                    return false;
                }
                value->has_ruleid = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->optionid)) {
                    return false;
                }
                value->has_optionid = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->value)) {
                    return false;
                }
                value->has_value = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerScaleHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_Scale) {
        writer.WriteFloat(2, value.Scale);
    }
    if (value.has_ObjID) {
        writer.WriteUInt64(3, value.ObjID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerScaleHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Scale)) {
                    return false;
                }
                value->has_Scale = true;
                break;
            }
            case 3: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerNavigateHC& value, ProtoWriter& writer) {
    if (value.has_TargetPos) {
        ProtoWriter inner;
        Serialize(value.TargetPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_Speed) {
        writer.WriteFloat(2, value.Speed);
    }
    if (value.has_CanControl) {
        writer.WriteBool(3, value.CanControl);
    }
    if (value.has_showTip) {
        writer.WriteBool(4, value.showTip);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerNavigateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->TargetPos)) {
                    return false;
                }
                value->has_TargetPos = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Speed)) {
                    return false;
                }
                value->has_Speed = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->CanControl)) {
                    return false;
                }
                value->has_CanControl = true;
                break;
            }
            case 4: {
                if (!reader.ReadBool(&value->showTip)) {
                    return false;
                }
                value->has_showTip = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerCommonSetHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_Value) {
        writer.WriteFloat(2, value.Value);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerCommonSetHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadFloat(&value->Value)) {
                    return false;
                }
                value->has_Value = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OpenEditFullyCustomModelHC& value, ProtoWriter& writer) {
    if (value.has_ContainerPos) {
        ProtoWriter inner;
        Serialize(value.ContainerPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    if (value.has_Edited) {
        writer.WriteBool(3, value.Edited);
    }
    if (value.has_url) {
        writer.WriteString(4, value.url);
    }
    if (value.has_version) {
        writer.WriteInt32(5, value.version);
    }
    if (value.has_result) {
        writer.WriteInt32(6, value.result);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OpenEditFullyCustomModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ContainerPos)) {
                    return false;
                }
                value->has_ContainerPos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->Edited)) {
                    return false;
                }
                value->has_Edited = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->url)) {
                    return false;
                }
                value->has_url = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->version)) {
                    return false;
                }
                value->has_version = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->result)) {
                    return false;
                }
                value->has_result = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloseFullyCustomModelUIHC& value, ProtoWriter& writer) {
    if (value.has_Result) {
        writer.WriteInt32(1, value.Result);
    }
    if (value.has_MapID) {
        writer.WriteInt32(2, value.MapID);
    }
    if (value.has_ContainerPos) {
        ProtoWriter inner;
        Serialize(value.ContainerPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
    if (value.has_Skey) {
        writer.WriteString(4, value.Skey);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CloseFullyCustomModelUIHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Result)) {
                    return false;
                }
                value->has_Result = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MapID)) {
                    return false;
                }
                value->has_MapID = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->ContainerPos)) {
                    return false;
                }
                value->has_ContainerPos = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Skey)) {
                    return false;
                }
                value->has_Skey = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RespDownLoadResUrlHC& value, ProtoWriter& writer) {
    if (value.has_Type) {
        writer.WriteInt32(1, value.Type);
    }
    if (value.has_ExternData) {
        writer.WriteString(2, value.ExternData);
    }
    if (value.has_DownloadUrl) {
        writer.WriteString(3, value.DownloadUrl);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RespDownLoadResUrlHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Type)) {
                    return false;
                }
                value->has_Type = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->ExternData)) {
                    return false;
                }
                value->has_ExternData = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->DownloadUrl)) {
                    return false;
                }
                value->has_DownloadUrl = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PreOpenEditFCMUIHC& value, ProtoWriter& writer) {
    if (value.has_State) {
        writer.WriteInt32(1, value.State);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PreOpenEditFCMUIHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->State)) {
                    return false;
                }
                value->has_State = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAssembleLineHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_from) {
        writer.WriteInt32(2, value.from);
    }
    if (value.has_to) {
        writer.WriteInt32(3, value.to);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->from)) {
                    return false;
                }
                value->has_from = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->to)) {
                    return false;
                }
                value->has_to = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleAssembleLineOperateHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
    if (value.has_State) {
        writer.WriteBool(2, value.State);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleAssembleLineOperateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->State)) {
                    return false;
                }
                value->has_State = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VehicleBindActorHC& value, ProtoWriter& writer) {
    if (value.has_vehicleObjID) {
        writer.WriteUInt64(1, value.vehicleObjID);
    }
    if (value.has_bindObjID) {
        writer.WriteUInt64(2, value.bindObjID);
    }
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(3, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VehicleBindActorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->vehicleObjID)) {
                    return false;
                }
                value->has_vehicleObjID = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt64(&value->bindObjID)) {
                    return false;
                }
                value->has_bindObjID = true;
                break;
            }
            case 3: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TriggerMusicHC& value, ProtoWriter& writer) {
    if (value.has_objid) {
        writer.WriteInt32(1, value.objid);
    }
    if (value.has_Name) {
        writer.WriteString(2, value.Name);
    }
    if (value.has_Volume) {
        writer.WriteFloat(3, value.Volume);
    }
    if (value.has_Pitch) {
        writer.WriteFloat(4, value.Pitch);
    }
    if (value.has_IsLoop) {
        writer.WriteBool(5, value.IsLoop);
    }
    if (value.has_PlayState) {
        writer.WriteInt32(6, value.PlayState);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TriggerMusicHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->objid)) {
                    return false;
                }
                value->has_objid = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 3: {
                if (!reader.ReadFloat(&value->Volume)) {
                    return false;
                }
                value->has_Volume = true;
                break;
            }
            case 4: {
                if (!reader.ReadFloat(&value->Pitch)) {
                    return false;
                }
                value->has_Pitch = true;
                break;
            }
            case 5: {
                if (!reader.ReadBool(&value->IsLoop)) {
                    return false;
                }
                value->has_IsLoop = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->PlayState)) {
                    return false;
                }
                value->has_PlayState = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CSPlayerPermitHC& value, ProtoWriter& writer) {
    if (value.has_TargetUin) {
        writer.WriteInt32(1, value.TargetUin);
    }
    if (value.has_Flags) {
        writer.WriteUInt32(2, value.Flags);
    }
    for (const auto& item : value.BanItems) {
        writer.WriteInt32(3, item);
    }
    if (value.has_DataType) {
        writer.WriteInt32(4, value.DataType);
    }
    for (const auto& item : value.Permits) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(5, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CSPlayerPermitHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->TargetUin)) {
                    return false;
                }
                value->has_TargetUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadUInt32(&value->Flags)) {
                    return false;
                }
                value->has_Flags = true;
                break;
            }
            case 3: {
                int32_t item{};
                if (!reader.ReadInt32(&item)) {
                    return false;
                }
                value->BanItems.push_back(item);
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->DataType)) {
                    return false;
                }
                value->has_DataType = true;
                break;
            }
            case 5: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_CSPermitData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Permits.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CSAuthorityHC& value, ProtoWriter& writer) {
    if (value.has_DataType) {
        writer.WriteInt32(1, value.DataType);
    }
    for (const auto& item : value.Authorities) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CSAuthorityHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->DataType)) {
                    return false;
                }
                value->has_DataType = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_CSAuthorityData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Authorities.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SSTaskHC& value, ProtoWriter& writer) {
    if (value.has_TargetUin) {
        writer.WriteInt32(1, value.TargetUin);
    }
    if (value.has_TaskId) {
        writer.WriteInt32(2, value.TaskId);
    }
    if (value.has_ParamJson) {
        writer.WriteString(3, value.ParamJson);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SSTaskHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->TargetUin)) {
                    return false;
                }
                value->has_TargetUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->TaskId)) {
                    return false;
                }
                value->has_TaskId = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->ParamJson)) {
                    return false;
                }
                value->has_ParamJson = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloudServerChangeHC& value, ProtoWriter& writer) {
    if (value.has_type) {
        writer.WriteInt32(1, value.type);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CloudServerChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->type)) {
                    return false;
                }
                value->has_type = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_TriggerOpenStoreHC& value, ProtoWriter& writer) {
    if (value.has_ObjID) {
        writer.WriteUInt64(1, value.ObjID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_TriggerOpenStoreHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_UsePackingFcmItemHC& value, ProtoWriter& writer) {
    if (value.has_result) {
        writer.WriteInt32(1, value.result);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_UsePackingFcmItemHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->result)) {
                    return false;
                }
                value->has_result = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CreatePackingCMHC& value, ProtoWriter& writer) {
    if (value.has_result) {
        writer.WriteInt32(1, value.result);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CreatePackingCMHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->result)) {
                    return false;
                }
                value->has_result = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OnePackingCMDataHC& value, ProtoWriter& writer) {
    if (value.has_PackingPos) {
        ProtoWriter inner;
        Serialize(value.PackingPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_Quat) {
        ProtoWriter inner;
        Serialize(value.Quat, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
    if (value.has_Name) {
        writer.WriteString(3, value.Name);
    }
    if (value.has_Model) {
        writer.WriteString(4, value.Model);
    }
    if (value.has_Dir) {
        writer.WriteInt32(5, value.Dir);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OnePackingCMDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->PackingPos)) {
                    return false;
                }
                value->has_PackingPos = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Quat)) {
                    return false;
                }
                value->has_Quat = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->Model)) {
                    return false;
                }
                value->has_Model = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Dir)) {
                    return false;
                }
                value->has_Dir = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OnePackingFCMDataHC& value, ProtoWriter& writer) {
    if (value.has_Name) {
        writer.WriteString(1, value.Name);
    }
    if (value.has_Desc) {
        writer.WriteString(2, value.Desc);
    }
    if (value.has_SKey) {
        writer.WriteString(3, value.SKey);
    }
    for (const auto& item : value.PackingCMs) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(4, inner.Bytes());
    }
    if (value.has_Dir) {
        writer.WriteInt32(5, value.Dir);
    }
    if (value.has_MinPos) {
        ProtoWriter inner;
        Serialize(value.MinPos, inner);
        writer.WriteMessage(6, inner.Bytes());
    }
    if (value.has_MaxPos) {
        ProtoWriter inner;
        Serialize(value.MaxPos, inner);
        writer.WriteMessage(7, inner.Bytes());
    }
    if (value.has_AuthUin) {
        writer.WriteInt32(8, value.AuthUin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OnePackingFCMDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->Name)) {
                    return false;
                }
                value->has_Name = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->Desc)) {
                    return false;
                }
                value->has_Desc = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->SKey)) {
                    return false;
                }
                value->has_SKey = true;
                break;
            }
            case 4: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_OnePackingCMDataHC item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->PackingCMs.push_back(item);
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Dir)) {
                    return false;
                }
                value->has_Dir = true;
                break;
            }
            case 6: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->MinPos)) {
                    return false;
                }
                value->has_MinPos = true;
                break;
            }
            case 7: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->MaxPos)) {
                    return false;
                }
                value->has_MaxPos = true;
                break;
            }
            case 8: {
                if (!reader.ReadInt32(&value->AuthUin)) {
                    return false;
                }
                value->has_AuthUin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PackingFCMDataHC& value, ProtoWriter& writer) {
    for (const auto& item : value.PackingFCMs) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PackingFCMDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_OnePackingFCMDataHC item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->PackingFCMs.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CloudRoomStatusTimeHC& value, ProtoWriter& writer) {
    if (value.has_Status) {
        writer.WriteInt32(1, value.Status);
    }
    if (value.has_Time) {
        writer.WriteInt32(2, value.Time);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CloudRoomStatusTimeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Status)) {
                    return false;
                }
                value->has_Status = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Time)) {
                    return false;
                }
                value->has_Time = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SensorContainerDataHC& value, ProtoWriter& writer) {
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_SensorValue) {
        writer.WriteInt32(2, value.SensorValue);
    }
    if (value.has_IsBreverse) {
        writer.WriteBool(3, value.IsBreverse);
    }
    if (value.has_ObjID) {
        writer.WriteUInt64(4, value.ObjID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SensorContainerDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->SensorValue)) {
                    return false;
                }
                value->has_SensorValue = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->IsBreverse)) {
                    return false;
                }
                value->has_IsBreverse = true;
                break;
            }
            case 4: {
                if (!reader.ReadUInt64(&value->ObjID)) {
                    return false;
                }
                value->has_ObjID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_DoorDataHC& value, ProtoWriter& writer) {
    if (value.has_BlockPos) {
        ProtoWriter inner;
        Serialize(value.BlockPos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_DoorDataHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->BlockPos)) {
                    return false;
                }
                value->has_BlockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerCarryActorHC& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_PlayerUIN) {
        writer.WriteInt32(2, value.PlayerUIN);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerCarryActorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->PlayerUIN)) {
                    return false;
                }
                value->has_PlayerUIN = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VillagerBodyChangeHC& value, ProtoWriter& writer) {
    if (value.has_ObjId) {
        writer.WriteUInt64(1, value.ObjId);
    }
    if (value.has_ChangeType) {
        writer.WriteInt32(2, value.ChangeType);
    }
    if (value.has_ChangeValue) {
        writer.WriteInt64(3, value.ChangeValue);
    }
    if (value.has_OtherValue) {
        writer.WriteString(4, value.OtherValue);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VillagerBodyChangeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ObjId)) {
                    return false;
                }
                value->has_ObjId = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ChangeType)) {
                    return false;
                }
                value->has_ChangeType = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt64(&value->ChangeValue)) {
                    return false;
                }
                value->has_ChangeValue = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->OtherValue)) {
                    return false;
                }
                value->has_OtherValue = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerTameActorHC& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_PlayerUIN) {
        writer.WriteInt32(2, value.PlayerUIN);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerTameActorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->PlayerUIN)) {
                    return false;
                }
                value->has_PlayerUIN = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VillagerCloth& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_bshow) {
        writer.WriteBool(2, value.bshow);
    }
    if (value.has_modlename) {
        writer.WriteString(3, value.modlename);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VillagerCloth* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->bshow)) {
                    return false;
                }
                value->has_bshow = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->modlename)) {
                    return false;
                }
                value->has_modlename = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorHeadDisplayIconHC& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_ItemID) {
        writer.WriteInt32(2, value.ItemID);
    }
    if (value.has_Tick) {
        writer.WriteInt32(3, value.Tick);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorHeadDisplayIconHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ItemID)) {
                    return false;
                }
                value->has_ItemID = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->Tick)) {
                    return false;
                }
                value->has_Tick = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActorPlayAnimByIdHC& value, ProtoWriter& writer) {
    if (value.has_ActorID) {
        writer.WriteUInt64(1, value.ActorID);
    }
    if (value.has_AnimID) {
        writer.WriteInt32(2, value.AnimID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActorPlayAnimByIdHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt64(&value->ActorID)) {
                    return false;
                }
                value->has_ActorID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->AnimID)) {
                    return false;
                }
                value->has_AnimID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VillageTotemTipHC& value, ProtoWriter& writer) {
    if (value.has_villagerNum) {
        writer.WriteInt32(1, value.villagerNum);
    }
    if (value.has_blockPos) {
        ProtoWriter inner;
        Serialize(value.blockPos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VillageTotemTipHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->villagerNum)) {
                    return false;
                }
                value->has_villagerNum = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockPos)) {
                    return false;
                }
                value->has_blockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_VillageTotemActiveHC& value, ProtoWriter& writer) {
    if (value.has_uin) {
        writer.WriteInt32(1, value.uin);
    }
    if (value.has_blockPos) {
        ProtoWriter inner;
        Serialize(value.blockPos, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_VillageTotemActiveHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->uin)) {
                    return false;
                }
                value->has_uin = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->blockPos)) {
                    return false;
                }
                value->has_blockPos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SaveTombStoneHC& value, ProtoWriter& writer) {
    if (value.has_Point) {
        ProtoWriter inner;
        Serialize(value.Point, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
    if (value.has_title) {
        writer.WriteString(2, value.title);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SaveTombStoneHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->Point)) {
                    return false;
                }
                value->has_Point = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->title)) {
                    return false;
                }
                value->has_title = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerLevelModeHC& value, ProtoWriter& writer) {
    if (value.has_SumExp) {
        writer.WriteInt32(1, value.SumExp);
    }
    if (value.has_CurLevel) {
        writer.WriteInt32(2, value.CurLevel);
    }
    if (value.has_CurExp) {
        writer.WriteInt32(3, value.CurExp);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerLevelModeHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SumExp)) {
                    return false;
                }
                value->has_SumExp = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->CurLevel)) {
                    return false;
                }
                value->has_CurLevel = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->CurExp)) {
                    return false;
                }
                value->has_CurExp = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ActionAttrStateHC& value, ProtoWriter& writer) {
    if (value.has_Attr) {
        writer.WriteUInt32(1, value.Attr);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ActionAttrStateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadUInt32(&value->Attr)) {
                    return false;
                }
                value->has_Attr = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Edu_RolesInfoHC& value, ProtoWriter& writer) {
    for (const auto& item : value.rolesInfo) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Edu_RolesInfoHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_Edu_RoleInfo item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->rolesInfo.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ImportModelHC& value, ProtoWriter& writer) {
    for (const auto& item : value.models) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ImportModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ImportModelData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->models.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_LightningHC& value, ProtoWriter& writer) {
    if (value.has_targetpos) {
        ProtoWriter inner;
        Serialize(value.targetpos, inner);
        writer.WriteMessage(1, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_LightningHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &value->targetpos)) {
                    return false;
                }
                value->has_targetpos = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_InteractMobPackHC& value, ProtoWriter& writer) {
    if (value.has_uiName) {
        writer.WriteString(1, value.uiName);
    }
    if (value.has_param) {
        writer.WriteString(2, value.param);
    }
    if (value.has_mobID) {
        writer.WriteInt64(3, value.mobID);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_InteractMobPackHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->uiName)) {
                    return false;
                }
                value->has_uiName = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->param)) {
                    return false;
                }
                value->has_param = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt64(&value->mobID)) {
                    return false;
                }
                value->has_mobID = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_UpdateMobBackpackHC& value, ProtoWriter& writer) {
    if (value.has_mobID) {
        writer.WriteInt64(1, value.mobID);
    }
    for (const auto& item : value.ItemInfo) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_UpdateMobBackpackHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt64(&value->mobID)) {
                    return false;
                }
                value->has_mobID = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_ItemData item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->ItemInfo.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerTransformSkinHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_playerindex) {
        writer.WriteInt32(2, value.playerindex);
    }
    if (value.has_customskins) {
        writer.WriteString(3, value.customskins);
    }
    if (value.has_Reason) {
        writer.WriteInt32(4, value.Reason);
    }
    if (value.has_MainPlayerId) {
        writer.WriteUInt64(5, value.MainPlayerId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerTransformSkinHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->playerindex)) {
                    return false;
                }
                value->has_playerindex = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->customskins)) {
                    return false;
                }
                value->has_customskins = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->Reason)) {
                    return false;
                }
                value->has_Reason = true;
                break;
            }
            case 5: {
                if (!reader.ReadUInt64(&value->MainPlayerId)) {
                    return false;
                }
                value->has_MainPlayerId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayerSaveArchHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_playTime) {
        writer.WriteInt32(2, value.playTime);
    }
    if (value.has_showTip) {
        writer.WriteBool(3, value.showTip);
    }
    if (value.has_userdata) {
        writer.WriteString(4, value.userdata);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayerSaveArchHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->playTime)) {
                    return false;
                }
                value->has_playTime = true;
                break;
            }
            case 3: {
                if (!reader.ReadBool(&value->showTip)) {
                    return false;
                }
                value->has_showTip = true;
                break;
            }
            case 4: {
                if (!reader.ReadString(&value->userdata)) {
                    return false;
                }
                value->has_userdata = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PrayTreeStageHC& value, ProtoWriter& writer) {
    if (value.has_stage) {
        writer.WriteInt32(1, value.stage);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PrayTreeStageHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->stage)) {
                    return false;
                }
                value->has_stage = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PrayTreeReqHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_stage) {
        writer.WriteInt32(2, value.stage);
    }
    if (value.has_treeId) {
        writer.WriteString(3, value.treeId);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PrayTreeReqHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->stage)) {
                    return false;
                }
                value->has_stage = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->treeId)) {
                    return false;
                }
                value->has_treeId = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PrayTreeInfoHC& value, ProtoWriter& writer) {
    if (value.has_stage) {
        writer.WriteInt32(1, value.stage);
    }
    if (value.has_treeId) {
        writer.WriteString(2, value.treeId);
    }
    if (value.has_hostuin) {
        writer.WriteInt32(3, value.hostuin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PrayTreeInfoHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->stage)) {
                    return false;
                }
                value->has_stage = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->treeId)) {
                    return false;
                }
                value->has_treeId = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->hostuin)) {
                    return false;
                }
                value->has_hostuin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PrayTreeTimeUpdateHC& value, ProtoWriter& writer) {
    if (value.has_stage) {
        writer.WriteInt32(1, value.stage);
    }
    if (value.has_treeTime) {
        writer.WriteString(2, value.treeTime);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PrayTreeTimeUpdateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->stage)) {
                    return false;
                }
                value->has_stage = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->treeTime)) {
                    return false;
                }
                value->has_treeTime = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_HomeNpcOpenHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_npcType) {
        writer.WriteInt32(2, value.npcType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_HomeNpcOpenHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->npcType)) {
                    return false;
                }
                value->has_npcType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PrayErrorHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_errorType) {
        writer.WriteInt32(2, value.errorType);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PrayErrorHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->errorType)) {
                    return false;
                }
                value->has_errorType = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_Open_HomeCloset_HC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_skinIDs) {
        writer.WriteString(2, value.skinIDs);
    }
    if (value.has_skinPartIDs) {
        writer.WriteString(3, value.skinPartIDs);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_Open_HomeCloset_HC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->skinIDs)) {
                    return false;
                }
                value->has_skinIDs = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->skinPartIDs)) {
                    return false;
                }
                value->has_skinPartIDs = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OpenDevGoodsBuyDialogHC& value, ProtoWriter& writer) {
    if (value.has_itemid) {
        writer.WriteInt32(1, value.itemid);
    }
    if (value.has_desc) {
        writer.WriteString(2, value.desc);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OpenDevGoodsBuyDialogHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->itemid)) {
                    return false;
                }
                value->has_itemid = true;
                break;
            }
            case 2: {
                if (!reader.ReadString(&value->desc)) {
                    return false;
                }
                value->has_desc = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_PlayGraphicsHC& value, ProtoWriter& writer) {
    if (value.has_Operateid) {
        writer.WriteInt32(1, value.Operateid);
    }
    for (const auto& item : value.graphicsinfo) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_PlayGraphicsHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Operateid)) {
                    return false;
                }
                value->has_Operateid = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_GraphicsAttr item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->graphicsinfo.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GodTempleCreateHC& value, ProtoWriter& writer) {
    if (value.has_onoff) {
        writer.WriteBool(1, value.onoff);
    }
    if (value.has_worldid) {
        writer.WriteInt32(2, value.worldid);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GodTempleCreateHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadBool(&value->onoff)) {
                    return false;
                }
                value->has_onoff = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->worldid)) {
                    return false;
                }
                value->has_worldid = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SFActivity_HC& value, ProtoWriter& writer) {
    if (value.has_type) {
        writer.WriteInt32(1, value.type);
    }
    if (value.has_taskId) {
        writer.WriteInt32(2, value.taskId);
    }
    if (value.has_value) {
        writer.WriteInt32(3, value.value);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SFActivity_HC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->type)) {
                    return false;
                }
                value->has_type = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->taskId)) {
                    return false;
                }
                value->has_taskId = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->value)) {
                    return false;
                }
                value->has_value = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ShapeAdditionAnimHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_status) {
        writer.WriteBool(2, value.status);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ShapeAdditionAnimHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadBool(&value->status)) {
                    return false;
                }
                value->has_status = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_OneHomelandRanchAnimal& value, ProtoWriter& writer) {
    if (value.has_SeedID) {
        writer.WriteInt32(1, value.SeedID);
    }
    if (value.has_GrowthTime) {
        writer.WriteInt32(2, value.GrowthTime);
    }
    if (value.has_FeedTime) {
        writer.WriteInt32(3, value.FeedTime);
    }
    if (value.has_HungerTime) {
        writer.WriteInt32(4, value.HungerTime);
    }
    if (value.has_Stage) {
        writer.WriteInt32(5, value.Stage);
    }
    if (value.has_HungerFlag) {
        writer.WriteBool(6, value.HungerFlag);
    }
    if (value.has_Serverid) {
        writer.WriteString(7, value.Serverid);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_OneHomelandRanchAnimal* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->SeedID)) {
                    return false;
                }
                value->has_SeedID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->GrowthTime)) {
                    return false;
                }
                value->has_GrowthTime = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->FeedTime)) {
                    return false;
                }
                value->has_FeedTime = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->HungerTime)) {
                    return false;
                }
                value->has_HungerTime = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Stage)) {
                    return false;
                }
                value->has_Stage = true;
                break;
            }
            case 6: {
                if (!reader.ReadBool(&value->HungerFlag)) {
                    return false;
                }
                value->has_HungerFlag = true;
                break;
            }
            case 7: {
                if (!reader.ReadString(&value->Serverid)) {
                    return false;
                }
                value->has_Serverid = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_HomelandRanchInfoHC& value, ProtoWriter& writer) {
    if (value.has_full) {
        writer.WriteBool(1, value.full);
    }
    for (const auto& item : value.Animals) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_HomelandRanchInfoHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadBool(&value->full)) {
                    return false;
                }
                value->has_full = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_OneHomelandRanchAnimal item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->Animals.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_UseItemByHomelandHC& value, ProtoWriter& writer) {
    if (value.has_ItemID) {
        writer.WriteInt32(1, value.ItemID);
    }
    if (value.has_ItemNum) {
        writer.WriteInt32(2, value.ItemNum);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_UseItemByHomelandHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ItemID)) {
                    return false;
                }
                value->has_ItemID = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->ItemNum)) {
                    return false;
                }
                value->has_ItemNum = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_CustomBaseModelHC& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
    if (value.has_TeamId) {
        writer.WriteInt32(2, value.TeamId);
    }
    if (value.has_ModelData) {
        writer.WriteString(3, value.ModelData);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_CustomBaseModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->TeamId)) {
                    return false;
                }
                value->has_TeamId = true;
                break;
            }
            case 3: {
                if (!reader.ReadString(&value->ModelData)) {
                    return false;
                }
                value->has_ModelData = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_ChangeActorModelHC& value, ProtoWriter& writer) {
    for (const auto& item : value.ObjID) {
        writer.WriteUInt64(1, item);
    }
    for (const auto& item : value.modelID) {
        writer.WriteString(2, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_ChangeActorModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                uint64_t item{};
                if (!reader.ReadUInt64(&item)) {
                    return false;
                }
                value->ObjID.push_back(item);
                break;
            }
            case 2: {
                std::string item{};
                if (!reader.ReadString(&item)) {
                    return false;
                }
                value->modelID.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_NotifiyActorModelHC& value, ProtoWriter& writer) {
    for (const auto& item : value.modelID) {
        writer.WriteString(1, item);
    }
    for (const auto& item : value.modelData) {
        writer.WriteString(2, item);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_NotifiyActorModelHC* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                std::string item{};
                if (!reader.ReadString(&item)) {
                    return false;
                }
                value->modelID.push_back(item);
                break;
            }
            case 2: {
                std::string item{};
                if (!reader.ReadString(&item)) {
                    return false;
                }
                value->modelData.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_RoomInfo& value, ProtoWriter& writer) {
    if (value.has_OwnerUin) {
        writer.WriteInt32(1, value.OwnerUin);
    }
    if (value.has_PlayerCount) {
        writer.WriteInt32(2, value.PlayerCount);
    }
    if (value.has_MaxPlayerCount) {
        writer.WriteInt32(3, value.MaxPlayerCount);
    }
    if (value.has_GameType) {
        writer.WriteInt32(4, value.GameType);
    }
    if (value.has_GpsLatitude) {
        writer.WriteDouble(5, value.GpsLatitude);
    }
    if (value.has_GpsLongtitude) {
        writer.WriteDouble(6, value.GpsLongtitude);
    }
    if (value.has_Timestamp) {
        writer.WriteInt64(7, value.Timestamp);
    }
    if (value.has_RoomTitle) {
        writer.WriteString(8, value.RoomTitle);
    }
    if (value.has_OwnerName) {
        writer.WriteString(9, value.OwnerName);
    }
    if (value.has_Password) {
        writer.WriteString(10, value.Password);
    }
    if (value.has_Description) {
        writer.WriteString(11, value.Description);
    }
    if (value.has_ExtraData) {
        writer.WriteString(12, value.ExtraData);
    }
    if (value.has_LastPing) {
        writer.WriteInt32(13, value.LastPing);
    }
    if (value.has_Province) {
        writer.WriteInt32(14, value.Province);
    }
    if (value.has_Isp) {
        writer.WriteInt32(15, value.Isp);
    }
    if (value.has_GameLabel) {
        writer.WriteInt32(16, value.GameLabel);
    }
    if (value.has_OWID) {
        writer.WriteInt64(17, value.OWID);
    }
    if (value.has_RegionIp) {
        writer.WriteInt32(18, value.RegionIp);
    }
    if (value.has_ApiID) {
        writer.WriteInt32(19, value.ApiID);
    }
    if (value.has_RoomIP) {
        writer.WriteString(20, value.RoomIP);
    }
    if (value.has_RoomPort) {
        writer.WriteInt32(21, value.RoomPort);
    }
    if (value.has_GameEnv) {
        writer.WriteInt32(22, value.GameEnv);
    }
    if (value.has_MapType) {
        writer.WriteString(23, value.MapType);
    }
    if (value.has_ConnectMode) {
        writer.WriteInt32(24, value.ConnectMode);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_RoomInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->OwnerUin)) {
                    return false;
                }
                value->has_OwnerUin = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->PlayerCount)) {
                    return false;
                }
                value->has_PlayerCount = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->MaxPlayerCount)) {
                    return false;
                }
                value->has_MaxPlayerCount = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->GameType)) {
                    return false;
                }
                value->has_GameType = true;
                break;
            }
            case 5: {
                if (!reader.ReadDouble(&value->GpsLatitude)) {
                    return false;
                }
                value->has_GpsLatitude = true;
                break;
            }
            case 6: {
                if (!reader.ReadDouble(&value->GpsLongtitude)) {
                    return false;
                }
                value->has_GpsLongtitude = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt64(&value->Timestamp)) {
                    return false;
                }
                value->has_Timestamp = true;
                break;
            }
            case 8: {
                if (!reader.ReadString(&value->RoomTitle)) {
                    return false;
                }
                value->has_RoomTitle = true;
                break;
            }
            case 9: {
                if (!reader.ReadString(&value->OwnerName)) {
                    return false;
                }
                value->has_OwnerName = true;
                break;
            }
            case 10: {
                if (!reader.ReadString(&value->Password)) {
                    return false;
                }
                value->has_Password = true;
                break;
            }
            case 11: {
                if (!reader.ReadString(&value->Description)) {
                    return false;
                }
                value->has_Description = true;
                break;
            }
            case 12: {
                if (!reader.ReadString(&value->ExtraData)) {
                    return false;
                }
                value->has_ExtraData = true;
                break;
            }
            case 13: {
                if (!reader.ReadInt32(&value->LastPing)) {
                    return false;
                }
                value->has_LastPing = true;
                break;
            }
            case 14: {
                if (!reader.ReadInt32(&value->Province)) {
                    return false;
                }
                value->has_Province = true;
                break;
            }
            case 15: {
                if (!reader.ReadInt32(&value->Isp)) {
                    return false;
                }
                value->has_Isp = true;
                break;
            }
            case 16: {
                if (!reader.ReadInt32(&value->GameLabel)) {
                    return false;
                }
                value->has_GameLabel = true;
                break;
            }
            case 17: {
                if (!reader.ReadInt64(&value->OWID)) {
                    return false;
                }
                value->has_OWID = true;
                break;
            }
            case 18: {
                if (!reader.ReadInt32(&value->RegionIp)) {
                    return false;
                }
                value->has_RegionIp = true;
                break;
            }
            case 19: {
                if (!reader.ReadInt32(&value->ApiID)) {
                    return false;
                }
                value->has_ApiID = true;
                break;
            }
            case 20: {
                if (!reader.ReadString(&value->RoomIP)) {
                    return false;
                }
                value->has_RoomIP = true;
                break;
            }
            case 21: {
                if (!reader.ReadInt32(&value->RoomPort)) {
                    return false;
                }
                value->has_RoomPort = true;
                break;
            }
            case 22: {
                if (!reader.ReadInt32(&value->GameEnv)) {
                    return false;
                }
                value->has_GameEnv = true;
                break;
            }
            case 23: {
                if (!reader.ReadString(&value->MapType)) {
                    return false;
                }
                value->has_MapType = true;
                break;
            }
            case 24: {
                if (!reader.ReadInt32(&value->ConnectMode)) {
                    return false;
                }
                value->has_ConnectMode = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_LoginRoomServerReq& value, ProtoWriter& writer) {
    if (value.has_Uin) {
        writer.WriteInt32(1, value.Uin);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_LoginRoomServerReq* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Uin)) {
                    return false;
                }
                value->has_Uin = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_SvrUrlInfo& value, ProtoWriter& writer) {
    if (value.has_Url) {
        writer.WriteString(1, value.Url);
    }
    if (value.has_Port) {
        writer.WriteInt32(2, value.Port);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_SvrUrlInfo* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadString(&value->Url)) {
                    return false;
                }
                value->has_Url = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->Port)) {
                    return false;
                }
                value->has_Port = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_LoginRoomServerRes& value, ProtoWriter& writer) {
    if (value.has_ResultCode) {
        writer.WriteInt32(1, value.ResultCode);
    }
    if (value.has_MaxVersion) {
        writer.WriteInt32(2, value.MaxVersion);
    }
    if (value.has_MinVersion) {
        writer.WriteInt32(3, value.MinVersion);
    }
    if (value.has_LatestVersion) {
        writer.WriteInt32(4, value.LatestVersion);
    }
    if (value.has_Province) {
        writer.WriteInt32(5, value.Province);
    }
    if (value.has_Isp) {
        writer.WriteInt32(6, value.Isp);
    }
    if (value.has_UseProxy) {
        writer.WriteInt32(7, value.UseProxy);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_LoginRoomServerRes* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ResultCode)) {
                    return false;
                }
                value->has_ResultCode = true;
                break;
            }
            case 2: {
                if (!reader.ReadInt32(&value->MaxVersion)) {
                    return false;
                }
                value->has_MaxVersion = true;
                break;
            }
            case 3: {
                if (!reader.ReadInt32(&value->MinVersion)) {
                    return false;
                }
                value->has_MinVersion = true;
                break;
            }
            case 4: {
                if (!reader.ReadInt32(&value->LatestVersion)) {
                    return false;
                }
                value->has_LatestVersion = true;
                break;
            }
            case 5: {
                if (!reader.ReadInt32(&value->Province)) {
                    return false;
                }
                value->has_Province = true;
                break;
            }
            case 6: {
                if (!reader.ReadInt32(&value->Isp)) {
                    return false;
                }
                value->has_Isp = true;
                break;
            }
            case 7: {
                if (!reader.ReadInt32(&value->UseProxy)) {
                    return false;
                }
                value->has_UseProxy = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_GetRoomsInfoRes& value, ProtoWriter& writer) {
    if (value.has_ResultCode) {
        writer.WriteInt32(1, value.ResultCode);
    }
    for (const auto& item : value.RoomInfoList) {
        ProtoWriter inner;
        Serialize(item, inner);
        writer.WriteMessage(2, inner.Bytes());
    }
}

inline bool Deserialize(ProtoReader& reader, PB_GetRoomsInfoRes* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->ResultCode)) {
                    return false;
                }
                value->has_ResultCode = true;
                break;
            }
            case 2: {
                std::vector<uint8_t> nested;
                if (!reader.ReadMessage(&nested)) {
                    return false;
                }
                PB_RoomInfo item{};
                ProtoReader inner(nested.data(), nested.size());
                if (!Deserialize(inner, &item)) {
                    return false;
                }
                value->RoomInfoList.push_back(item);
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

inline void Serialize(const PB_JoinRoomRes& value, ProtoWriter& writer) {
    if (value.has_Result) {
        writer.WriteInt32(1, value.Result);
    }
}

inline bool Deserialize(ProtoReader& reader, PB_JoinRoomRes* value) {
    if (value == nullptr) {
        return false;
    }
    while (!reader.Done()) {
        uint32_t field = 0;
        WireType type = WireType::kVarint;
        if (!reader.ReadTag(&field, &type)) {
            return false;
        }
        switch (field) {
            case 1: {
                if (!reader.ReadInt32(&value->Result)) {
                    return false;
                }
                value->has_Result = true;
                break;
            }
            default:
                if (!reader.Skip(type)) {
                    return false;
                }
                break;
        }
    }
    return true;
}

}  /* namespace immirror::net::gen */
