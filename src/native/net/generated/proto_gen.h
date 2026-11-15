#pragma once

#include <cstdint>
#include <string>
#include <vector>

/*
 * 由 scripts/gen_proto_cpp.py 从逆向工程的 4 个 proto 文件生成, 请勿手改.
 * 只含字段声明与 has_ 标志; 按依赖拓扑排序, 无前向引用.
 * 序列化在后续阶段实现.
 */
namespace immirror::net::gen {

enum class ePBActorTypes : int32_t {
    kPB_ACTORTYPEGENERAL = 0,
    kPB_ACTORTYPEROLE = 1,
    kPB_ACTORTYPEMONSTER = 2,
    kPB_ACTORTYPEBOSS = 3,
    kPB_ACTORTYPEBLOCK = 4,
    kPB_ACTORTYPEITEM = 5,
};

enum class ePBEffectType : int32_t {
    kPB_EFFECT_PARTICLE = 0,
    kPB_EFFECT_PICKITEM = 1,
    kPB_EFFECT_SOUND = 2,
    kPB_EFFECT_ACTORBODY = 3,
    kPB_EFFECT_DESTROYBLOCK = 4,
    kPB_EFFECT_PLAYMUSICGRID = 5,
    kPB_EFFECT_STOPMUSICGRID = 6,
    kPB_EFFECT_STRINGACTORBODY = 7,
    kPB_EFFECT_CRACKBLOCK = 8,
    kPB_EFFECT_TIRGGERSOUND = 9,
    kPB_EFFECT_VEHICLE = 10,
    kPB_EFFECT_STOPPARTICLE = 11,
};

enum class ePBErrorCode : int32_t {
    kPB_ERROR_UNKNOWN = 1,
    kPB_ERROR_WRONG_MSG_CODE = 11,
    kPB_ERROR_WRONG_REQUESTER = 12,
    kPB_ERROR_WRONG_ARGS = 13,
    kPB_ERROR_PLAYER_NOT_LOGIN = 14,
    kPB_ERROR_ILLEGAL_ARGS = 15,
    kPB_ERROR_OP_NOT_FOUND = 16,
    kPB_ERROR_BACKPACK_FULL = 21,
    kPB_ERROR_STORAGE_FULL = 22,
    kPB_ERROR_ENCHANT_NOT_ENOUGH = 25,
    kPB_ERROR_ENCHANT_FAILED = 26,
    kPB_ERROR_ENCHANT_NOT_CHANGED = 27,
    kPB_ERROR_CRAFT_NOT_ENOUGH = 28,
    kPB_ERROR_STAR_NOT_ENOUGH = 29,
    kPB_ERROR_REPAIR_NOT_ENOUGH = 30,
    kPB_ERROR_REPAIR_NOT_DAMAGED = 31,
    kPB_ERROR_ROLE_ENTER_BANNED = 10011,
    kPB_ERROR_ROLE_ENTER_TOO_OFTEN = 10012,
    kPB_ERROR_ROLE_ENTER_SERVER_FULL = 10013,
    kPB_ERROR_ROLE_MOVE_ILLEGAL_POS = 20031,
    kPB_MAX_ERROR_CODE = 102410,
};

enum class ePBJoinRoomResCode : int32_t {
    kPB_JOIN_ROOM_SUCCESS = 0,
    kPB_JOIN_ROOM_FULL = 1,
    kPB_JOIN_ROOM_WRONG_PASSWD = 2,
    kPB_JOIN_ROOM_NOT_EXISTS = 3,
};

enum class ePBMsgCode : int32_t {
    kPB_REQ_ERROR_HC = 10,
    kPB_HEARTBEAT_CH = 11,
    kPB_HEARTBEAT_HC = 12,
    kPB_SYNC_CHUNK_DATA_CH = 101,
    kPB_SYNC_CHUNK_DATA_HC = 102,
    kPB_BLOCK_DATA_UPDATE_CH = 103,
    kPB_BLOCK_DATA_UPDATE_HC = 104,
    kPB_ROLE_ENTER_WORLD_CH = 1001,
    kPB_ROLE_ENTER_WORLD_HC = 1002,
    kPB_ROLE_LEAVE_WORLD_CH = 1003,
    kPB_ROLE_LEAVE_WORLD_HC = 1004,
    kPB_ACTOR_ENTER_AOI_HC = 1006,
    kPB_ACTOR_LEAVE_AOI_HC = 1008,
    kPB_ROLE_MOVE_CH = 2001,
    kPB_TRAIN_MOVE_CH = 2002,
    kPB_ACTOR_MOVE_HC = 2004,
    kPB_TRAIN_MOVE_HC = 2005,
    kPB_ACTOR_TELEPORT_CH = 2007,
    kPB_ACTOR_TELEPORT_HC = 2008,
    kPB_ACTOR_MOTION_HC = 2009,
    kPB_MECHA_MOTION_HC = 2010,
    kPB_GUN_INFO_CH = 2011,
    kPB_SYNC_SETINFO_CH = 2012,
    kPB_SYNC_GRIDUSERDATA_CH = 2013,
    kPB_SYNC_GRIDUSERDATA_HC = 2014,
    kPB_SYNC_TRIGGERBLOCK_HC = 2015,
    kPB_BLOCK_INTERACT_HC = 2997,
    kPB_BLOCK_PUNCH_HC = 2998,
    kPB_ITEM_USE_HC = 2999,
    kPB_ACTOR_INTERACT_HC = 3000,
    kPB_BLOCK_INTERACT_CH = 3002,
    kPB_BLOCK_PUNCH_CH = 3003,
    kPB_ITEM_USE_CH = 3004,
    kPB_ACTOR_INTERACT_CH = 3005,
    kPB_ACTOR_ANIM_CH = 3006,
    kPB_ACTOR_ANIM_HC = 3007,
    kPB_BACKPACK_GRID_UPDATE_HC = 3008,
    kPB_BACKPACK_GRID_SWAP_CH = 3009,
    kPB_BACKPACK_MOVEITEM_CH = 3010,
    kPB_BACKPACK_GRID_DISCARD_CH = 3011,
    kPB_BACKPACK_EQUIP_WEAPON_CH = 3015,
    kPB_BACKPACK_EQUIP_WEAPON_HC = 3016,
    kPB_CLOSE_CONTAINER_CH = 3017,
    kPB_CLOSE_CONTAINER_HC = 3018,
    kPB_OPEN_CONTAINER_HC = 3020,
    kPB_UPDATE_CONTAINER_HC = 3022,
    kPB_SET_CONTAINERTEXT_CH = 3023,
    kPB_ACTOR_EQUIP_ITEM_HC = 3026,
    kPB_BACKPACK_STORE_CH = 3029,
    kPB_BACKPACK_LOOT_CH = 3031,
    kPB_BACKPACK_SORT_CH = 3033,
    kPB_BACKPACK_SETITEM_CH = 3034,
    kPB_STORAGEBOX_SORT_CH = 3035,
    kPB_CRAFT_ITEM_CH = 3037,
    kPB_ENCHANT_ITEM_CH = 3039,
    kPB_ENCHANT_ITEM_RANDOM_CH = 3041,
    kPB_ENCHANT_ITEM_SUCCESS_HC = 3042,
    kPB_REPAIR_ITEM_CH = 3043,
    kPB_REPAIR_ITEM_SUCCESS_HC = 3044,
    kPB_GUN_DORELOAD_CH = 3045,
    kPB_GUN_DORELOAD_HC = 3046,
    kPB_ACCOUNT_HORSE_CH = 3050,
    kPB_ACCOUNT_HORSE_HC = 3051,
    kPB_ACTOT_SET_CUSTOM_CH = 3052,
    kPB_ACTOT_SET_CUSTOM_HC = 3053,
    kPB_ACTOR_ATTR_CHANGE_HC = 4000,
    kPB_ACTOR_BUFF_CHANGE_HC = 4001,
    kPB_ACTOR_REVIVE_CH = 4002,
    kPB_ACTOR_REVIVE_HC = 4003,
    kPB_PLAYER_ATTR_CHANGE_HC = 4004,
    kPB_MOB_BODY_CHANGE_HC = 4005,
    kPB_ROOM_JRUISDICTION_CH = 4006,
    kPB_ROOM_JRUISDICTION_HC = 4007,
    kPB_CHAT_CH = 4010,
    kPB_CHAT_HC = 4011,
    kPB_WGLOBAL_UPDATE_HC = 4012,
    kPB_PLAYERS_UPDATEINFO_HC = 4013,
    kPB_GAME_TIPS_HC = 4014,
    kPB_PLAYEFFECT_HC = 4015,
    kPB_PLAYER_MOUNTACTOR_CH = 4016,
    kPB_PLAYER_MOUNTACTOR_HC = 4017,
    kPB_PLAYER_MOVEINPUT_CH = 4018,
    kPB_PLAYER_SLEEP_HC = 4020,
    kPB_PLAYER_SLEEP_CH = 4021,
    kPB_OPENWINDOW_HC = 4022,
    kPB_NPCTRADE_CH = 4023,
    kPB_LASTPING_HC = 4024,
    kPB_CGAMESTAGE_HC = 4025,
    kPB_PLAYERPERMIT_HC = 4026,
    kPB_SKILLCD_HC = 4029,
    kPB_ACTOR_MOUNTACTOR_HC = 4030,
    kPB_ACTOR_REVERSE_HC = 4031,
    kPB_ACTOR_BIND_HC = 4032,
    kPB_PLAYWEAPONEFFECT_HC = 4033,
    kPB_SCRIPTVAR_HC = 4034,
    kPB_SET_SPECTATORMODE_CH = 4037,
    kPB_SET_SPECTATORMODE_HC = 4038,
    kPB_SET_SPECTATORTYPE_CH = 4039,
    kPB_SET_SPECTATORTYPE_HC = 4040,
    kPB_SET_SPECTATOR_PLAYER_CH = 4041,
    kPB_OTHER_PLAYER_ATTR_CHANGE_HC = 4042,
    kPB_ACTOR_GET_ACCOUNT_ITEM = 5001,
    kPB_SPECIALITEM_USE_CH = 5002,
    kPB_SPECIALITEM_USE_HC = 5003,
    kPB_LEAVE_ROOM_INFO_HC = 5004,
    kPB_INVITEJOINROOM_HC = 5005,
    kPB_SET_SPECTATOR_PLAYER_HC = 5006,
    kPB_SET_PLAYER_MODEL_ANI_CH = 5007,
    kPB_SET_PLAYER_MODEL_ANI_HC = 5008,
    kPB_SEND_VIEWMODE_SPECTATOR_CH = 5009,
    kPB_SEND_VIEWMODE_SPECTATOR_HC = 5010,
    kPB_SET_BOBBING_SPECTATOR_CH = 5011,
    kPB_SET_BOBBING_SPECTATOR_HC = 5012,
    kPB_ITEM_SKILL_USE_CH = 5013,
    kPB_ITEM_SKILL_USE_HC = 5014,
    kPB_BALL_OPERATE_CH = 5015,
    kPB_BALL_OPERATE_HC = 5016,
    kPB_RESET_ROUND_HC = 5017,
    kPB_ROCKET_ATTRIB_CHANGE_HC = 5018,
    kPB_ROCKET_TELEPORT_CH = 5019,
    kPB_SET_HOOK_HC = 5020,
    kPB_SET_HOOK_CH = 5021,
    kPB_WORLD_TIMES_HC = 5022,
    kPB_STATISTIC_HC = 5023,
    kPB_TOTEMPOINT_HC = 5024,
    kPB_NEED_CONTAINER_PASSWORD_HC = 5025,
    kPB_NEED_CONTAINER_PASSWORD_CH = 5026,
    kPB_HORSEFLYSTATE_HC = 5027,
    kPB_OPENDIALOGUE_HC = 5028,
    kPB_CLOSEDIALOGUE_HC = 5029,
    kPB_CLOSEDIALOGUE_CH = 5030,
    kPB_ANSWERTASK_CH = 5031,
    kPB_UPDATETASK_HC = 5032,
    kPB_SYNCTASK_ENTERWORLD_HC = 5033,
    kPB_COMPLETE_TASK_HC = 5034,
    kPB_COMPLETE_TASK_CH = 5035,
    kPB_ATTRACT_ATTRIB_CHANGE_HC = 5036,
    kPB_ACTOR_BODY_TEXTURE_HC = 5037,
    kPB_PLAYER_ADDAVARTAR_HC = 5038,
    kPB_PLAYER_CHANGEMODEL_HC = 5039,
    kPB_PLAYER_AVARTARCOLOR_HC = 5040,
    kPB_PLAYER_ACT_CH = 5041,
    kPB_PLAYER_ACT_HC = 5042,
    kPB_CREATE_BLUEPRINT_HC = 5043,
    kPB_MEASURE_DISTANCE_HC = 5044,
    kPB_BLUEPRINT_PREBLOCK_CH = 5045,
    kPB_BLUEPRINT_PREBLOCK_HC = 5046,
    kPB_GRAVITY_OPERATE_CH = 5047,
    kPB_GRAVITY_OPERATE_HC = 5048,
    kPB_PLAYER_BODY_COLOR_HC = 5049,
    kPB_CUSTOM_MODEL_HC = 5050,
    kPB_CUSTOM_ITEMIDS_HC = 5051,
    kPB_PLAYER_SPAWN_POINT_HC = 5052,
    kPB_MAKE_CUSTOM_MODEL_CH = 5053,
    kPB_SELECT_MOB_SPAWN_CH = 5054,
    kPB_CUSTOM_MODELCLASS_HC = 5055,
    kPB_TRANSFER_RECORD_HC = 5056,
    kPB_TRANSFER_RECORD_CH = 5057,
    kPB_TRANSFER_ADD_DEL_HC = 5058,
    kPB_TRANSFER_STATUS_CH = 5059,
    kPB_TRANSFER_STATUS_HC = 5060,
    kPB_SYNC_LOVEAMBASSADOR_ICONID_HC = 5061,
    kPB_SYNC_LOVEAMBASSADOR_ICONID_CH = 5062,
    kPB_TRANSFER_DATA_HC = 5063,
    kPB_ACTOR_TRANSFER_CH = 5064,
    kPB_ACTOR_TRANSFER_HC = 5065,
    kPB_BACKPACK_SETITEMWITHOUTLIMIT_CH = 5066,
    kPB_NPCSHOP_GETSHOPINFO_CH = 5067,
    kPB_NPCSHOP_RESPGETSHOPINFO_HC = 5068,
    kPB_NPCSHOP_BUYSKU_CH = 5069,
    kPB_NPCSHOP_NOTIFYBUY_HC = 5070,
    kPB_VEHICLE_MOVE_HC = 5071,
    kPB_OPEN_EDIT_ACTORMODEL_HC = 5072,
    kPB_CLOSE_EDIT_ACTORMODEL_CH = 5073,
    kPB_CLOSE_EDIT_ACTORMODEL_HC = 5074,
    kPB_CUSTOMACTOR_MODELDATA_HC = 5075,
    kPB_PACKGIFT_NOTIFYITEMCHANGE_HC = 5076,
    kPB_VEHICLE_PREBLOCK_CH = 5077,
    kPB_VEHICLE_PREBLOCK_HC = 5078,
    kPB_VEHICLE_ITEMUSE_CH = 5079,
    kPB_VEHICLE_STARTBLOCK_CH = 5080,
    kPB_VEHICLE_ALL_ITEMID_HC = 5081,
    kPB_VEHICLE_ONE_ITEMID_HC = 5082,
    kPB_VEHICLE_ATTRIB_CHANGE_HC = 5083,
    kPB_VEHICLE_ATTRIB_CHANGE_CH = 5084,
    kPB_WORKSHOP_ITEMINFO_CH = 5085,
    kPB_WORKSHOP_ITEMINFO_HC = 5086,
    kPB_VEHICLEASSEMBLEBLOCK_UPDATE_HC = 5087,
    kPB_PLAYER_VEHICLE_MOVEINPUT_CH = 5088,
    kPB_PLAYER_RESETVEHICLE_CH = 5089,
    kPB_PLAYER_MOTIONSTATECHANGE_CH = 5090,
    kPB_PLAYER_CLICK_CH = 5091,
    kPB_PLAYER_CAMERAROTATE_HC = 5092,
    kPB_PLAYER_CHANGEVIEWMODE_HC = 5093,
    kPB_PLAYER_CANMOVE_HC = 5094,
    kPB_PLAYER_CANCONTROL_HC = 5095,
    kPB_PLAYER_SETATTR_HC = 5096,
    kPB_PLAYER_FREEZING_HC = 5097,
    kPB_PLAYER_SELECTSHORTCUT_CH = 5098,
    kPB_GAMERULE_HC = 5099,
    kPB_BASKETBALL_OPERATE_HC = 5100,
    kPB_BASKETBALL_OPERATE_CH = 5101,
    kPB_TRIGGER_TIMER_HC = 6000,
    kPB_WORKSHOP_BUILD_HC = 6001,
    kPB_TRIGGER_PLAYER_ATTRI_CH = 6002,
    kPB_PLAYER_ATTR_SCALE_HC = 6003,
    kPB_PLAYER_ATTR_SCALE_CH = 6004,
    kPB_PLAYER_NAVIGATE_HC = 6005,
    kPB_PLAYER_FACE_YAW_HC = 6006,
    kPB_OPENEDIT_FULLYCUSTOMMODEL_HC = 6007,
    kPB_REQ_DOWNLOADRES_URL_CH = 6008,
    kPB_CLOSE_FULLYCUSTOMMODEL_UI_CH = 6009,
    kPB_CLOSE_FULLYCUSTOMMODEL_UI_HC = 6010,
    kPB_RESP_DOWNLOADRES_URL_HC = 6011,
    kPB_PRE_OPEN_EDIT_FCM_UI = 6012,
    kPB_EFFECTSCALE_HC = 6013,
    kPB_PLAYER_NAVFINISHED_CH = 6014,
    kPB_TRIGGER_MUSIC_HC = 6015,
    kPB_TRIGGER_SOUND_CH = 6016,
    kPB_PLAYER_JUMP_HC = 6017,
    kPB_PLAYER_JUMP_CH = 6018,
    kPB_PLAYER_SPECIAL_SKILL_CH = 6019,
    kPB_HORSE_SKILLCD_HC = 6020,
    kPB_CLOUDSERVER_PERMIT_CH = 6021,
    kPB_CLOUDSERVER_PERMIT_HC = 6022,
    kPB_CLOUDSERVER_AUTHORITY_HC = 6023,
    kPB_CLOUDSERVER_AUTHORITY_CH = 6024,
    kPB_SS_SYNC_TASK_HC = 6025,
    kPB_SS_SYNC_TASK_CH = 6026,
    kPB_VEHICLEASSEMBLEBLOCK_ALL_HC = 6027,
    kPB_VEHICLE_ASSEMBLE_LINE_CH = 6028,
    kPB_VEHICLE_ASSEMBLE_LINE_HC = 6029,
    kPB_VEHICLE_ASSEMBLE_LINE_OPERATE_CH = 6030,
    kPB_VEHICLE_ASSEMBLE_LINE_OPERATE_HC = 6031,
    kPB_ACTIONEDATA_UPDATE_CH = 6032,
    kPB_VEHICLE_WORKSHOP_LINE_CH = 6033,
    kPB_CLOUDSERVER_CHANGE_TEAM_CH = 6034,
    kPB_CLOUDSERVER_CHANGE_STATE_HC = 6035,
    kPB_YM_CHANGEROLE_HC = 6036,
    kPB_YM_CHANGEROLE_CH = 6037,
    kPB_YM_VOICE_CH = 6038,
    kPB_YM_VOICE_HC = 6039,
    kPB_CLOUDSERVER_ROOM_AUTOMUTE_CH = 6040,
    kPB_VEHICLE_WORKSHOP_LINE_UPDATE_CH = 6041,
    kPB_MAP_EDIT_HANDLE_CH = 6042,
    kPB_MAP_EDIT_REVOKE_CH = 6043,
    kPB_CLOUD_ROOM_OWNER_START_GAME_CH = 6044,
    kPB_CLOUD_ROOM_KICK_OFF_CH = 6045,
    kPB_TRIGGER_OPENSTORE_HC = 6046,
    kPB_USE_PACKINGFCMITEM_CH = 6047,
    kPB_USE_PACKINGFCMITEM_HC = 6048,
    kPB_CREATE_PACKINGCM_CH = 6049,
    kPB_CREATE_PACKINGCM_HC = 6050,
    kPB_PACKING_FCMDATA_HC = 6051,
    kPB_PLAYER_INPUTCONTENT_CH = 6052,
    kPB_PLAYER_INPUTKEYS_CH = 6053,
    kPB_CLOUD_ROOM_STATUSTIME_HC = 6054,
    kPB_PLAYER_VEHICLE_SLEEP_HC = 6055,
    kPB_SENSOR_CONTAINER_DATA_CH = 6056,
    kPB_SENSOR_CONTAINER_DATA_HC = 6057,
    kPB_VEHICLE_BIND_ACTOR_HC = 6058,
    kPB_DOOR_DATA_HC = 6059,
    kPB_PLAYER_CARRYACTOR_CH = 6060,
    kPB_PLAYER_CARRYACTOR_HC = 6061,
    kPB_VILLAGER_BODY_CHANGE_HC = 6062,
    kPB_PLAYER_TAME_ACTOR_HC = 6063,
    kPB_VILLAGER_MODIFY_NAME_CH = 6064,
    kPB_VILLAGER_CLOTH_HC = 6065,
    kPB_ACTOR_HEAD_DISPLAY_ICON_HC = 6066,
    kPB_ACTOR_PLAY_ANIM_BY_ID_HC = 6067,
    kPB_VILLAGE_TOTEM_TIP_HC = 6068,
    kPB_VILLAGE_TOTEM_ACTIVE_HC = 6069,
    kPB_SAVE_TOMB_STONE_HC = 6070,
    kPB_PLAYER_LEVELMODE_HC = 6071,
    kPB_ACTION_ATTR_STATE_HC = 6072,
    kPB_EDU_ROLEINFO_HC = 6073,
    kPB_PLAYER_GOTOPOS_CH = 6074,
    kPB_IMPORT_MODEL_HC = 6075,
    kPB_CUSTOM_MODEL_PRE_HC = 6076,
    kPB_CUSTOM_MODEL_PRE_CH = 6077,
    kPB_RESETDEFORMATION_CH = 6078,
    kPB_DEFORMATION_SKIN_CH = 6079,
    kPB_PLAYERTRANSFORMSKIN_HC = 6080,
    kPB_RESTORE_DEFORMATION_CH = 6081,
    kPB_PLAYER_SAVE_ARCH_HC = 6082,
    kPB_PLAYER_PUSH_ARCH_CH = 6083,
    kPB_LIGHTNING_HC = 6084,
    kPB_INTERACT_MOBPACK_HC = 6085,
    kPB_UPDATE_MOB_BACKPACK_HC = 6086,
    kPB_MOVE_MOBBACKPACKITEM_CH = 6087,
    kPB_INTERACT_MOBBACKPACKITEM_CH = 6088,
    kPB_ALTAR_LUCKY_DRAW_CH = 6089,
    kPB_SFACTIVITY_HC = 6090,
    kPB_OPEN_DEVGOODSBUY_DIALOGHC = 6091,
    kPB_HOME_PRAY_INFO_HC = 6092,
    kPB_HOME_PRAY_TREE_STATE_HC = 6093,
    kPB_HOME_PRAY_REQ_HC = 6094,
    kPB_HOME_PRAY_TIME_CH = 6095,
    kPB_HOME_PRAY_TIMEUPDATE_HC = 6096,
    kPB_HOME_PRAY_ERROR_HC = 6097,
    kPB_OPEN_HOMENPC_HC = 6098,
    kPB_OPEN_HOMECLOSET_HC = 6099,
    kPB_GODTEMPLE_CREATE_HC = 6100,
    kPB_SHAPE_ADDITION_ANIM_HC = 6101,
    kPB_HOME_SUMMONPET_CH = 6102,
    kPB_HOMELAND_RANCH_HC = 6103,
    kPB_HOMELAND_RANCH_ANIMAL_UPDATE_CH = 6104,
    kPB_TRIGGER_GRAPHICS_HC = 6105,
    kPB_USEITEM_BY_HOMELAND_HC = 6106,
    kPB_PLAYER_CUSTOM_BASEMODEL_HC = 6107,
    kPB_CHANGE_ACTOR_MODEL_HC = 6108,
    kPB_REQUEST_MODEL_CH = 6109,
    kPB_NOTIFIY_MODEL_HC = 6110,
    kPB_WORLD_SYNC_SAVE_HC = 10002,
    kPB_MAX_MSG_CODE = 10240,
};

enum class ePBRoomCmd : int32_t {
    kPB_LOGIN_ROOM_SERVER_REQ = 1,
    kPB_LOGIN_ROOM_SERVER_RES = 2,
    kPB_CREATE_ROOM_REQ = 11,
    kPB_CREATE_ROOM_RES = 12,
    kPB_UPDATE_ROOM_INFO_REQ = 31,
    kPB_UPDATE_ROOM_INFO_RES = 32,
    kPB_ROOM_HEARTBEAT_REQ = 33,
    kPB_ROOM_HEARTBEAT_RES = 34,
    kPB_GET_ROOMS_INFO_REQ = 101,
    kPB_GET_ROOMS_INFO_RES = 102,
    kPB_GET_ROOMS_INFO_BY_USERS_REQ = 103,
    kPB_GET_ROOMS_INFO_BY_USERS_RES = 104,
    kPB_GET_ROOMS_INFO_BY_IS_PASSWORD_REQ = 105,
    kPB_GET_ROOMS_INFO_BY_IS_PASSWORD_RES = 106,
    kPB_GET_ROOMS_INFO_BY_STRING_REQ = 107,
    kPB_GET_ROOMS_INFO_BY_STRING_RES = 108,
    kPB_GET_ROOMS_INFO_BY_GAME_TYPE_REQ = 109,
    kPB_GET_ROOMS_INFO_BY_GAME_TYPE_RES = 110,
    kPB_GET_ROOMS_INFO_BY_GPS_REQ = 111,
    kPB_GET_ROOMS_INFO_BY_GPS_RES = 112,
    kPB_GET_ROOMS_INFO_BY_MAX_PLAYER_REQ = 113,
    kPB_GET_ROOMS_INFO_BY_MAX_PLAYER_RES = 114,
    kPB_GET_ROOMS_INFO_BY_ISP_REQ = 115,
    kPB_GET_ROOMS_INFO_BY_ISP_RES = 116,
    kPB_GET_ROOMS_INFO_BY_GAME_LABEL_REQ = 117,
    kPB_GET_ROOMS_INFO_BY_GAME_LABEL_RES = 118,
    kPB_DELETE_ROOM_REQ = 201,
    kPB_DELETE_ROOM_RES = 202,
    kPB_JOIN_ROOM_NOTIFICATION = 211,
    kPB_LEAVE_ROOM_NOTIFICATION = 213,
    kPB_JOIN_ROOM_REQ = 214,
    kPB_JOIN_ROOM_RES = 215,
    kPB_UPDATE_ROOM_OWORLD_PIC_REQ = 216,
    kPB_UPDATE_ROOM_OWORLD_PIC_RES = 217,
    kPB_SELECT_ROOM_OWORLD_PIC_REQ = 218,
    kPB_SELECT_ROOM_OWORLD_PIC_RES = 219,
    kPB_MAX_ROOM_SERVER_CMD = 255,
};

enum class ePBRoomResCode : int32_t {
    kPB_ROOM_OP_SUCCESS = 1,
    kPB_ROOM_OP_SUCCESS_LIMIT = 2,
    kPB_ROOM_OP_FAILED = 10,
    kPB_ROOM_OP_ERROR_UNKNOW = 250,
    kPB_ROOM_OP_NEARBY_SUCCESS = 3,
};

enum class ePBStanceType : int32_t {
    kPB_STANCESTAND = 1,
    kPB_STANCEWALK = 2,
    kPB_STANCERUN = 3,
    kPB_STANCEJUMP = 4,
    kPB_STANCELAY = 5,
    kPB_STANCESWIM = 6,
    kPB_STANCEFLY = 7,
};

struct PB_HeartBeatCH {
    uint64_t BeatCode{};  /* = 1 */
    bool has_BeatCode = false;
};

struct PB_Vector3 {
    int32_t X{};  /* = 1 */
    bool has_X = false;
    int32_t Y{};  /* = 2 */
    bool has_Y = false;
    int32_t Z{};  /* = 3 */
    bool has_Z = false;
};

struct PB_SyncChunkDataCH {
    PB_Vector3 SectionCoord{};  /* = 1 */
    bool has_SectionCoord = false;
    std::string trunkmd5{};  /* = 2 */
    bool has_trunkmd5 = false;
};

struct PB_PlayerVipInfo {
    int32_t VipType{};  /* = 1 */
    bool has_VipType = false;
    int32_t VipLevel{};  /* = 2 */
    bool has_VipLevel = false;
    int32_t VipExp{};  /* = 3 */
    bool has_VipExp = false;
};

struct PB_RoleInfo {
    int32_t Model{};  /* = 1 */
    bool has_Model = false;
    std::string NickName{};  /* = 2 */
    bool has_NickName = false;
    int32_t SkinID{};  /* = 3 */
    bool has_SkinID = false;
    std::string CustomJson{};  /* = 4 */
    bool has_CustomJson = false;
    int32_t FrameID{};  /* = 5 */
    bool has_FrameID = false;
};

struct PB_RoleEnterWorldCH {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t GeniusLv{};  /* = 2 */
    bool has_GeniusLv = false;
    int32_t UICtrlMode{};  /* = 3 */
    bool has_UICtrlMode = false;
    PB_RoleInfo RoleInfo{};  /* = 4 */
    bool has_RoleInfo = false;
    PB_PlayerVipInfo VipInfo{};  /* = 5 */
    bool has_VipInfo = false;
    int32_t lang{};  /* = 6 */
    bool has_lang = false;
    int32_t apiid{};  /* = 7 */
    bool has_apiid = false;
    int32_t reserved{};  /* = 8 */
    bool has_reserved = false;
    std::string Auth{};  /* = 9 */
    bool has_Auth = false;
};

struct PB_RoleLeaveWorldCH {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
};

struct PB_MoveMotion {
    PB_Vector3 Position{};  /* = 1 */
    bool has_Position = false;
    uint32_t Yaw{};  /* = 2 */
    bool has_Yaw = false;
    uint32_t Pitch{};  /* = 3 */
    bool has_Pitch = false;
    uint32_t MapID{};  /* = 4 */
    bool has_MapID = false;
    int32_t ChangeFlags{};  /* = 5 */
    bool has_ChangeFlags = false;
};

struct PB_RoleMoveCH {
    PB_MoveMotion MoveMotion{};  /* = 1 */
    bool has_MoveMotion = false;
    PB_Vector3 AddMotion{};  /* = 2 */
    bool has_AddMotion = false;
    int32_t rentToken{};  /* = 3 */
    bool has_rentToken = false;
};

struct PB_TrainMoveCH {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    uint32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    int32_t CarReverse{};  /* = 3 */
    bool has_CarReverse = false;
    int32_t OutIndex{};  /* = 4 */
    bool has_OutIndex = false;
    float CurveT{};  /* = 5 */
    bool has_CurveT = false;
    float MotionX{};  /* = 6 */
    bool has_MotionX = false;
    float MotionY{};  /* = 7 */
    bool has_MotionY = false;
    float MotionZ{};  /* = 8 */
    bool has_MotionZ = false;
    PB_Vector3 RailKnot{};  /* = 9 */
    bool has_RailKnot = false;
};

struct PB_ActorTeleportCH {
    int64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t TargetMap{};  /* = 2 */
    bool has_TargetMap = false;
    PB_Vector3 TargetPos{};  /* = 3 */
    bool has_TargetPos = false;
};

struct PB_GunInfoCH {
    float CurSpread{};  /* = 1 */
    bool has_CurSpread = false;
    float CurJaw{};  /* = 2 */
    bool has_CurJaw = false;
    float CurPitch{};  /* = 3 */
    bool has_CurPitch = false;
    PB_Vector3 CurPos{};  /* = 4 */
    bool has_CurPos = false;
};

struct PB_SetInfoCH {
    int32_t Color{};  /* = 1 */
    bool has_Color = false;
};

struct PB_ItemGridUserData {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t GridIndex{};  /* = 2 */
    bool has_GridIndex = false;
    std::string UserDataStr{};  /* = 3 */
    bool has_UserDataStr = false;
    int32_t Type{};  /* = 4 */
    bool has_Type = false;
};

struct PB_BlockInteractCH {
    int32_t face{};  /* = 1 */
    bool has_face = false;
    int32_t colptx{};  /* = 2 */
    bool has_colptx = false;
    int32_t colpty{};  /* = 3 */
    bool has_colpty = false;
    int32_t colptz{};  /* = 4 */
    bool has_colptz = false;
    PB_Vector3 blockpos{};  /* = 5 */
    bool has_blockpos = false;
};

struct PB_BlockPunchCH {
    int32_t status{};  /* = 1 */
    bool has_status = false;
    int32_t face{};  /* = 2 */
    bool has_face = false;
    int32_t digmethod{};  /* = 3 */
    bool has_digmethod = false;
    PB_Vector3 blockpos{};  /* = 4 */
    bool has_blockpos = false;
};

struct PB_ItemUseCH {
    int32_t itemid{};  /* = 1 */
    bool has_itemid = false;
    int32_t status{};  /* = 2 */
    bool has_status = false;
    int32_t shift{};  /* = 3 */
    bool has_shift = false;
    float CurSpread{};  /* = 4 */
    bool has_CurSpread = false;
    float CurYaw{};  /* = 5 */
    bool has_CurYaw = false;
    float CurPitch{};  /* = 6 */
    bool has_CurPitch = false;
    PB_Vector3 CurPos{};  /* = 7 */
    bool has_CurPos = false;
};

struct PB_SpecialSkillCH {
    int32_t status{};  /* = 1 */
    bool has_status = false;
};

struct PB_SetHookCH {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    uint64_t hookID{};  /* = 2 */
    bool has_hookID = false;
};

struct PB_ItemSkillUseCH {
    int32_t itemid{};  /* = 1 */
    bool has_itemid = false;
    int32_t status{};  /* = 2 */
    bool has_status = false;
    int32_t skillid{};  /* = 3 */
    bool has_skillid = false;
    float CurSpread{};  /* = 4 */
    bool has_CurSpread = false;
    PB_Vector3 CurPos{};  /* = 5 */
    bool has_CurPos = false;
    float CurDirX{};  /* = 6 */
    bool has_CurDirX = false;
    float CurDirY{};  /* = 7 */
    bool has_CurDirY = false;
    float CurDirZ{};  /* = 8 */
    bool has_CurDirZ = false;
};

struct PB_ActorInteractCH {
    int32_t itype{};  /* = 1 */
    bool has_itype = false;
    uint64_t target{};  /* = 2 */
    bool has_target = false;
    int32_t iplot{};  /* = 3 */
    bool has_iplot = false;
    float CurYaw{};  /* = 4 */
    bool has_CurYaw = false;
    float CurPitch{};  /* = 5 */
    bool has_CurPitch = false;
    PB_Vector3 CurPos{};  /* = 6 */
    bool has_CurPos = false;
};

struct PB_ActorAnimCH {
    int32_t anim{};  /* = 1 */
    bool has_anim = false;
    int32_t anim1{};  /* = 2 */
    bool has_anim1 = false;
    int32_t actid{};  /* = 3 */
    bool has_actid = false;
    int32_t actidTrigger{};  /* = 4 */
    bool has_actidTrigger = false;
};

struct PB_BackPackGridSwapCH {
    int32_t FromGridId{};  /* = 1 */
    bool has_FromGridId = false;
    int32_t ToGridId{};  /* = 2 */
    bool has_ToGridId = false;
};

struct PB_BackPackMoveItemCH {
    int32_t FromIndex{};  /* = 1 */
    bool has_FromIndex = false;
    int32_t ToIndex{};  /* = 2 */
    bool has_ToIndex = false;
    int32_t Num{};  /* = 3 */
    bool has_Num = false;
};

struct PB_BackPackGridDiscardCH {
    int32_t GridId{};  /* = 1 */
    bool has_GridId = false;
    int32_t Num{};  /* = 2 */
    bool has_Num = false;
};

struct PB_BackPackEquipWeaponCH {
    int32_t GridId{};  /* = 1 */
    bool has_GridId = false;
};

struct PB_NeedContainerPasswordCH {
    PB_Vector3 Pos{};  /* = 1 */
    bool has_Pos = false;
    int32_t Password{};  /* = 2 */
    bool has_Password = false;
    uint64_t VehicleObjID{};  /* = 3 */
    bool has_VehicleObjID = false;
};

struct PB_CloseContainerCH {
    int32_t BaseIndex{};  /* = 1 */
    bool has_BaseIndex = false;
};

struct PB_SetContainerTextCH {
    int32_t BaseIndex{};  /* = 1 */
    bool has_BaseIndex = false;
    std::string Text{};  /* = 2 */
    bool has_Text = false;
};

struct PB_BackPackStoreCH {
    int32_t FromIndex{};  /* = 1 */
    bool has_FromIndex = false;
    int32_t Num{};  /* = 2 */
    bool has_Num = false;
};

struct PB_BackPackLootCH {
    int32_t FromIndex{};  /* = 1 */
    bool has_FromIndex = false;
    int32_t Num{};  /* = 2 */
    bool has_Num = false;
};

struct PB_BackPackSortCH {
    int32_t BaseIndex{};  /* = 1 */
    bool has_BaseIndex = false;
};

struct PB_BackPackSetItemCH {
    int32_t ItemId{};  /* = 1 */
    bool has_ItemId = false;
    int32_t ToIndex{};  /* = 2 */
    bool has_ToIndex = false;
    int32_t Num{};  /* = 3 */
    bool has_Num = false;
};

struct PB_BackPackSetItemWithoutLimitCH {
    int32_t ItemId{};  /* = 1 */
    bool has_ItemId = false;
    int32_t ToIndex{};  /* = 2 */
    bool has_ToIndex = false;
    int32_t Num{};  /* = 3 */
    bool has_Num = false;
    std::string Userdata_Str{};  /* = 4 */
    bool has_Userdata_Str = false;
};

struct PB_StorageBoxSortCH {
    int32_t BaseIndex{};  /* = 1 */
    bool has_BaseIndex = false;
};

struct PB_CraftItemCH {
    int32_t CraftId{};  /* = 1 */
    bool has_CraftId = false;
};

struct PB_EnchantItemCH {
    int32_t GridIndex{};  /* = 1 */
    bool has_GridIndex = false;
    int32_t FrmGridIndex{};  /* = 2 */
    bool has_FrmGridIndex = false;
    std::vector<int32_t> EnchantIds;  /* = 3 */
};

struct PB_EnchantItemRandomCH {
    int32_t GridIndex{};  /* = 1 */
    bool has_GridIndex = false;
};

struct PB_RepairItemCH {
    int32_t GridIndex{};  /* = 1 */
    bool has_GridIndex = false;
    int32_t MaterialID{};  /* = 2 */
    bool has_MaterialID = false;
    int32_t UseNum{};  /* = 3 */
    bool has_UseNum = false;
};

struct PB_GunDoReloadCH {
    int32_t BulletID{};  /* = 1 */
    bool has_BulletID = false;
    int32_t Num{};  /* = 2 */
    bool has_Num = false;
};

struct PB_AccountHorseCH {
    int32_t HorseID{};  /* = 1 */
    bool has_HorseID = false;
    int32_t CmdType{};  /* = 2 */
    bool has_CmdType = false;
    int32_t CmdData{};  /* = 3 */
    bool has_CmdData = false;
};

struct PB_ActorReviveCH {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t Type{};  /* = 2 */
    bool has_Type = false;
};

struct PB_JruisdicTionCH {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
};

struct PB_ChatCH {
    int32_t ChatType{};  /* = 1 */
    bool has_ChatType = false;
    int32_t TargetUin{};  /* = 2 */
    bool has_TargetUin = false;
    std::string Content{};  /* = 3 */
    bool has_Content = false;
    int32_t Language{};  /* = 4 */
    bool has_Language = false;
};

struct PB_PlayerMountActorCH {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    bool IsShapeShift{};  /* = 2 */
    bool has_IsShapeShift = false;
    int32_t InteractBlockId{};  /* = 3 */
    bool has_InteractBlockId = false;
};

struct PB_PlayerMoveInputCH {
    float MoveForward{};  /* = 1 */
    bool has_MoveForward = false;
    float MoveStrafing{};  /* = 2 */
    bool has_MoveStrafing = false;
    int32_t Jumping{};  /* = 3 */
    bool has_Jumping = false;
    int32_t Sneaking{};  /* = 4 */
    bool has_Sneaking = false;
};

struct PB_PlayerSleepCH {
    int32_t Flags{};  /* = 1 */
    bool has_Flags = false;
};

struct PB_NpcTradeCH {
    int32_t OpType{};  /* = 1 */
    bool has_OpType = false;
    int32_t Index{};  /* = 2 */
    bool has_Index = false;
    int32_t WatchAD{};  /* = 3 */
    bool has_WatchAD = false;
    int32_t RewardNum{};  /* = 4 */
    bool has_RewardNum = false;
};

struct PB_YMVoiceCH {
    int32_t YMMemberID{};  /* = 1 */
    bool has_YMMemberID = false;
    int32_t YMSpeakerSwitch{};  /* = 2 */
    bool has_YMSpeakerSwitch = false;
    int32_t YMMicSwitch{};  /* = 3 */
    bool has_YMMicSwitch = false;
    int32_t YMMemberRole{};  /* = 4 */
    bool has_YMMemberRole = false;
};

struct PB_GVChangeRoleCH {
    int32_t ChangeResult{};  /* = 1 */
    bool has_ChangeResult = false;
};

struct PB_YMChangeRoleCH {
    int32_t ChangeResult{};  /* = 1 */
    bool has_ChangeResult = false;
};

struct PB_GetAccountItemsCH {
    int32_t ItemId{};  /* = 1 */
    bool has_ItemId = false;
    int32_t Num{};  /* = 2 */
    bool has_Num = false;
};

struct PB_SpecialItemUseCH {
    int32_t GridIndex{};  /* = 1 */
    bool has_GridIndex = false;
    int32_t ItemId{};  /* = 2 */
    bool has_ItemId = false;
    int32_t ItemNum{};  /* = 3 */
    bool has_ItemNum = false;
};

struct PB_SetSpectatorModeCH {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t SpectatorMode{};  /* = 2 */
    bool has_SpectatorMode = false;
};

struct PB_SetSpectatorTypeCH {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t SpectatorType{};  /* = 2 */
    bool has_SpectatorType = false;
};

struct PB_SetSpectatorPlayerCH {
    int32_t SpectatorUin{};  /* = 1 */
    bool has_SpectatorUin = false;
    int32_t ToSpectatorUin{};  /* = 2 */
    bool has_ToSpectatorUin = false;
};

struct PB_SetPlayerModelAniCH {
    int32_t SpectatorUin{};  /* = 1 */
    bool has_SpectatorUin = false;
    int32_t ToSpectatorUin{};  /* = 2 */
    bool has_ToSpectatorUin = false;
    int32_t ModelAnimalType{};  /* = 3 */
    bool has_ModelAnimalType = false;
    int32_t ModelAnimalExt{};  /* = 4 */
    bool has_ModelAnimalExt = false;
};

struct PB_SendMyViewmodeToSpectatorCH {
    int32_t SpectatorUin{};  /* = 1 */
    bool has_SpectatorUin = false;
    int32_t ToSpectatorUin{};  /* = 2 */
    bool has_ToSpectatorUin = false;
    int32_t MyViewmode{};  /* = 3 */
    bool has_MyViewmode = false;
};

struct PB_SetBobbingToSpectatorCH {
    int32_t SpectatorUin{};  /* = 1 */
    bool has_SpectatorUin = false;
    int32_t ToSpectatorUin{};  /* = 2 */
    bool has_ToSpectatorUin = false;
    int32_t Bobbing{};  /* = 3 */
    bool has_Bobbing = false;
};

struct PB_BallOperateCH {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    uint64_t ActorID{};  /* = 2 */
    bool has_ActorID = false;
    int32_t ExtendData{};  /* = 3 */
    bool has_ExtendData = false;
};

struct PB_RocketTeleportCH {
    int32_t MapId{};  /* = 1 */
    bool has_MapId = false;
};

struct PB_CloseDialogueCH {
};

struct PB_AnswerTaskCH {
    int32_t TaskID{};  /* = 1 */
    bool has_TaskID = false;
    int32_t PlotID{};  /* = 2 */
    bool has_PlotID = false;
    int32_t Type{};  /* = 3 */
    bool has_Type = false;
};

struct PB_CompleteTaskCH {
    int32_t TaskID{};  /* = 1 */
    bool has_TaskID = false;
};

struct PB_PlayActCH {
    int32_t ActID{};  /* = 1 */
    bool has_ActID = false;
    int32_t ActIDTrigger{};  /* = 2 */
    bool has_ActIDTrigger = false;
};

struct PB_BluePrintPreBlockCH {
    PB_Vector3 blockpos{};  /* = 1 */
    bool has_blockpos = false;
};

struct PB_GravityOperateCH {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    uint64_t ActorID{};  /* = 2 */
    bool has_ActorID = false;
    int32_t ExtendData{};  /* = 3 */
    bool has_ExtendData = false;
};

struct PB_MakeCustomModelCH {
    PB_Vector3 Point{};  /* = 1 */
    bool has_Point = false;
    std::string ModelName{};  /* = 2 */
    bool has_ModelName = false;
    std::string ModelDesc{};  /* = 3 */
    bool has_ModelDesc = false;
    int32_t ModelType{};  /* = 4 */
    bool has_ModelType = false;
};

struct PB_SelectMobSpawnerCH {
    PB_Vector3 Point{};  /* = 1 */
    bool has_Point = false;
    int32_t MobResID{};  /* = 2 */
    bool has_MobResID = false;
    int32_t SpawnCount{};  /* = 3 */
    bool has_SpawnCount = false;
    int32_t MaxNearbyMobs{};  /* = 4 */
    bool has_MaxNearbyMobs = false;
    int32_t SpawnWide{};  /* = 5 */
    bool has_SpawnWide = false;
    int32_t SpawnHigh{};  /* = 6 */
    bool has_SpawnHigh = false;
    int32_t MinSpawnDelay{};  /* = 7 */
    bool has_MinSpawnDelay = false;
    int32_t IsNumberDetection{};  /* = 8 */
    bool has_IsNumberDetection = false;
    int32_t IsSpawnDelay{};  /* = 9 */
    bool has_IsSpawnDelay = false;
};

struct PB_GetNpcShopInfoCH {
    int32_t ShopID{};  /* = 1 */
    bool has_ShopID = false;
};

struct PB_BuyNpcShopItemCH {
    int32_t ShopID{};  /* = 1 */
    bool has_ShopID = false;
    int32_t SkuID{};  /* = 2 */
    bool has_SkuID = false;
    int32_t BuyCount{};  /* = 3 */
    bool has_BuyCount = false;
};

struct PB_ActorOneAvatarModelData {
    std::string ModelFilename{};  /* = 1 */
    bool has_ModelFilename = false;
    float Scale{};  /* = 2 */
    bool has_Scale = false;
    int32_t Yaw{};  /* = 3 */
    bool has_Yaw = false;
    int32_t Pitch{};  /* = 4 */
    bool has_Pitch = false;
    PB_Vector3 OffsetPos{};  /* = 5 */
    bool has_OffsetPos = false;
    int32_t Roll{};  /* = 6 */
    bool has_Roll = false;
    bool NewRotateMode{};  /* = 7 */
    bool has_NewRotateMode = false;
};

struct PB_ActorOneBoneModelData {
    std::string BoneName{};  /* = 1 */
    bool has_BoneName = false;
    std::vector<PB_ActorOneAvatarModelData> AvatarModels;  /* = 2 */
};

struct PB_CloseEditActorModelCH {
    std::vector<PB_ActorOneBoneModelData> BoneModels;  /* = 1 */
    int32_t OperateType{};  /* = 2 */
    bool has_OperateType = false;
    int32_t ModelType{};  /* = 3 */
    bool has_ModelType = false;
    std::string ModelName{};  /* = 4 */
    bool has_ModelName = false;
    bool SkinDisplay{};  /* = 5 */
    bool has_SkinDisplay = false;
};

struct PB_PackGiftNotifyItemChgCH {
    int32_t ShortCutIdx{};  /* = 1 */
    bool has_ShortCutIdx = false;
    int32_t CostItemInfo{};  /* = 2 */
    bool has_CostItemInfo = false;
    std::vector<int32_t> addlist;  /* = 3 */
};

struct PB_VehiclePreBlockCH {
    PB_Vector3 blockpos{};  /* = 1 */
    bool has_blockpos = false;
};

struct PB_VehicleItemUseCH {
    int32_t ShortCutIdx{};  /* = 1 */
    bool has_ShortCutIdx = false;
    int32_t dir{};  /* = 2 */
    bool has_dir = false;
    PB_Vector3 pos{};  /* = 3 */
    bool has_pos = false;
};

struct PB_VehicleStartBlockCH {
    PB_Vector3 blockpos{};  /* = 1 */
    bool has_blockpos = false;
};

struct PB_VehicleAttribChangeCH {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t Fuel{};  /* = 2 */
    bool has_Fuel = false;
    int32_t PartIndex{};  /* = 3 */
    bool has_PartIndex = false;
    int32_t EngineState{};  /* = 4 */
    bool has_EngineState = false;
};

struct PB_WorkshopItemInfoCH {
    PB_Vector3 ContainerPos{};  /* = 1 */
    bool has_ContainerPos = false;
};

struct PB_PlayerVehicleMoveInputCH {
    float Accel{};  /* = 1 */
    bool has_Accel = false;
    float Brake{};  /* = 2 */
    bool has_Brake = false;
    float Left{};  /* = 3 */
    bool has_Left = false;
    float Right{};  /* = 4 */
    bool has_Right = false;
};

struct PB_PlayerResetVehicleCH {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
};

struct PB_PlayerMotionStateChangeCH {
    int32_t StateType{};  /* = 1 */
    bool has_StateType = false;
    bool StateSwitch{};  /* = 2 */
    bool has_StateSwitch = false;
};

struct PB_PlayerClickCH {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t ActorID{};  /* = 2 */
    bool has_ActorID = false;
    int32_t BlockID{};  /* = 3 */
    bool has_BlockID = false;
    PB_Vector3 BlockPos{};  /* = 4 */
    bool has_BlockPos = false;
};

struct PB_PlayerSelectShortcutCH {
    uint64_t objid{};  /* = 1 */
    bool has_objid = false;
    int32_t toolID{};  /* = 2 */
    bool has_toolID = false;
    int32_t itemNum{};  /* = 3 */
    bool has_itemNum = false;
};

struct PB_TRIGGERPLAYERATTRICH {
    std::vector<int32_t> ObActorAttrList;  /* = 1 */
};

struct PB_ReqDownLoadResUrlCH {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    std::string ExternData{};  /* = 2 */
    bool has_ExternData = false;
};

struct PB_CloseFullyCustomModelUICH {
    int32_t OperateType{};  /* = 1 */
    bool has_OperateType = false;
    std::string Url{};  /* = 2 */
    bool has_Url = false;
    std::string Skey{};  /* = 3 */
    bool has_Skey = false;
    int32_t version{};  /* = 4 */
    bool has_version = false;
    std::string Name{};  /* = 5 */
    bool has_Name = false;
    std::string Desc{};  /* = 6 */
    bool has_Desc = false;
};

struct PB_PlayerNavFinishedCH {
    int32_t objid{};  /* = 1 */
    bool has_objid = false;
};

struct PB_VehicleAssembleLineCH {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t from{};  /* = 2 */
    bool has_from = false;
    int32_t to{};  /* = 3 */
    bool has_to = false;
};

struct PB_VehicleAssembleLineOperateCH {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t Blockid{};  /* = 2 */
    bool has_Blockid = false;
    PB_Vector3 BlockPos{};  /* = 3 */
    bool has_BlockPos = false;
    bool IsClicked{};  /* = 4 */
    bool has_IsClicked = false;
    int32_t Type{};  /* = 5 */
    bool has_Type = false;
    int32_t KeyId{};  /* = 6 */
    bool has_KeyId = false;
};

struct PB_UpdateActionerDataCH {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    PB_Vector3 BlockPos{};  /* = 2 */
    bool has_BlockPos = false;
    std::string datastr{};  /* = 3 */
    bool has_datastr = false;
};

struct PB_CSPlayerPermitCH {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t Bit{};  /* = 2 */
    bool has_Bit = false;
    bool BitVal{};  /* = 3 */
    bool has_BitVal = false;
    int32_t BanItem{};  /* = 4 */
    bool has_BanItem = false;
    bool IsBan{};  /* = 5 */
    bool has_IsBan = false;
};

struct PB_VehicleWorkshopLineCH {
    PB_Vector3 ContainerPos{};  /* = 1 */
    bool has_ContainerPos = false;
    PB_Vector3 FromPos{};  /* = 2 */
    bool has_FromPos = false;
    PB_Vector3 ToPos{};  /* = 3 */
    bool has_ToPos = false;
};

struct PB_CSChangePlayerTeamCH {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t TeamId{};  /* = 2 */
    bool has_TeamId = false;
};

struct PB_CSRentRoomAutoMuteCH {
    uint32_t SpamPreventionMinutes{};  /* = 1 */
    bool has_SpamPreventionMinutes = false;
};

struct PB_VehicleAssembleLineUpdateCH {
    PB_Vector3 BlockPos{};  /* = 1 */
    bool has_BlockPos = false;
};

struct PB_MapEditHandleCH {
    int32_t EditMode{};  /* = 1 */
    bool has_EditMode = false;
    int32_t ShapeType{};  /* = 2 */
    bool has_ShapeType = false;
    int32_t HandleType{};  /* = 3 */
    bool has_HandleType = false;
    int32_t ExtraParamLength{};  /* = 4 */
    bool has_ExtraParamLength = false;
    int32_t ExtraParamWidth{};  /* = 5 */
    bool has_ExtraParamWidth = false;
    int32_t ExtraParamHeight{};  /* = 6 */
    bool has_ExtraParamHeight = false;
    int32_t ExtraParamRadius{};  /* = 7 */
    bool has_ExtraParamRadius = false;
    bool ExtraParamIsSolid{};  /* = 8 */
    bool has_ExtraParamIsSolid = false;
    int32_t ExtraParamScale{};  /* = 9 */
    bool has_ExtraParamScale = false;
    bool ExtraParamIsContinue{};  /* = 10 */
    bool has_ExtraParamIsContinue = false;
    int32_t ExtraParamDir{};  /* = 11 */
    bool has_ExtraParamDir = false;
    int32_t ExtraParamFillerId{};  /* = 12 */
    bool has_ExtraParamFillerId = false;
    int32_t ExtraParamReplaceId{};  /* = 13 */
    bool has_ExtraParamReplaceId = false;
    int32_t ExtraParamDistance{};  /* = 14 */
    bool has_ExtraParamDistance = false;
    int32_t ExtraParamOffsetX{};  /* = 15 */
    bool has_ExtraParamOffsetX = false;
    int32_t ExtraParamOffsetY{};  /* = 16 */
    bool has_ExtraParamOffsetY = false;
    int32_t ExtraParamOffsetZ{};  /* = 17 */
    bool has_ExtraParamOffsetZ = false;
    int32_t ExtraParamRotateAngle{};  /* = 18 */
    bool has_ExtraParamRotateAngle = false;
    int32_t ExtraParamTwoPointCPDir{};  /* = 19 */
    bool has_ExtraParamTwoPointCPDir = false;
    bool ExtraParamIsOnlyFillAtmosphere{};  /* = 20 */
    bool has_ExtraParamIsOnlyFillAtmosphere = false;
    PB_Vector3 CenterPos{};  /* = 21 */
    bool has_CenterPos = false;
    PB_Vector3 BeginPos{};  /* = 22 */
    bool has_BeginPos = false;
    PB_Vector3 EndPos{};  /* = 23 */
    bool has_EndPos = false;
    int32_t SaveCmd{};  /* = 24 */
    bool has_SaveCmd = false;
};

struct PB_MapEditRevokeCH {
    bool Revoke{};  /* = 1 */
    bool has_Revoke = false;
};

struct PB_CloudRoomOwnerStartGameCH {
};

struct PB_CSKickOffDataCH {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t KickerType{};  /* = 2 */
    bool has_KickerType = false;
};

struct PB_UsePackingFCMItemCH {
    int32_t ItemId{};  /* = 1 */
    bool has_ItemId = false;
    PB_Vector3 UsePos{};  /* = 2 */
    bool has_UsePos = false;
};

struct PB_CreatePackingCMCH {
    PB_Vector3 OffsetPos{};  /* = 1 */
    bool has_OffsetPos = false;
    int32_t RotateType{};  /* = 2 */
    bool has_RotateType = false;
    std::string Name{};  /* = 3 */
    bool has_Name = false;
    std::string Desc{};  /* = 4 */
    bool has_Desc = false;
    int32_t CreateType{};  /* = 5 */
    bool has_CreateType = false;
    PB_Vector3 StartPos{};  /* = 6 */
    bool has_StartPos = false;
    PB_Vector3 EndPos{};  /* = 7 */
    bool has_EndPos = false;
};

struct PB_InputContentCH {
    uint64_t objId{};  /* = 1 */
    bool has_objId = false;
    std::string content{};  /* = 2 */
    bool has_content = false;
};

struct PB_InputKeyCH {
    uint64_t objId{};  /* = 1 */
    bool has_objId = false;
    int32_t keyType{};  /* = 2 */
    bool has_keyType = false;
    std::string eventType{};  /* = 3 */
    bool has_eventType = false;
};

struct PB_SensorContainerDataCH {
    PB_Vector3 BlockPos{};  /* = 1 */
    bool has_BlockPos = false;
    int32_t SensorValue{};  /* = 2 */
    bool has_SensorValue = false;
    bool IsBreverse{};  /* = 3 */
    bool has_IsBreverse = false;
    uint64_t ObjID{};  /* = 4 */
    bool has_ObjID = false;
};

struct PB_PlayerCarryActorCH {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    PB_Vector3 Pos{};  /* = 2 */
    bool has_Pos = false;
};

struct PB_VillagerModifyName {
    uint64_t ObjId{};  /* = 1 */
    bool has_ObjId = false;
    std::string Name{};  /* = 2 */
    bool has_Name = false;
};

struct PB_PlayerGotoPosCH {
    uint64_t ObjId{};  /* = 1 */
    bool has_ObjId = false;
    PB_Vector3 Pos{};  /* = 2 */
    bool has_Pos = false;
};

struct PB_CustomModelPrepareCH {
    int32_t Index{};  /* = 2 */
    bool has_Index = false;
    int32_t HaveFile{};  /* = 3 */
    bool has_HaveFile = false;
};

struct PB_MoveMobBackpackItemCH {
    int32_t gridIndex{};  /* = 1 */
    bool has_gridIndex = false;
    int32_t moveType{};  /* = 2 */
    bool has_moveType = false;
};

struct PB_InteractMobBackpackItemCH {
    int32_t fromIndex{};  /* = 1 */
    bool has_fromIndex = false;
    int32_t toIndex{};  /* = 2 */
    bool has_toIndex = false;
};

struct PB_AltarLuckyDrawCH {
    PB_Vector3 Pos{};  /* = 1 */
    bool has_Pos = false;
};

struct PB_PlayerRestoreTransformSkinCH {
};

struct PB_PlayerDeformationSkinCH {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    bool IsShapeShift{};  /* = 2 */
    bool has_IsShapeShift = false;
};

struct PB_PlayerResetDeformationCH {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    bool IsShapeShift{};  /* = 2 */
    bool has_IsShapeShift = false;
};

struct PB_PlayerBaseAttrCH {
    float CurHP{};  /* = 1 */
    bool has_CurHP = false;
    float MaxHP{};  /* = 2 */
    bool has_MaxHP = false;
    float CurFoodLv{};  /* = 3 */
    bool has_CurFoodLv = false;
    float MaxFoodLv{};  /* = 4 */
    bool has_MaxFoodLv = false;
    float BaseSpeed{};  /* = 5 */
    bool has_BaseSpeed = false;
    float AttackPunch{};  /* = 6 */
    bool has_AttackPunch = false;
    float AttackRange{};  /* = 7 */
    bool has_AttackRange = false;
    float DefensePunch{};  /* = 8 */
    bool has_DefensePunch = false;
    float DefenseRange{};  /* = 9 */
    bool has_DefenseRange = false;
    int32_t CurExp{};  /* = 10 */
    bool has_CurExp = false;
    int32_t CurSexp{};  /* = 11 */
    bool has_CurSexp = false;
    int32_t CurLevel{};  /* = 12 */
    bool has_CurLevel = false;
};

struct PB_ActorBuff {
    uint32_t BuffID{};  /* = 1 */
    bool has_BuffID = false;
    uint32_t BuffLV{};  /* = 2 */
    bool has_BuffLV = false;
    int32_t Ticks{};  /* = 3 */
    bool has_Ticks = false;
    int32_t BuffInstanceId{};  /* = 4 */
    bool has_BuffInstanceId = false;
};

struct PB_ItemData {
    int32_t Index{};  /* = 1 */
    bool has_Index = false;
    int32_t ItemID{};  /* = 2 */
    bool has_ItemID = false;
    int32_t Durable{};  /* = 3 */
    bool has_Durable = false;
    uint32_t Num{};  /* = 4 */
    bool has_Num = false;
    int32_t UserData{};  /* = 5 */
    bool has_UserData = false;
    std::vector<int32_t> Enchs;  /* = 6 */
    std::string UserDataStr{};  /* = 7 */
    bool has_UserDataStr = false;
};

struct PB_PlayerArchEntityCH {
    PB_PlayerBaseAttrCH BaseAttr{};  /* = 1 */
    bool has_BaseAttr = false;
    std::vector<PB_ActorBuff> RoleBuffs;  /* = 2 */
    std::vector<PB_ItemData> RoleEquips;  /* = 3 */
    std::vector<PB_ItemData> RoleBackpack;  /* = 4 */
    std::vector<PB_ItemData> RoleShortcut;  /* = 5 */
    std::string CloudVars{};  /* = 6 */
    bool has_CloudVars = false;
};

struct PB_PrayTreeTimeCH {
    int32_t uin{};  /* = 1 */
    bool has_uin = false;
    int32_t stage{};  /* = 2 */
    bool has_stage = false;
};

struct PB_SUMMONPETCH {
    int32_t monsterid{};  /* = 1 */
    bool has_monsterid = false;
    std::string serverid{};  /* = 2 */
    bool has_serverid = false;
    int32_t petid{};  /* = 3 */
    bool has_petid = false;
    int32_t stage{};  /* = 4 */
    bool has_stage = false;
    int32_t quality{};  /* = 5 */
    bool has_quality = false;
};

struct PB_HomeLandRanchUpdateAnimalStateCH {
    uint64_t objid{};  /* = 1 */
    bool has_objid = false;
    int32_t enterstate{};  /* = 2 */
    bool has_enterstate = false;
    std::string serverid{};  /* = 3 */
    bool has_serverid = false;
};

struct PB_RequestModelName {
    std::string modelID{};  /* = 1 */
    bool has_modelID = false;
};

struct PB_Quaternion {
    float X{};  /* = 1 */
    bool has_X = false;
    float Y{};  /* = 2 */
    bool has_Y = false;
    float Z{};  /* = 3 */
    bool has_Z = false;
    float W{};  /* = 4 */
    bool has_W = false;
};

struct PB_Pos {
    int32_t X{};  /* = 1 */
    bool has_X = false;
    int32_t Y{};  /* = 2 */
    bool has_Y = false;
    int32_t Z{};  /* = 3 */
    bool has_Z = false;
    uint32_t Map{};  /* = 4 */
    bool has_Map = false;
};

struct PB_BodyDir {
    float RotationYaw{};  /* = 1 */
    bool has_RotationYaw = false;
    float RotationPitch{};  /* = 2 */
    bool has_RotationPitch = false;
    PB_Vector3 Motion{};  /* = 3 */
    bool has_Motion = false;
};

struct PB_Item {
    uint32_t DefID{};  /* = 1 */
    bool has_DefID = false;
    uint32_t Idx{};  /* = 2 */
    bool has_Idx = false;
    uint32_t Pile{};  /* = 3 */
    bool has_Pile = false;
    std::string UserDataStr{};  /* = 4 */
    bool has_UserDataStr = false;
};

struct PB_ArmFumo {
    std::vector<int32_t> FumoIDs;  /* = 1 */
};

struct PB_Arm {
    uint32_t DefID{};  /* = 1 */
    bool has_DefID = false;
    uint32_t Idx{};  /* = 2 */
    bool has_Idx = false;
    uint32_t Color{};  /* = 3 */
    bool has_Color = false;
    uint32_t Dur{};  /* = 4 */
    bool has_Dur = false;
    uint32_t RepairNum{};  /* = 5 */
    bool has_RepairNum = false;
    std::string Name{};  /* = 6 */
    bool has_Name = false;
    PB_ArmFumo Fumo{};  /* = 7 */
    bool has_Fumo = false;
};

struct PB_ItemSpecial {
    int32_t SpecailTypeID{};  /* = 1 */
    bool has_SpecailTypeID = false;
    std::string Buff{};  /* = 2 */
    bool has_Buff = false;
};

struct PB_ItemGridData {
    PB_Item Item{};  /* = 1 */
    bool has_Item = false;
    PB_Arm Arm{};  /* = 2 */
    bool has_Arm = false;
    PB_ItemSpecial ItemSpecial{};  /* = 3 */
    bool has_ItemSpecial = false;
};

struct PB_ItemGrid {
    PB_BodyDir Dir{};  /* = 1 */
    bool has_Dir = false;
    float FallDistance{};  /* = 2 */
    bool has_FallDistance = false;
    int32_t SpecialFlag{};  /* = 3 */
    bool has_SpecialFlag = false;
    int32_t Type{};  /* = 4 */
    bool has_Type = false;
    PB_ItemGridData ItemGridData{};  /* = 5 */
    bool has_ItemGridData = false;
};

struct PB_RoleBackpack {
    int32_t PackTmp{};  /* = 1 */
    bool has_PackTmp = false;
};

struct PB_ChunkBlob {
    int32_t UnzipLen{};  /* = 1 */
    bool has_UnzipLen = false;
    int32_t BlobLen{};  /* = 2 */
    bool has_BlobLen = false;
    std::string BlobDetail{};  /* = 3 */
    bool has_BlobDetail = false;
};

struct PB_ChunkSaveDB {
    uint64_t OWID{};  /* = 1 */
    bool has_OWID = false;
    uint32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    int32_t x{};  /* = 3 */
    bool has_x = false;
    int32_t z{};  /* = 4 */
    bool has_z = false;
    int32_t Version{};  /* = 5 */
    bool has_Version = false;
    int32_t ShareFlag{};  /* = 6 */
    bool has_ShareFlag = false;
    PB_ChunkBlob ChunkBlob{};  /* = 7 */
    bool has_ChunkBlob = false;
};

struct PB_GridPak {
    std::vector<PB_ItemGrid> Grids;  /* = 1 */
};

struct PB_ShortcutPak {
    int32_t HandIdx{};  /* = 1 */
    bool has_HandIdx = false;
    std::vector<PB_ItemGrid> Grids;  /* = 2 */
};

struct PB_ArmPak {
    std::vector<PB_ItemGrid> Grids;  /* = 1 */
};

struct PB_BuffAttr {
    uint32_t AttrID{};  /* = 1 */
    bool has_AttrID = false;
    float Val{};  /* = 2 */
    bool has_Val = false;
};

struct PB_RolePackage {
    PB_GridPak GridPak{};  /* = 1 */
    bool has_GridPak = false;
    PB_ShortcutPak ShortcutPak{};  /* = 2 */
    bool has_ShortcutPak = false;
    PB_ArmPak ArmPak{};  /* = 3 */
    bool has_ArmPak = false;
    PB_Pos RevicePos{};  /* = 4 */
    bool has_RevicePos = false;
};

struct PB_ActorBuffList {
    std::vector<PB_ActorBuff> Buffs;  /* = 1 */
    std::vector<PB_BuffAttr> RelBuffAttrs;  /* = 2 */
    std::vector<PB_BuffAttr> AbsBuffAttrs;  /* = 3 */
};

struct PB_RoleData {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    uint64_t OWID{};  /* = 2 */
    bool has_OWID = false;
    int32_t HP{};  /* = 3 */
    bool has_HP = false;
    int32_t Oxygen{};  /* = 4 */
    bool has_Oxygen = false;
    int32_t FoodLevel{};  /* = 5 */
    bool has_FoodLevel = false;
    int32_t FoodSatLevel{};  /* = 6 */
    bool has_FoodSatLevel = false;
    int32_t UsedStamina{};  /* = 7 */
    bool has_UsedStamina = false;
    int32_t Exp{};  /* = 8 */
    bool has_Exp = false;
    int32_t Level{};  /* = 9 */
    bool has_Level = false;
    int32_t LastLoginTime{};  /* = 10 */
    bool has_LastLoginTime = false;
    int32_t LoginNum{};  /* = 11 */
    bool has_LoginNum = false;
    float FallDist{};  /* = 12 */
    bool has_FallDist = false;
    int32_t Flags{};  /* = 13 */
    bool has_Flags = false;
    int32_t LiveTicks{};  /* = 14 */
    bool has_LiveTicks = false;
    uint64_t RideActorID{};  /* = 15 */
    bool has_RideActorID = false;
    PB_Pos Pos{};  /* = 16 */
    bool has_Pos = false;
    PB_BodyDir Dir{};  /* = 17 */
    bool has_Dir = false;
    PB_RolePackage Package{};  /* = 18 */
    bool has_Package = false;
    PB_ActorBuffList Buff{};  /* = 19 */
    bool has_Buff = false;
    uint64_t CarringActorID{};  /* = 20 */
    bool has_CarringActorID = false;
};

struct PB_BodyEffectBrief {
    int32_t effectID{};  /* = 1 */
    bool has_effectID = false;
    float effectScale{};  /* = 2 */
    bool has_effectScale = false;
    int32_t effectClass{};  /* = 3 */
    bool has_effectClass = false;
};

struct PB_EffectTriggerSound {
    std::string Name{};  /* = 1 */
    bool has_Name = false;
    float Volume{};  /* = 2 */
    bool has_Volume = false;
    float Pitch{};  /* = 3 */
    bool has_Pitch = false;
    bool IsLoop{};  /* = 4 */
    bool has_IsLoop = false;
    int32_t PlayState{};  /* = 5 */
    bool has_PlayState = false;
    uint64_t ObjId{};  /* = 6 */
    bool has_ObjId = false;
    PB_Vector3 Pos{};  /* = 7 */
    bool has_Pos = false;
    int32_t position{};  /* = 8 */
    bool has_position = false;
};

struct PB_PlayerInfo {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t anim{};  /* = 2 */
    bool has_anim = false;
    int32_t anim1{};  /* = 3 */
    bool has_anim1 = false;
    PB_RoleData RoleData{};  /* = 4 */
    bool has_RoleData = false;
    uint32_t BodyColor{};  /* = 5 */
    bool has_BodyColor = false;
    float bodyscale_invalid{};  /* = 6 */
    bool has_bodyscale_invalid = false;
    std::vector<PB_BodyEffectBrief> effectList;  /* = 7 */
    float customscale{};  /* = 8 */
    bool has_customscale = false;
    std::vector<PB_EffectTriggerSound> soundList;  /* = 9 */
    int32_t actSeqId{};  /* = 10 */
    bool has_actSeqId = false;
    int32_t accountSkinID{};  /* = 11 */
    bool has_accountSkinID = false;
};

struct PB_RedStoneIdx {
    int32_t x{};  /* = 1 */
    bool has_x = false;
    int32_t z{};  /* = 2 */
    bool has_z = false;
};

struct PB_RedStoneData {
    std::vector<PB_RedStoneIdx> RedStoneIdxList;  /* = 1 */
};

struct PB_GlobalBin {
    int32_t BinLen{};  /* = 1 */
    bool has_BinLen = false;
    std::string BinContent{};  /* = 2 */
    bool has_BinContent = false;
};

struct PB_OWGlobalMisc {
    int32_t GlobalFlag{};  /* = 1 */
    bool has_GlobalFlag = false;
    int32_t ChunkVer{};  /* = 2 */
    bool has_ChunkVer = false;
    int32_t ChunkVerBroadCast{};  /* = 3 */
    bool has_ChunkVerBroadCast = false;
    PB_Pos InitPos{};  /* = 4 */
    bool has_InitPos = false;
    PB_Pos RevicePos{};  /* = 5 */
    bool has_RevicePos = false;
    PB_RedStoneData RedStoneData{};  /* = 6 */
    bool has_RedStoneData = false;
    PB_GlobalBin GlobalBin{};  /* = 7 */
    bool has_GlobalBin = false;
};

struct PB_OWGlobal {
    uint64_t OWID{};  /* = 1 */
    bool has_OWID = false;
    int32_t ID{};  /* = 2 */
    bool has_ID = false;
    int32_t Uin{};  /* = 3 */
    bool has_Uin = false;
    int32_t SvrStart{};  /* = 4 */
    bool has_SvrStart = false;
    int32_t GridChgNum{};  /* = 5 */
    bool has_GridChgNum = false;
    PB_OWGlobalMisc Misc{};  /* = 6 */
    bool has_Misc = false;
};

struct PB_WorldCreateData {
    int32_t TerrType{};  /* = 1 */
    bool has_TerrType = false;
    uint32_t RandSeed1{};  /* = 2 */
    bool has_RandSeed1 = false;
    uint32_t RandSeed2{};  /* = 3 */
    bool has_RandSeed2 = false;
    int32_t RoleModel{};  /* = 4 */
    bool has_RoleModel = false;
    std::string SeedStr{};  /* = 5 */
    bool has_SeedStr = false;
    uint32_t TilesX{};  /* = 6 */
    bool has_TilesX = false;
    uint32_t TilesZ{};  /* = 7 */
    bool has_TilesZ = false;
};

struct PB_WorldDesc {
    uint64_t WorldId{};  /* = 1 */
    bool has_WorldId = false;
    int32_t WorldType{};  /* = 2 */
    bool has_WorldType = false;
    int32_t OwnerUin{};  /* = 3 */
    bool has_OwnerUin = false;
    PB_WorldCreateData CreateData{};  /* = 4 */
    bool has_CreateData = false;
    int32_t DeveloperFlag{};  /* = 5 */
    bool has_DeveloperFlag = false;
    uint64_t FromOWID{};  /* = 6 */
    bool has_FromOWID = false;
    int32_t RealOwnerUin{};  /* = 7 */
    bool has_RealOwnerUin = false;
    int32_t WorldOpen{};  /* = 8 */
    bool has_WorldOpen = false;
    std::string WorldName{};  /* = 9 */
    bool has_WorldName = false;
    int32_t TempType{};  /* = 10 */
    bool has_TempType = false;
    int64_t pwid{};  /* = 11 */
    bool has_pwid = false;
    uint32_t SpecialType{};  /* = 12 */
    bool has_SpecialType = false;
};

struct PB_SkillCDData {
    int32_t NumSkillCD{};  /* = 1 */
    bool has_NumSkillCD = false;
    std::vector<int32_t> ItemID;  /* = 2 */
    std::vector<float> CD;  /* = 3 */
};

struct PB_ActorRoleInfo {
    PB_RoleInfo Info{};  /* = 1 */
    bool has_Info = false;
    PB_PlayerInfo Player{};  /* = 2 */
    bool has_Player = false;
};

struct PB_ActorGeneral {
    int32_t MapId{};  /* = 1 */
    bool has_MapId = false;
    int32_t InfoLen{};  /* = 2 */
    bool has_InfoLen = false;
    std::string ActorDetail{};  /* = 3 */
    bool has_ActorDetail = false;
};

struct PB_ActorInfo {
    PB_ActorGeneral ActorGeneral{};  /* = 1 */
    bool has_ActorGeneral = false;
    PB_ActorRoleInfo RoleInfo{};  /* = 2 */
    bool has_RoleInfo = false;
};

struct PB_PlayerBriefInfo {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    int32_t HP{};  /* = 3 */
    bool has_HP = false;
    std::string NickName{};  /* = 4 */
    bool has_NickName = false;
    int32_t PlayerIndex{};  /* = 5 */
    bool has_PlayerIndex = false;
    int32_t TeamID{};  /* = 6 */
    bool has_TeamID = false;
    std::vector<int32_t> CGVars;  /* = 7 */
    int32_t InSpectator{};  /* = 8 */
    bool has_InSpectator = false;
    PB_Vector3 Pos{};  /* = 9 */
    bool has_Pos = false;
    PB_PlayerVipInfo VipInfo{};  /* = 10 */
    bool has_VipInfo = false;
    std::string CustomJson{};  /* = 11 */
    bool has_CustomJson = false;
    int32_t SkinID{};  /* = 12 */
    bool has_SkinID = false;
    int32_t FrameID{};  /* = 13 */
    bool has_FrameID = false;
    std::string CustomModel{};  /* = 14 */
    bool has_CustomModel = false;
    int32_t AcctountSkinID{};  /* = 15 */
    bool has_AcctountSkinID = false;
};

struct PB_EffectParticle {
    std::string Name{};  /* = 1 */
    bool has_Name = false;
    int32_t Age{};  /* = 2 */
    bool has_Age = false;
    int32_t Color{};  /* = 3 */
    bool has_Color = false;
    float Yaw{};  /* = 4 */
    bool has_Yaw = false;
    float Pitch{};  /* = 5 */
    bool has_Pitch = false;
    PB_Vector3 Pos{};  /* = 6 */
    bool has_Pos = false;
};

struct PB_EffectPickItem {
    uint64_t PickerObj{};  /* = 1 */
    bool has_PickerObj = false;
    uint64_t ItemObj{};  /* = 2 */
    bool has_ItemObj = false;
    int32_t YOffset{};  /* = 3 */
    bool has_YOffset = false;
};

struct PB_EffectSound {
    std::string Name{};  /* = 1 */
    bool has_Name = false;
    float Volume{};  /* = 2 */
    bool has_Volume = false;
    float Pitch{};  /* = 3 */
    bool has_Pitch = false;
    int32_t Flags{};  /* = 4 */
    bool has_Flags = false;
    int32_t Segment{};  /* = 5 */
    bool has_Segment = false;
    PB_Vector3 Pos{};  /* = 6 */
    bool has_Pos = false;
};

struct PB_EffectActorBody {
    uint64_t ObjId{};  /* = 1 */
    bool has_ObjId = false;
    int32_t BodyEffect{};  /* = 2 */
    bool has_BodyEffect = false;
    int32_t Status{};  /* = 3 */
    bool has_Status = false;
};

struct PB_EffectStringActorBody {
    uint64_t ObjId{};  /* = 1 */
    bool has_ObjId = false;
    std::string EffectName{};  /* = 2 */
    bool has_EffectName = false;
    int32_t Status{};  /* = 3 */
    bool has_Status = false;
};

struct PB_EffectDestroyBlock {
    int32_t Face{};  /* = 1 */
    bool has_Face = false;
    int32_t SubType{};  /* = 2 */
    bool has_SubType = false;
    int32_t Age{};  /* = 3 */
    bool has_Age = false;
    PB_Vector3 Pos{};  /* = 4 */
    bool has_Pos = false;
    int32_t ID{};  /* = 5 */
    bool has_ID = false;
};

struct PB_EffectCrackBlock {
    int32_t Stage{};  /* = 1 */
    bool has_Stage = false;
    uint64_t ActorId{};  /* = 2 */
    bool has_ActorId = false;
    PB_Vector3 BlockPos{};  /* = 3 */
    bool has_BlockPos = false;
};

struct PB_EffectVehicle {
    std::string Name{};  /* = 1 */
    bool has_Name = false;
    uint64_t ActorId{};  /* = 2 */
    bool has_ActorId = false;
    PB_Vector3 BlockPos{};  /* = 3 */
    bool has_BlockPos = false;
    int32_t Age{};  /* = 4 */
    bool has_Age = false;
};

struct PB_EffectPlayMusicGrid {
    std::string Name{};  /* = 1 */
    bool has_Name = false;
    float Pitch{};  /* = 2 */
    bool has_Pitch = false;
    int32_t Flags{};  /* = 3 */
    bool has_Flags = false;
    int32_t Segment{};  /* = 4 */
    bool has_Segment = false;
    PB_Vector3 BlockPos{};  /* = 5 */
    bool has_BlockPos = false;
};

struct PB_EffectStopMusicGrid {
    PB_Vector3 BlockPos{};  /* = 1 */
    bool has_BlockPos = false;
};

struct PB_IntertactData {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    int32_t ID{};  /* = 2 */
    bool has_ID = false;
    int32_t Show{};  /* = 3 */
    bool has_Show = false;
};

struct PB_TaskContentData {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    int32_t ID{};  /* = 2 */
    bool has_ID = false;
    int32_t Num{};  /* = 3 */
    bool has_Num = false;
    int32_t CompletedNum{};  /* = 4 */
    bool has_CompletedNum = false;
};

struct PB_TaskInfoData {
    int32_t ID{};  /* = 1 */
    bool has_ID = false;
    int32_t State{};  /* = 2 */
    bool has_State = false;
    int32_t PlotID{};  /* = 3 */
    bool has_PlotID = false;
    std::vector<PB_TaskContentData> Contents;  /* = 4 */
};

struct PB_PreBlockData {
    int32_t ID{};  /* = 1 */
    bool has_ID = false;
    PB_Vector3 BlockPos{};  /* = 2 */
    bool has_BlockPos = false;
};

struct PB_CustomModelClassData {
    std::string ClassName{};  /* = 1 */
    bool has_ClassName = false;
    std::vector<std::string> ModelNames;  /* = 2 */
    int32_t folderindex{};  /* = 3 */
    bool has_folderindex = false;
};

struct PB_MobDisplayData {
    int32_t mobId{};  /* = 1 */
    bool has_mobId = false;
    int32_t itemId{};  /* = 2 */
    bool has_itemId = false;
    int32_t itemCount{};  /* = 3 */
    bool has_itemCount = false;
    int32_t animId{};  /* = 4 */
    bool has_animId = false;
};

struct PB_NpcShopItemData {
    int32_t SkuID{};  /* = 1 */
    bool has_SkuID = false;
    int32_t ItemID{};  /* = 2 */
    bool has_ItemID = false;
    int32_t OnceBuyNum{};  /* = 3 */
    bool has_OnceBuyNum = false;
    int32_t MaxCanBuyCount{};  /* = 4 */
    bool has_MaxCanBuyCount = false;
    int32_t RefreshDuration{};  /* = 5 */
    bool has_RefreshDuration = false;
    int32_t StarNum{};  /* = 6 */
    bool has_StarNum = false;
    int32_t CostItemInfo1{};  /* = 7 */
    bool has_CostItemInfo1 = false;
    int32_t CostItemInfo2{};  /* = 8 */
    bool has_CostItemInfo2 = false;
    int32_t LeftCount{};  /* = 9 */
    bool has_LeftCount = false;
    int32_t EndTime{};  /* = 10 */
    bool has_EndTime = false;
};

struct PB_NpcShopData {
    int32_t ShopID{};  /* = 1 */
    bool has_ShopID = false;
    std::string ShopName{};  /* = 2 */
    bool has_ShopName = false;
    std::string ShopDesc{};  /* = 3 */
    bool has_ShopDesc = false;
    std::string InnerKey{};  /* = 4 */
    bool has_InnerKey = false;
    std::vector<PB_NpcShopItemData> ShopItemData;  /* = 5 */
};

struct PB_BasketBallOperate {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    uint64_t ActorID{};  /* = 2 */
    bool has_ActorID = false;
    bool IsSelectedTarget{};  /* = 3 */
    bool has_IsSelectedTarget = false;
    int32_t FallResult{};  /* = 4 */
    bool has_FallResult = false;
    int32_t ExtendData{};  /* = 5 */
    bool has_ExtendData = false;
    int32_t Uin{};  /* = 6 */
    bool has_Uin = false;
    PB_Vector3 pos{};  /* = 7 */
    bool has_pos = false;
    float yaw{};  /* = 8 */
    bool has_yaw = false;
    int32_t SelectedActorID{};  /* = 9 */
    bool has_SelectedActorID = false;
    float pitch{};  /* = 10 */
    bool has_pitch = false;
};

struct PB_CSAuthorityData {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t Type{};  /* = 2 */
    bool has_Type = false;
    uint32_t Flag{};  /* = 3 */
    bool has_Flag = false;
};

struct PB_CSPermitData {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t Type{};  /* = 2 */
    bool has_Type = false;
    uint32_t Flag{};  /* = 3 */
    bool has_Flag = false;
};

struct PB_Edu_RoleInfo {
    uint32_t objId{};  /* = 1 */
    bool has_objId = false;
    float yaw{};  /* = 2 */
    bool has_yaw = false;
};

struct PB_ImportModelData {
    std::string Key{};  /* = 1 */
    bool has_Key = false;
    std::string Name{};  /* = 2 */
    bool has_Name = false;
    std::string Desc{};  /* = 3 */
    bool has_Desc = false;
    int32_t Type{};  /* = 4 */
    bool has_Type = false;
    int32_t AuthUin{};  /* = 5 */
    bool has_AuthUin = false;
    std::string AuthName{};  /* = 6 */
    bool has_AuthName = false;
    std::string FileName{};  /* = 7 */
    bool has_FileName = false;
};

struct PB_GraphicsAttr {
    int32_t grapicsid{};  /* = 1 */
    bool has_grapicsid = false;
    int32_t Type{};  /* = 2 */
    bool has_Type = false;
    int32_t Groupid{};  /* = 3 */
    bool has_Groupid = false;
    std::string Title{};  /* = 4 */
    bool has_Title = false;
    int32_t Apha{};  /* = 5 */
    bool has_Apha = false;
    int32_t CurVal{};  /* = 6 */
    bool has_CurVal = false;
    int32_t MaxVal{};  /* = 7 */
    bool has_MaxVal = false;
    int32_t Color{};  /* = 8 */
    bool has_Color = false;
    int32_t Fontsize{};  /* = 9 */
    bool has_Fontsize = false;
    PB_Vector3 WordPos{};  /* = 10 */
    bool has_WordPos = false;
    uint64_t bindObjID{};  /* = 11 */
    bool has_bindObjID = false;
    int32_t x2{};  /* = 12 */
    bool has_x2 = false;
    int32_t y2{};  /* = 13 */
    bool has_y2 = false;
};

struct PB_MsgErrorHC {
    int32_t ErrCode{};  /* = 1 */
    bool has_ErrCode = false;
};

struct PB_HeartBeatHC {
    int32_t BeatCode{};  /* = 1 */
    bool has_BeatCode = false;
};

struct PB_SyncChunkDataHC {
    int32_t SectionFlags{};  /* = 1 */
    bool has_SectionFlags = false;
    int32_t Initialize{};  /* = 2 */
    bool has_Initialize = false;
    PB_ChunkSaveDB ChunkData{};  /* = 3 */
    bool has_ChunkData = false;
};

struct PB_BlockUpdateHC {
    int32_t ChunkX{};  /* = 1 */
    bool has_ChunkX = false;
    int32_t ChunkZ{};  /* = 2 */
    bool has_ChunkZ = false;
    int32_t MapID{};  /* = 3 */
    bool has_MapID = false;
    std::vector<uint32_t> Blocks;  /* = 4 */
    std::string ContainerBuf{};  /* = 5 */
    bool has_ContainerBuf = false;
};

struct PB_RoleEnterWorldHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    PB_PlayerInfo PlayerInfo{};  /* = 2 */
    bool has_PlayerInfo = false;
    PB_OWGlobal GlobalInfo{};  /* = 3 */
    bool has_GlobalInfo = false;
    PB_WorldDesc WorldDesc{};  /* = 4 */
    bool has_WorldDesc = false;
    PB_SkillCDData SkillCDData{};  /* = 5 */
    bool has_SkillCDData = false;
    std::vector<int32_t> UnlockItems;  /* = 6 */
    std::string Url{};  /* = 7 */
    bool has_Url = false;
    bool HasRole{};  /* = 8 */
    bool has_HasRole = false;
};

struct PB_RoleLeaveWorldHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
};

struct PB_ActorEnterAOIHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t ActorType{};  /* = 2 */
    bool has_ActorType = false;
    PB_ActorInfo ActorInfo{};  /* = 3 */
    bool has_ActorInfo = false;
    int32_t Spectator_Mode{};  /* = 4 */
    bool has_Spectator_Mode = false;
    int32_t Spectator_Type{};  /* = 5 */
    bool has_Spectator_Type = false;
    uint64_t HookID{};  /* = 6 */
    bool has_HookID = false;
    uint32_t PlayMode{};  /* = 7 */
    bool has_PlayMode = false;
    uint32_t PlayOperate{};  /* = 8 */
    bool has_PlayOperate = false;
    uint64_t childUUID{};  /* = 9 */
    bool has_childUUID = false;
};

struct AssembleBlockInfo {
    uint32_t Block{};  /* = 1 */
    bool has_Block = false;
    uint32_t data{};  /* = 2 */
    bool has_data = false;
    uint32_t Info{};  /* = 3 */
    bool has_Info = false;
};

struct PB_VehicleAssembleBlockUpdateHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    std::vector<AssembleBlockInfo> BlockInfo;  /* = 2 */
    std::string ContainerBuf{};  /* = 3 */
    bool has_ContainerBuf = false;
};

struct PB_VehicleAssembleBlockAllHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t UnzipLen{};  /* = 2 */
    bool has_UnzipLen = false;
    int32_t BlobLen{};  /* = 3 */
    bool has_BlobLen = false;
    std::string BlobDetail{};  /* = 4 */
    bool has_BlobDetail = false;
};

struct PB_ActorLeaveAOIHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
};

struct PB_ActorMoveHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    PB_MoveMotion MoveMotion{};  /* = 2 */
    bool has_MoveMotion = false;
};

struct PB_TrainMoveHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    uint32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    float CurveT{};  /* = 3 */
    bool has_CurveT = false;
    int32_t CarReverse{};  /* = 4 */
    bool has_CarReverse = false;
    uint64_t HeadCar{};  /* = 5 */
    bool has_HeadCar = false;
    uint64_t TailCar{};  /* = 6 */
    bool has_TailCar = false;
    uint32_t OutIndex{};  /* = 7 */
    bool has_OutIndex = false;
    PB_Vector3 RailKnot{};  /* = 8 */
    bool has_RailKnot = false;
};

struct PB_ActorTeleportHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t TargetMap{};  /* = 2 */
    bool has_TargetMap = false;
    PB_Vector3 TargetPos{};  /* = 3 */
    bool has_TargetPos = false;
};

struct PB_ActorMotionHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    float x{};  /* = 2 */
    bool has_x = false;
    float y{};  /* = 3 */
    bool has_y = false;
    float z{};  /* = 4 */
    bool has_z = false;
    bool isChangePos{};  /* = 5 */
    bool has_isChangePos = false;
};

struct PB_MechaMotionHC {
    uint64_t ObjId{};  /* = 1 */
    bool has_ObjId = false;
    int32_t MotionType{};  /* = 2 */
    bool has_MotionType = false;
    float MotionParam{};  /* = 3 */
    bool has_MotionParam = false;
};

struct PB_SyncTriggerBlock {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    PB_Vector3 BlockPos{};  /* = 2 */
    bool has_BlockPos = false;
};

struct PB_BlockInteractHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t face{};  /* = 2 */
    bool has_face = false;
    PB_Vector3 blockpos{};  /* = 3 */
    bool has_blockpos = false;
};

struct PB_BlockPunchHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t status{};  /* = 2 */
    bool has_status = false;
    int32_t face{};  /* = 3 */
    bool has_face = false;
    int32_t digmethod{};  /* = 4 */
    bool has_digmethod = false;
    PB_Vector3 blockpos{};  /* = 5 */
    bool has_blockpos = false;
};

struct PB_ItemUseHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t itemid{};  /* = 2 */
    bool has_itemid = false;
    int32_t status{};  /* = 3 */
    bool has_status = false;
    int32_t shift{};  /* = 4 */
    bool has_shift = false;
};

struct PB_SetHookHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    uint64_t hookID{};  /* = 2 */
    bool has_hookID = false;
};

struct PB_ItemSkillUseHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t itemid{};  /* = 2 */
    bool has_itemid = false;
    int32_t status{};  /* = 3 */
    bool has_status = false;
    int32_t skillid{};  /* = 4 */
    bool has_skillid = false;
};

struct PB_ActorInteractHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    uint64_t target{};  /* = 2 */
    bool has_target = false;
    int32_t itype{};  /* = 3 */
    bool has_itype = false;
    int32_t iplot{};  /* = 4 */
    bool has_iplot = false;
};

struct PB_ActorAnimHC {
    uint64_t actorid{};  /* = 1 */
    bool has_actorid = false;
    int32_t anim{};  /* = 2 */
    bool has_anim = false;
    int32_t anim1{};  /* = 3 */
    bool has_anim1 = false;
    int32_t actid{};  /* = 4 */
    bool has_actid = false;
    float bodyscale_invalid{};  /* = 5 */
    bool has_bodyscale_invalid = false;
    float customscale{};  /* = 6 */
    bool has_customscale = false;
    int32_t actidTrigger{};  /* = 7 */
    bool has_actidTrigger = false;
};

struct PB_BackPackGridUpdateHC {
    std::vector<PB_ItemData> ItemInfo;  /* = 1 */
};

struct PB_BackPackEquipWeaponHC {
    int32_t GridId{};  /* = 1 */
    bool has_GridId = false;
};

struct PB_CloseContainerHC {
    int32_t BaseIndex{};  /* = 1 */
    bool has_BaseIndex = false;
};

struct PB_OpenContainerHC {
    int32_t BaseIndex{};  /* = 1 */
    bool has_BaseIndex = false;
    int32_t TotalItemGrids{};  /* = 2 */
    bool has_TotalItemGrids = false;
    std::vector<float> AttribInfo;  /* = 3 */
    std::string Text{};  /* = 4 */
    bool has_Text = false;
    uint64_t NpcID{};  /* = 5 */
    bool has_NpcID = false;
    PB_Vector3 Pos{};  /* = 6 */
    bool has_Pos = false;
    std::vector<PB_ItemData> ItemInfo;  /* = 7 */
    uint64_t VehicleObjID{};  /* = 8 */
    bool has_VehicleObjID = false;
};

struct PB_NeedContainerPasswordHC {
    PB_Vector3 Pos{};  /* = 1 */
    bool has_Pos = false;
    int32_t state{};  /* = 2 */
    bool has_state = false;
    uint64_t VehicleObjID{};  /* = 3 */
    bool has_VehicleObjID = false;
};

struct PB_UpdateContainerHC {
    int32_t BaseIndex{};  /* = 1 */
    bool has_BaseIndex = false;
    std::vector<float> AttribInfo;  /* = 2 */
    std::string Text{};  /* = 3 */
    bool has_Text = false;
    PB_Vector3 Pos{};  /* = 4 */
    bool has_Pos = false;
    std::vector<PB_ItemData> ItemInfo;  /* = 5 */
};

struct PB_ActorEquipItemHC {
    uint64_t ObjId{};  /* = 1 */
    bool has_ObjId = false;
    int32_t SlotType{};  /* = 2 */
    bool has_SlotType = false;
    PB_ItemData ItemInfo{};  /* = 3 */
    bool has_ItemInfo = false;
};

struct PB_EnchantItemSuccessHC {
    int32_t GridIndex{};  /* = 1 */
    bool has_GridIndex = false;
};

struct PB_RepairItemSuccessHC {
    int32_t GridIndex{};  /* = 1 */
    bool has_GridIndex = false;
};

struct PB_GunDoReloadHC {
    int32_t BulletID{};  /* = 1 */
    bool has_BulletID = false;
    int32_t Num{};  /* = 2 */
    bool has_Num = false;
};

struct PB_AccountHorseHC {
    int32_t HorseID{};  /* = 1 */
    bool has_HorseID = false;
    int32_t SyncType{};  /* = 2 */
    bool has_SyncType = false;
    int32_t SyncData{};  /* = 3 */
    bool has_SyncData = false;
};

struct PB_ActorAttrChangeHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    float HP{};  /* = 2 */
    bool has_HP = false;
    uint64_t BeHurtTarget{};  /* = 3 */
    bool has_BeHurtTarget = false;
};

struct PB_ActorBuffChangeHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    std::vector<PB_ActorBuff> Buffs;  /* = 2 */
};

struct PB_ActorReviveHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    int32_t ReviveType{};  /* = 3 */
    bool has_ReviveType = false;
    float ReviveYaw{};  /* = 4 */
    bool has_ReviveYaw = false;
    float RevivePitch{};  /* = 5 */
    bool has_RevivePitch = false;
    PB_Vector3 RevivePosition{};  /* = 6 */
    bool has_RevivePosition = false;
};

struct PB_PlayerAttrChangeHC {
    float HP{};  /* = 1 */
    bool has_HP = false;
    float Oxygen{};  /* = 2 */
    bool has_Oxygen = false;
    int32_t Exp{};  /* = 3 */
    bool has_Exp = false;
    int32_t FoodLevel{};  /* = 4 */
    bool has_FoodLevel = false;
};

struct PB_MobBodyChangeHC {
    uint64_t ObjId{};  /* = 1 */
    bool has_ObjId = false;
    int32_t BodyColor{};  /* = 2 */
    bool has_BodyColor = false;
    int32_t Sheared{};  /* = 3 */
    bool has_Sheared = false;
};

struct PB_JruisdicTionHC {
    int32_t Ret{};  /* = 1 */
    bool has_Ret = false;
};

struct PB_ChatHC {
    int32_t ChatType{};  /* = 1 */
    bool has_ChatType = false;
    int32_t Uin{};  /* = 2 */
    bool has_Uin = false;
    std::string Speaker{};  /* = 3 */
    bool has_Speaker = false;
    std::string Content{};  /* = 4 */
    bool has_Content = false;
    int32_t Language{};  /* = 5 */
    bool has_Language = false;
};

struct PB_WGlobalUpdateHC {
    int32_t WorldTime{};  /* = 1 */
    bool has_WorldTime = false;
    int32_t ViewRange{};  /* = 2 */
    bool has_ViewRange = false;
    int32_t MapID{};  /* = 3 */
    bool has_MapID = false;
    int32_t Raining{};  /* = 4 */
    bool has_Raining = false;
    int32_t DayNightTime{};  /* = 5 */
    bool has_DayNightTime = false;
    int32_t Darking{};  /* = 6 */
    bool has_Darking = false;
    int32_t CurWeather{};  /* = 7 */
    bool has_CurWeather = false;
};

struct PB_PlayersUpdateInfoHC {
    std::vector<int32_t> TeamScores;  /* = 1 */
    std::vector<int32_t> TeamFlags;  /* = 2 */
    std::vector<PB_PlayerBriefInfo> Players;  /* = 3 */
};

struct PB_GameTipsHC {
    int32_t TipsType{};  /* = 1 */
    bool has_TipsType = false;
    int32_t Id{};  /* = 2 */
    bool has_Id = false;
    int32_t Num{};  /* = 3 */
    bool has_Num = false;
    std::string OtherName{};  /* = 4 */
    bool has_OtherName = false;
};

struct PB_PlayEffectHC {
    int32_t EffectType{};  /* = 1 */
    bool has_EffectType = false;
    PB_EffectParticle Particle{};  /* = 2 */
    bool has_Particle = false;
    PB_EffectPickItem PickItem{};  /* = 3 */
    bool has_PickItem = false;
    PB_EffectSound Sound{};  /* = 4 */
    bool has_Sound = false;
    PB_EffectActorBody ActorBody{};  /* = 5 */
    bool has_ActorBody = false;
    PB_EffectDestroyBlock DestroyBlock{};  /* = 6 */
    bool has_DestroyBlock = false;
    PB_EffectPlayMusicGrid PlayMusicGrid{};  /* = 7 */
    bool has_PlayMusicGrid = false;
    PB_EffectStopMusicGrid StopMusicGrid{};  /* = 8 */
    bool has_StopMusicGrid = false;
    PB_EffectStringActorBody StringActorBody{};  /* = 9 */
    bool has_StringActorBody = false;
    PB_EffectCrackBlock CrackBlock{};  /* = 10 */
    bool has_CrackBlock = false;
    float effectScale{};  /* = 11 */
    bool has_effectScale = false;
    int32_t effectClass{};  /* = 12 */
    bool has_effectClass = false;
    PB_EffectTriggerSound TriggerSound{};  /* = 13 */
    bool has_TriggerSound = false;
    PB_EffectVehicle EffectVehicle{};  /* = 14 */
    bool has_EffectVehicle = false;
};

struct PB_EffectScaleHC {
    int32_t EffectType{};  /* = 1 */
    bool has_EffectType = false;
    float effectScale{};  /* = 2 */
    bool has_effectScale = false;
    uint64_t objID{};  /* = 3 */
    bool has_objID = false;
    std::string effectName{};  /* = 4 */
    bool has_effectName = false;
    PB_Vector3 Pos{};  /* = 5 */
    bool has_Pos = false;
};

struct PB_PlayerMountActorHC {
    int32_t PlayerUIN{};  /* = 1 */
    bool has_PlayerUIN = false;
    int32_t RidePosIndex{};  /* = 2 */
    bool has_RidePosIndex = false;
    uint64_t ActorID{};  /* = 3 */
    bool has_ActorID = false;
    int32_t force{};  /* = 4 */
    bool has_force = false;
    int32_t InteractBlockId{};  /* = 5 */
    bool has_InteractBlockId = false;
};

struct PB_PlayerSleepHC {
    int32_t Flags{};  /* = 1 */
    bool has_Flags = false;
    PB_Vector3 Pos{};  /* = 2 */
    bool has_Pos = false;
};

struct PB_OpenWindowHC {
    int32_t id{};  /* = 1 */
    bool has_id = false;
};

struct PB_LastPingHC {
    std::vector<int32_t> MPLastPingOne;  /* = 1 */
};

struct PB_CGameStageHC {
    int32_t Stage{};  /* = 1 */
    bool has_Stage = false;
    int32_t GameTime{};  /* = 2 */
    bool has_GameTime = false;
};

struct PB_PlayerPermitHC {
    uint32_t RoomFlags{};  /* = 1 */
    bool has_RoomFlags = false;
    uint32_t PlayerFlags{};  /* = 2 */
    bool has_PlayerFlags = false;
    uint32_t SpamPreventionMinutes{};  /* = 3 */
    bool has_SpamPreventionMinutes = false;
    std::vector<int32_t> BanItems;  /* = 4 */
    std::vector<uint32_t> MutePlayerlist;  /* = 5 */
    std::vector<uint32_t> PlayerSpeakingWhitelist;  /* = 6 */
};

struct PB_YMVoiceHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t YMMemberID{};  /* = 2 */
    bool has_YMMemberID = false;
    int32_t SpeakerSwitch{};  /* = 3 */
    bool has_SpeakerSwitch = false;
    int32_t MicSwitch{};  /* = 4 */
    bool has_MicSwitch = false;
    int32_t YMMemberRole{};  /* = 5 */
    bool has_YMMemberRole = false;
};

struct PB_SkillCDHC {
    int32_t ItemID{};  /* = 1 */
    bool has_ItemID = false;
    float CD{};  /* = 2 */
    bool has_CD = false;
};

struct PB_Horse_SkillCDHC {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    int32_t Index{};  /* = 2 */
    bool has_Index = false;
    float CD{};  /* = 3 */
    bool has_CD = false;
};

struct PB_ActorMountActorHC {
    uint64_t ActorIDOwner{};  /* = 1 */
    bool has_ActorIDOwner = false;
    uint64_t ActorID{};  /* = 2 */
    bool has_ActorID = false;
    int32_t RidePosIndex{};  /* = 3 */
    bool has_RidePosIndex = false;
};

struct PB_ActorReverseHC {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    int32_t Reverse{};  /* = 2 */
    bool has_Reverse = false;
};

struct PB_ActorBindHC {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    uint64_t ActorIDBind{};  /* = 2 */
    bool has_ActorIDBind = false;
    PB_Vector3 OffetBind{};  /* = 3 */
    bool has_OffetBind = false;
};

struct PB_PlayWeaponEffectHC {
    std::string EffectName{};  /* = 1 */
    bool has_EffectName = false;
    int32_t EffectID{};  /* = 2 */
    bool has_EffectID = false;
    int32_t EffectStatus{};  /* = 3 */
    bool has_EffectStatus = false;
    int32_t EffectScale{};  /* = 4 */
    bool has_EffectScale = false;
    uint64_t ObjId{};  /* = 5 */
    bool has_ObjId = false;
};

struct PB_ScriptVarHC {
    std::vector<float> ScriptVars;  /* = 1 */
};

struct PB_GVChangeRoleHC {
    int32_t GVMemberRole{};  /* = 1 */
    bool has_GVMemberRole = false;
};

struct PB_YMChangeRoleHC {
    int32_t YMMemberRole{};  /* = 1 */
    bool has_YMMemberRole = false;
};

struct PB_SpecialItemUseHC {
    int32_t ItemId{};  /* = 1 */
    bool has_ItemId = false;
    int32_t ItemNum{};  /* = 2 */
    bool has_ItemNum = false;
};

struct PB_LeaveRoomInfoHC {
    int32_t Cause{};  /* = 1 */
    bool has_Cause = false;
    int32_t KickerType{};  /* = 2 */
    bool has_KickerType = false;
};

struct PB_InviteJoinRoomHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    std::string RoomState{};  /* = 2 */
    bool has_RoomState = false;
    std::string PassWorld{};  /* = 3 */
    bool has_PassWorld = false;
};

struct PB_SetSpectatorModeHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t SpectatorMode{};  /* = 2 */
    bool has_SpectatorMode = false;
};

struct PB_SetSpectatorTypeHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t SpectatorType{};  /* = 2 */
    bool has_SpectatorType = false;
};

struct PB_SetSpectatorPlayerHC {
    int32_t SpectatorUin{};  /* = 1 */
    bool has_SpectatorUin = false;
    int32_t ToSpectatorUin{};  /* = 2 */
    bool has_ToSpectatorUin = false;
};

struct PB_SetPlayerModelAniHC {
    int32_t SpectatorUin{};  /* = 1 */
    bool has_SpectatorUin = false;
    int32_t ToSpectatorUin{};  /* = 2 */
    bool has_ToSpectatorUin = false;
    int32_t ModelAnimalType{};  /* = 3 */
    bool has_ModelAnimalType = false;
    int32_t ModelAnimalExt{};  /* = 4 */
    bool has_ModelAnimalExt = false;
};

struct PB_SendMyViewmodeToSpectatorHC {
    int32_t SpectatorUin{};  /* = 1 */
    bool has_SpectatorUin = false;
    int32_t ToSpectatorUin{};  /* = 2 */
    bool has_ToSpectatorUin = false;
    int32_t MyViewmode{};  /* = 3 */
    bool has_MyViewmode = false;
};

struct PB_SetBobbingToSpectatorHC {
    int32_t SpectatorUin{};  /* = 1 */
    bool has_SpectatorUin = false;
    int32_t ToSpectatorUin{};  /* = 2 */
    bool has_ToSpectatorUin = false;
    int32_t Bobbing{};  /* = 3 */
    bool has_Bobbing = false;
};

struct PB_BallOperateHC {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    uint64_t ActorID{};  /* = 2 */
    bool has_ActorID = false;
    int32_t ExtendData{};  /* = 3 */
    bool has_ExtendData = false;
    int32_t Uin{};  /* = 4 */
    bool has_Uin = false;
};

struct PB_BasketBallOperateHC {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    uint64_t ActorID{};  /* = 2 */
    bool has_ActorID = false;
    bool IsSelectedTarget{};  /* = 3 */
    bool has_IsSelectedTarget = false;
    int32_t FallResult{};  /* = 4 */
    bool has_FallResult = false;
    int32_t ExtendData{};  /* = 5 */
    bool has_ExtendData = false;
    int32_t Uin{};  /* = 6 */
    bool has_Uin = false;
};

struct PB_ResetRoundHC {
};

struct PB_RocketAttribChangeHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t State{};  /* = 2 */
    bool has_State = false;
    int32_t Fuel{};  /* = 3 */
    bool has_Fuel = false;
};

struct PB_ActorBodyTextureHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    std::string TexName{};  /* = 2 */
    bool has_TexName = false;
    std::string MeshName{};  /* = 3 */
    bool has_MeshName = false;
};

struct PB_AttractAttribChangeHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t State{};  /* = 2 */
    bool has_State = false;
    int32_t blockID{};  /* = 3 */
    bool has_blockID = false;
};

struct PB_WorldTimesHC {
    std::vector<int32_t> Times;  /* = 1 */
};

struct PB_StatisticHC {
    int32_t EventID{};  /* = 1 */
    bool has_EventID = false;
    int32_t WorldType{};  /* = 2 */
    bool has_WorldType = false;
    std::string FristName{};  /* = 3 */
    bool has_FristName = false;
    std::string Param1{};  /* = 4 */
    bool has_Param1 = false;
    std::string Param2{};  /* = 5 */
    bool has_Param2 = false;
    std::string Param3{};  /* = 6 */
    bool has_Param3 = false;
    std::string Param4{};  /* = 7 */
    bool has_Param4 = false;
    std::string Param5{};  /* = 8 */
    bool has_Param5 = false;
    std::string Param6{};  /* = 9 */
    bool has_Param6 = false;
    std::string Param7{};  /* = 10 */
    bool has_Param7 = false;
};

struct PB_TotemPointHC {
    int32_t Op{};  /* = 1 */
    bool has_Op = false;
    PB_Vector3 Point{};  /* = 2 */
    bool has_Point = false;
    int32_t MapID{};  /* = 3 */
    bool has_MapID = false;
};

struct PB_HorseFlyStateHC {
    float m_fEnergy{};  /* = 1 */
    bool has_m_fEnergy = false;
    int32_t m_bTired{};  /* = 2 */
    bool has_m_bTired = false;
};

struct PB_OpenDialogueHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    std::vector<PB_IntertactData> InteractData;  /* = 2 */
    int32_t ItemID{};  /* = 3 */
    bool has_ItemID = false;
};

struct PB_CloseDialogueHC {
};

struct PB_UpdateTaskHC {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    int32_t ID{};  /* = 2 */
    bool has_ID = false;
    int32_t Num{};  /* = 3 */
    bool has_Num = false;
};

struct PB_SyncTaskEnterWorldHC {
    std::vector<PB_TaskInfoData> TaskInfoData;  /* = 1 */
};

struct PB_CompleteTaskHC {
    int32_t TaskID{};  /* = 1 */
    bool has_TaskID = false;
};

struct PB_PlayerAddAvartarHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t avatarmodel{};  /* = 2 */
    bool has_avatarmodel = false;
    int32_t index{};  /* = 3 */
    bool has_index = false;
};

struct PB_PlayerChangeModelHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t playerindex{};  /* = 2 */
    bool has_playerindex = false;
    std::string customskins{};  /* = 3 */
    bool has_customskins = false;
    int32_t Reason{};  /* = 4 */
    bool has_Reason = false;
};

struct PB_PlayerAvartarColorHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    float r{};  /* = 2 */
    bool has_r = false;
    float g{};  /* = 3 */
    bool has_g = false;
    float b{};  /* = 4 */
    bool has_b = false;
    float partID{};  /* = 5 */
    bool has_partID = false;
    float modelID{};  /* = 6 */
    bool has_modelID = false;
    float block{};  /* = 7 */
    bool has_block = false;
};

struct PB_PlayActHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t ActID{};  /* = 2 */
    bool has_ActID = false;
    int32_t ActIDTrigger{};  /* = 3 */
    bool has_ActIDTrigger = false;
};

struct PB_CreateBlueprintHC {
    PB_Vector3 Point{};  /* = 1 */
    bool has_Point = false;
    std::string sheetname{};  /* = 2 */
    bool has_sheetname = false;
    std::string authorname{};  /* = 3 */
    bool has_authorname = false;
};

struct PB_MeasureDistanceHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    PB_Vector3 blockpos{};  /* = 3 */
    bool has_blockpos = false;
    std::vector<PB_Vector3> findpos;  /* = 4 */
};

struct PB_BluePrintPreBlockHC {
    PB_Vector3 BlockPos{};  /* = 1 */
    bool has_BlockPos = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    std::vector<PB_PreBlockData> PreBlocks;  /* = 3 */
};

struct PB_GravityOperateHC {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    uint64_t ActorID{};  /* = 2 */
    bool has_ActorID = false;
    int32_t ExtendData{};  /* = 3 */
    bool has_ExtendData = false;
    int32_t Uin{};  /* = 4 */
    bool has_Uin = false;
};

struct PB_PlayerBodyColorHC {
    uint32_t DestColor{};  /* = 1 */
    bool has_DestColor = false;
    uint32_t CurColor{};  /* = 2 */
    bool has_CurColor = false;
    int32_t Uin{};  /* = 3 */
    bool has_Uin = false;
};

struct PB_CustomModelHC {
    int32_t UnzipLen{};  /* = 1 */
    bool has_UnzipLen = false;
    int32_t BlobLen{};  /* = 2 */
    bool has_BlobLen = false;
    std::string BlobDetail{};  /* = 3 */
    bool has_BlobDetail = false;
    std::string FileName{};  /* = 4 */
    bool has_FileName = false;
    int32_t ItemID{};  /* = 5 */
    bool has_ItemID = false;
    std::string ModelName{};  /* = 6 */
    bool has_ModelName = false;
    std::string ModelDesc{};  /* = 7 */
    bool has_ModelDesc = false;
    std::string ClassName{};  /* = 8 */
    bool has_ClassName = false;
    int32_t Type{};  /* = 9 */
    bool has_Type = false;
    PB_Vector3 Box{};  /* = 10 */
    bool has_Box = false;
    int32_t folderIndex{};  /* = 11 */
    bool has_folderIndex = false;
    bool IsDownload{};  /* = 12 */
    bool has_IsDownload = false;
    int32_t AuthUin{};  /* = 13 */
    bool has_AuthUin = false;
};

struct PB_CustomModelPrepareHC {
    std::string FileName{};  /* = 1 */
    bool has_FileName = false;
    int32_t Index{};  /* = 2 */
    bool has_Index = false;
};

struct PB_CustomItemIDsHC {
    std::vector<int32_t> CustomItemIDs;  /* = 1 */
    std::vector<std::string> CustomModelFileNames;  /* = 2 */
    std::vector<std::string> CustomModelClassNames;  /* = 3 */
    std::vector<int32_t> CustomTypes;  /* = 4 */
    std::vector<int32_t> InvolvedIds;  /* = 5 */
    std::vector<int32_t> CustomModelFolderIndexs;  /* = 6 */
};

struct PB_PlayerSpawnPointHC {
    int32_t x{};  /* = 1 */
    bool has_x = false;
    int32_t y{};  /* = 2 */
    bool has_y = false;
    int32_t z{};  /* = 3 */
    bool has_z = false;
    int32_t Uin{};  /* = 4 */
    bool has_Uin = false;
};

struct PB_CustomModelClassHC {
    std::vector<PB_CustomModelClassData> Data;  /* = 1 */
};

struct PB_TransferOneRecordHC {
    int32_t DesID{};  /* = 1 */
    bool has_DesID = false;
    int32_t TeamColor{};  /* = 2 */
    bool has_TeamColor = false;
    int32_t PassItemID{};  /* = 3 */
    bool has_PassItemID = false;
    int32_t PassItemNum{};  /* = 4 */
    bool has_PassItemNum = false;
    int32_t ForbidItemID{};  /* = 5 */
    bool has_ForbidItemID = false;
    bool IsExpendable{};  /* = 6 */
    bool has_IsExpendable = false;
};

struct PB_TransferRecordHC {
    int32_t SrcID{};  /* = 1 */
    bool has_SrcID = false;
    bool IsEdit{};  /* = 2 */
    bool has_IsEdit = false;
    PB_TransferOneRecordHC OneRecord{};  /* = 3 */
    bool has_OneRecord = false;
};

struct PB_TransferNameTipHC {
    int32_t SrcID{};  /* = 1 */
    bool has_SrcID = false;
    std::string TransferName{};  /* = 2 */
    bool has_TransferName = false;
    std::string TransferTip{};  /* = 3 */
    bool has_TransferTip = false;
    bool ShowName{};  /* = 4 */
    bool has_ShowName = false;
    int32_t Status{};  /* = 5 */
    bool has_Status = false;
};

struct PB_TransferAddDelHC {
    bool AddDel{};  /* = 1 */
    bool has_AddDel = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    PB_Vector3 postion{};  /* = 3 */
    bool has_postion = false;
    int32_t Status{};  /* = 4 */
    bool has_Status = false;
    int32_t TransferID{};  /* = 5 */
    bool has_TransferID = false;
    uint64_t vehicleObj{};  /* = 6 */
    bool has_vehicleObj = false;
    PB_Vector3 postion_v{};  /* = 7 */
    bool has_postion_v = false;
};

struct PB_TransferDataHC {
    int32_t ID{};  /* = 1 */
    bool has_ID = false;
    std::string TransferName{};  /* = 2 */
    bool has_TransferName = false;
    std::string TransferTip{};  /* = 3 */
    bool has_TransferTip = false;
    bool ShowName{};  /* = 4 */
    bool has_ShowName = false;
    int32_t MapID{};  /* = 5 */
    bool has_MapID = false;
    PB_Vector3 postion{};  /* = 6 */
    bool has_postion = false;
    int32_t Status{};  /* = 7 */
    bool has_Status = false;
    std::vector<PB_TransferOneRecordHC> OneRecord;  /* = 8 */
    uint64_t vehicleObj{};  /* = 9 */
    bool has_vehicleObj = false;
    PB_Vector3 postion_v{};  /* = 10 */
    bool has_postion_v = false;
};

struct PB_TransferTargetHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t DesID{};  /* = 2 */
    bool has_DesID = false;
    int32_t SrcID{};  /* = 3 */
    bool has_SrcID = false;
};

struct PB_OpenUIHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t Type{};  /* = 2 */
    bool has_Type = false;
    int32_t ID{};  /* = 3 */
    bool has_ID = false;
};

struct PB_RespNpcShopInfoHC {
    PB_NpcShopData NpcShopInfo{};  /* = 1 */
    bool has_NpcShopInfo = false;
};

struct PB_NotifyNpcShopBuySkuHC {
    int32_t Ret{};  /* = 1 */
    bool has_Ret = false;
    int32_t ShopID{};  /* = 2 */
    bool has_ShopID = false;
    int32_t SkuID{};  /* = 3 */
    bool has_SkuID = false;
    int32_t LeftNum{};  /* = 4 */
    bool has_LeftNum = false;
    int32_t EndTime{};  /* = 5 */
    bool has_EndTime = false;
    int32_t Uin{};  /* = 6 */
    bool has_Uin = false;
    int32_t BuyCount{};  /* = 7 */
    bool has_BuyCount = false;
};

struct PB_VehiclePosDesc {
    PB_Vector3 Position{};  /* = 1 */
    bool has_Position = false;
    PB_Quaternion RotateQuat{};  /* = 2 */
    bool has_RotateQuat = false;
};

struct PB_VehicleSTrustersPowerLevel {
    PB_Vector3 Position{};  /* = 1 */
    bool has_Position = false;
    uint32_t PowerLevel{};  /* = 2 */
    bool has_PowerLevel = false;
    int64_t CurPower{};  /* = 3 */
    bool has_CurPower = false;
};

struct PB_VehicleMoveHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    std::vector<PB_VehiclePosDesc> ChassisPos;  /* = 2 */
    std::vector<PB_VehiclePosDesc> WheelPos;  /* = 3 */
    std::vector<PB_VehicleSTrustersPowerLevel> STrustersPower;  /* = 4 */
};

struct PB_OpenEditActorModelHC {
    PB_Vector3 ContainerPos{};  /* = 1 */
    bool has_ContainerPos = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
};

struct PB_CloseEditActorModelHC {
    std::vector<PB_ActorOneBoneModelData> BoneModels;  /* = 1 */
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    PB_Vector3 ContainerPos{};  /* = 3 */
    bool has_ContainerPos = false;
    int32_t ModelType{};  /* = 4 */
    bool has_ModelType = false;
    std::string ModelMark{};  /* = 5 */
    bool has_ModelMark = false;
    std::string ModelName{};  /* = 6 */
    bool has_ModelName = false;
    bool SkinDisplay{};  /* = 7 */
    bool has_SkinDisplay = false;
};

struct PB_OneCustomActorModelDataHC {
    std::vector<PB_ActorOneBoneModelData> BoneModels;  /* = 1 */
    std::string ModelMark{};  /* = 2 */
    bool has_ModelMark = false;
    int32_t Type{};  /* = 3 */
    bool has_Type = false;
    std::string ModelName{};  /* = 4 */
    bool has_ModelName = false;
    bool SkinDisplay{};  /* = 5 */
    bool has_SkinDisplay = false;
    int32_t AuthUin{};  /* = 6 */
    bool has_AuthUin = false;
};

struct PB_CustomActorModelDataHC {
    std::vector<PB_OneCustomActorModelDataHC> ModelDatas;  /* = 1 */
};

struct PB_VehiclePreBlockHC {
    PB_Vector3 BlockPos{};  /* = 1 */
    bool has_BlockPos = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    std::string AttrInfo{};  /* = 3 */
    bool has_AttrInfo = false;
    std::vector<PB_PreBlockData> PreBlocks;  /* = 4 */
};

struct PB_VehicleItemIdHC {
    std::vector<int32_t> ItemID;  /* = 1 */
};

struct PB_VehicleAttribChangeHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t Fuel{};  /* = 2 */
    bool has_Fuel = false;
    int32_t PartIndex{};  /* = 3 */
    bool has_PartIndex = false;
    int32_t ActualSpeed{};  /* = 4 */
    bool has_ActualSpeed = false;
    int32_t ShowSpeed{};  /* = 5 */
    bool has_ShowSpeed = false;
    float EngineRotationSpeed{};  /* = 6 */
    bool has_EngineRotationSpeed = false;
    int32_t EngineState{};  /* = 7 */
    bool has_EngineState = false;
    std::vector<PB_VehicleSTrustersPowerLevel> STrustersPower;  /* = 8 */
};

struct PB_WorkshopItemInfoHC {
    int32_t ItemID{};  /* = 1 */
    bool has_ItemID = false;
    std::string ItemName{};  /* = 2 */
    bool has_ItemName = false;
    std::string ItemDesc{};  /* = 3 */
    bool has_ItemDesc = false;
    bool IsStart{};  /* = 4 */
    bool has_IsStart = false;
};

struct PB_PlayerCameraRotateHC {
    float Pitch{};  /* = 1 */
    bool has_Pitch = false;
    float Yaw{};  /* = 2 */
    bool has_Yaw = false;
};

struct PB_PlayerChangeViewModeHC {
    int32_t ViewMode{};  /* = 1 */
    bool has_ViewMode = false;
    bool Lock{};  /* = 2 */
    bool has_Lock = false;
    bool ClientMode{};  /* = 3 */
    bool has_ClientMode = false;
};

struct PB_PlayerCanMoveHC {
    bool CanMove{};  /* = 1 */
    bool has_CanMove = false;
};

struct PB_PlayerCanControlHC {
    bool CanControl{};  /* = 1 */
    bool has_CanControl = false;
};

struct PB_PlayerSetAttrHC {
    int32_t AttrType{};  /* = 1 */
    bool has_AttrType = false;
    float Val{};  /* = 2 */
    bool has_Val = false;
};

struct PB_TriggerTimerDataHC {
    int32_t TimerID{};  /* = 1 */
    bool has_TimerID = false;
    int32_t Time{};  /* = 2 */
    bool has_Time = false;
    int32_t Type{};  /* = 3 */
    bool has_Type = false;
    std::string Title{};  /* = 4 */
    bool has_Title = false;
};

struct PB_PlayerFreezingHC {
    int32_t freezingflag{};  /* = 1 */
    bool has_freezingflag = false;
};

struct PB_WorkshopBuildHC {
    bool Isbuild{};  /* = 1 */
    bool has_Isbuild = false;
    int32_t Mapid{};  /* = 2 */
    bool has_Mapid = false;
    PB_Vector3 ContainerPos{};  /* = 3 */
    bool has_ContainerPos = false;
};

struct PB_GameRuleHC {
    int32_t ruleid{};  /* = 1 */
    bool has_ruleid = false;
    int32_t optionid{};  /* = 2 */
    bool has_optionid = false;
    float value{};  /* = 3 */
    bool has_value = false;
};

struct PB_PlayerScaleHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    float Scale{};  /* = 2 */
    bool has_Scale = false;
    uint64_t ObjID{};  /* = 3 */
    bool has_ObjID = false;
};

struct PB_PlayerNavigateHC {
    PB_Vector3 TargetPos{};  /* = 1 */
    bool has_TargetPos = false;
    float Speed{};  /* = 2 */
    bool has_Speed = false;
    bool CanControl{};  /* = 3 */
    bool has_CanControl = false;
    bool showTip{};  /* = 4 */
    bool has_showTip = false;
};

struct PB_PlayerCommonSetHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    float Value{};  /* = 2 */
    bool has_Value = false;
};

struct PB_OpenEditFullyCustomModelHC {
    PB_Vector3 ContainerPos{};  /* = 1 */
    bool has_ContainerPos = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    bool Edited{};  /* = 3 */
    bool has_Edited = false;
    std::string url{};  /* = 4 */
    bool has_url = false;
    int32_t version{};  /* = 5 */
    bool has_version = false;
    int32_t result{};  /* = 6 */
    bool has_result = false;
};

struct PB_CloseFullyCustomModelUIHC {
    int32_t Result{};  /* = 1 */
    bool has_Result = false;
    int32_t MapID{};  /* = 2 */
    bool has_MapID = false;
    PB_Vector3 ContainerPos{};  /* = 3 */
    bool has_ContainerPos = false;
    std::string Skey{};  /* = 4 */
    bool has_Skey = false;
};

struct PB_RespDownLoadResUrlHC {
    int32_t Type{};  /* = 1 */
    bool has_Type = false;
    std::string ExternData{};  /* = 2 */
    bool has_ExternData = false;
    std::string DownloadUrl{};  /* = 3 */
    bool has_DownloadUrl = false;
};

struct PB_PreOpenEditFCMUIHC {
    int32_t State{};  /* = 1 */
    bool has_State = false;
};

struct PB_VehicleAssembleLineHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    int32_t from{};  /* = 2 */
    bool has_from = false;
    int32_t to{};  /* = 3 */
    bool has_to = false;
};

struct PB_VehicleAssembleLineOperateHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
    bool State{};  /* = 2 */
    bool has_State = false;
    PB_Vector3 BlockPos{};  /* = 3 */
    bool has_BlockPos = false;
};

struct PB_VehicleBindActorHC {
    uint64_t vehicleObjID{};  /* = 1 */
    bool has_vehicleObjID = false;
    uint64_t bindObjID{};  /* = 2 */
    bool has_bindObjID = false;
    PB_Vector3 BlockPos{};  /* = 3 */
    bool has_BlockPos = false;
};

struct PB_TriggerMusicHC {
    int32_t objid{};  /* = 1 */
    bool has_objid = false;
    std::string Name{};  /* = 2 */
    bool has_Name = false;
    float Volume{};  /* = 3 */
    bool has_Volume = false;
    float Pitch{};  /* = 4 */
    bool has_Pitch = false;
    bool IsLoop{};  /* = 5 */
    bool has_IsLoop = false;
    int32_t PlayState{};  /* = 6 */
    bool has_PlayState = false;
};

struct PB_CSPlayerPermitHC {
    int32_t TargetUin{};  /* = 1 */
    bool has_TargetUin = false;
    uint32_t Flags{};  /* = 2 */
    bool has_Flags = false;
    std::vector<int32_t> BanItems;  /* = 3 */
    int32_t DataType{};  /* = 4 */
    bool has_DataType = false;
    std::vector<PB_CSPermitData> Permits;  /* = 5 */
};

struct PB_CSAuthorityHC {
    int32_t DataType{};  /* = 1 */
    bool has_DataType = false;
    std::vector<PB_CSAuthorityData> Authorities;  /* = 2 */
};

struct PB_SSTaskHC {
    int32_t TargetUin{};  /* = 1 */
    bool has_TargetUin = false;
    int32_t TaskId{};  /* = 2 */
    bool has_TaskId = false;
    std::string ParamJson{};  /* = 3 */
    bool has_ParamJson = false;
};

struct PB_CloudServerChangeHC {
    int32_t type{};  /* = 1 */
    bool has_type = false;
};

struct PB_TriggerOpenStoreHC {
    uint64_t ObjID{};  /* = 1 */
    bool has_ObjID = false;
};

struct PB_UsePackingFcmItemHC {
    int32_t result{};  /* = 1 */
    bool has_result = false;
};

struct PB_CreatePackingCMHC {
    int32_t result{};  /* = 1 */
    bool has_result = false;
};

struct PB_OnePackingCMDataHC {
    PB_Vector3 PackingPos{};  /* = 1 */
    bool has_PackingPos = false;
    PB_Quaternion Quat{};  /* = 2 */
    bool has_Quat = false;
    std::string Name{};  /* = 3 */
    bool has_Name = false;
    std::string Model{};  /* = 4 */
    bool has_Model = false;
    int32_t Dir{};  /* = 5 */
    bool has_Dir = false;
};

struct PB_OnePackingFCMDataHC {
    std::string Name{};  /* = 1 */
    bool has_Name = false;
    std::string Desc{};  /* = 2 */
    bool has_Desc = false;
    std::string SKey{};  /* = 3 */
    bool has_SKey = false;
    std::vector<PB_OnePackingCMDataHC> PackingCMs;  /* = 4 */
    int32_t Dir{};  /* = 5 */
    bool has_Dir = false;
    PB_Vector3 MinPos{};  /* = 6 */
    bool has_MinPos = false;
    PB_Vector3 MaxPos{};  /* = 7 */
    bool has_MaxPos = false;
    int32_t AuthUin{};  /* = 8 */
    bool has_AuthUin = false;
};

struct PB_PackingFCMDataHC {
    std::vector<PB_OnePackingFCMDataHC> PackingFCMs;  /* = 1 */
};

struct PB_CloudRoomStatusTimeHC {
    int32_t Status{};  /* = 1 */
    bool has_Status = false;
    int32_t Time{};  /* = 2 */
    bool has_Time = false;
};

struct PB_SensorContainerDataHC {
    PB_Vector3 BlockPos{};  /* = 1 */
    bool has_BlockPos = false;
    int32_t SensorValue{};  /* = 2 */
    bool has_SensorValue = false;
    bool IsBreverse{};  /* = 3 */
    bool has_IsBreverse = false;
    uint64_t ObjID{};  /* = 4 */
    bool has_ObjID = false;
};

struct PB_DoorDataHC {
    PB_Vector3 BlockPos{};  /* = 1 */
    bool has_BlockPos = false;
};

struct PB_PlayerCarryActorHC {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    int32_t PlayerUIN{};  /* = 2 */
    bool has_PlayerUIN = false;
};

struct PB_VillagerBodyChangeHC {
    uint64_t ObjId{};  /* = 1 */
    bool has_ObjId = false;
    int32_t ChangeType{};  /* = 2 */
    bool has_ChangeType = false;
    int64_t ChangeValue{};  /* = 3 */
    bool has_ChangeValue = false;
    std::string OtherValue{};  /* = 4 */
    bool has_OtherValue = false;
};

struct PB_PlayerTameActorHC {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    int32_t PlayerUIN{};  /* = 2 */
    bool has_PlayerUIN = false;
};

struct PB_VillagerCloth {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    bool bshow{};  /* = 2 */
    bool has_bshow = false;
    std::string modlename{};  /* = 3 */
    bool has_modlename = false;
};

struct PB_ActorHeadDisplayIconHC {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    int32_t ItemID{};  /* = 2 */
    bool has_ItemID = false;
    int32_t Tick{};  /* = 3 */
    bool has_Tick = false;
};

struct PB_ActorPlayAnimByIdHC {
    uint64_t ActorID{};  /* = 1 */
    bool has_ActorID = false;
    int32_t AnimID{};  /* = 2 */
    bool has_AnimID = false;
};

struct PB_VillageTotemTipHC {
    int32_t villagerNum{};  /* = 1 */
    bool has_villagerNum = false;
    PB_Vector3 blockPos{};  /* = 2 */
    bool has_blockPos = false;
};

struct PB_VillageTotemActiveHC {
    int32_t uin{};  /* = 1 */
    bool has_uin = false;
    PB_Vector3 blockPos{};  /* = 2 */
    bool has_blockPos = false;
};

struct PB_SaveTombStoneHC {
    PB_Vector3 Point{};  /* = 1 */
    bool has_Point = false;
    std::string title{};  /* = 2 */
    bool has_title = false;
};

struct PB_PlayerLevelModeHC {
    int32_t SumExp{};  /* = 1 */
    bool has_SumExp = false;
    int32_t CurLevel{};  /* = 2 */
    bool has_CurLevel = false;
    int32_t CurExp{};  /* = 3 */
    bool has_CurExp = false;
};

struct PB_ActionAttrStateHC {
    uint32_t Attr{};  /* = 1 */
    bool has_Attr = false;
};

struct PB_Edu_RolesInfoHC {
    std::vector<PB_Edu_RoleInfo> rolesInfo;  /* = 1 */
};

struct PB_ImportModelHC {
    std::vector<PB_ImportModelData> models;  /* = 1 */
};

struct PB_LightningHC {
    PB_Vector3 targetpos{};  /* = 1 */
    bool has_targetpos = false;
};

struct PB_InteractMobPackHC {
    std::string uiName{};  /* = 1 */
    bool has_uiName = false;
    std::string param{};  /* = 2 */
    bool has_param = false;
    int64_t mobID{};  /* = 3 */
    bool has_mobID = false;
};

struct PB_UpdateMobBackpackHC {
    int64_t mobID{};  /* = 1 */
    bool has_mobID = false;
    std::vector<PB_ItemData> ItemInfo;  /* = 2 */
};

struct PB_PlayerTransformSkinHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t playerindex{};  /* = 2 */
    bool has_playerindex = false;
    std::string customskins{};  /* = 3 */
    bool has_customskins = false;
    int32_t Reason{};  /* = 4 */
    bool has_Reason = false;
    uint64_t MainPlayerId{};  /* = 5 */
    bool has_MainPlayerId = false;
};

struct PB_PlayerSaveArchHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t playTime{};  /* = 2 */
    bool has_playTime = false;
    bool showTip{};  /* = 3 */
    bool has_showTip = false;
    std::string userdata{};  /* = 4 */
    bool has_userdata = false;
};

struct PB_PrayTreeStageHC {
    int32_t stage{};  /* = 1 */
    bool has_stage = false;
};

struct PB_PrayTreeReqHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t stage{};  /* = 2 */
    bool has_stage = false;
    std::string treeId{};  /* = 3 */
    bool has_treeId = false;
};

struct PB_PrayTreeInfoHC {
    int32_t stage{};  /* = 1 */
    bool has_stage = false;
    std::string treeId{};  /* = 2 */
    bool has_treeId = false;
    int32_t hostuin{};  /* = 3 */
    bool has_hostuin = false;
};

struct PB_PrayTreeTimeUpdateHC {
    int32_t stage{};  /* = 1 */
    bool has_stage = false;
    std::string treeTime{};  /* = 2 */
    bool has_treeTime = false;
};

struct PB_HomeNpcOpenHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t npcType{};  /* = 2 */
    bool has_npcType = false;
};

struct PB_PrayErrorHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t errorType{};  /* = 2 */
    bool has_errorType = false;
};

struct PB_Open_HomeCloset_HC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    std::string skinIDs{};  /* = 2 */
    bool has_skinIDs = false;
    std::string skinPartIDs{};  /* = 3 */
    bool has_skinPartIDs = false;
};

struct PB_OpenDevGoodsBuyDialogHC {
    int32_t itemid{};  /* = 1 */
    bool has_itemid = false;
    std::string desc{};  /* = 2 */
    bool has_desc = false;
};

struct PB_PlayGraphicsHC {
    int32_t Operateid{};  /* = 1 */
    bool has_Operateid = false;
    std::vector<PB_GraphicsAttr> graphicsinfo;  /* = 2 */
};

struct PB_GodTempleCreateHC {
    bool onoff{};  /* = 1 */
    bool has_onoff = false;
    int32_t worldid{};  /* = 2 */
    bool has_worldid = false;
};

struct PB_SFActivity_HC {
    int32_t type{};  /* = 1 */
    bool has_type = false;
    int32_t taskId{};  /* = 2 */
    bool has_taskId = false;
    int32_t value{};  /* = 3 */
    bool has_value = false;
};

struct PB_ShapeAdditionAnimHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    bool status{};  /* = 2 */
    bool has_status = false;
};

struct PB_OneHomelandRanchAnimal {
    int32_t SeedID{};  /* = 1 */
    bool has_SeedID = false;
    int32_t GrowthTime{};  /* = 2 */
    bool has_GrowthTime = false;
    int32_t FeedTime{};  /* = 3 */
    bool has_FeedTime = false;
    int32_t HungerTime{};  /* = 4 */
    bool has_HungerTime = false;
    int32_t Stage{};  /* = 5 */
    bool has_Stage = false;
    bool HungerFlag{};  /* = 6 */
    bool has_HungerFlag = false;
    std::string Serverid{};  /* = 7 */
    bool has_Serverid = false;
};

struct PB_HomelandRanchInfoHC {
    bool full{};  /* = 1 */
    bool has_full = false;
    std::vector<PB_OneHomelandRanchAnimal> Animals;  /* = 2 */
};

struct PB_UseItemByHomelandHC {
    int32_t ItemID{};  /* = 1 */
    bool has_ItemID = false;
    int32_t ItemNum{};  /* = 2 */
    bool has_ItemNum = false;
};

struct PB_CustomBaseModelHC {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
    int32_t TeamId{};  /* = 2 */
    bool has_TeamId = false;
    std::string ModelData{};  /* = 3 */
    bool has_ModelData = false;
};

struct PB_ChangeActorModelHC {
    std::vector<uint64_t> ObjID;  /* = 1 */
    std::vector<std::string> modelID;  /* = 2 */
};

struct PB_NotifiyActorModelHC {
    std::vector<std::string> modelID;  /* = 1 */
    std::vector<std::string> modelData;  /* = 2 */
};

struct PB_RoomInfo {
    int32_t OwnerUin{};  /* = 1 */
    bool has_OwnerUin = false;
    int32_t PlayerCount{};  /* = 2 */
    bool has_PlayerCount = false;
    int32_t MaxPlayerCount{};  /* = 3 */
    bool has_MaxPlayerCount = false;
    int32_t GameType{};  /* = 4 */
    bool has_GameType = false;
    double GpsLatitude{};  /* = 5 */
    bool has_GpsLatitude = false;
    double GpsLongtitude{};  /* = 6 */
    bool has_GpsLongtitude = false;
    int64_t Timestamp{};  /* = 7 */
    bool has_Timestamp = false;
    std::string RoomTitle{};  /* = 8 */
    bool has_RoomTitle = false;
    std::string OwnerName{};  /* = 9 */
    bool has_OwnerName = false;
    std::string Password{};  /* = 10 */
    bool has_Password = false;
    std::string Description{};  /* = 11 */
    bool has_Description = false;
    std::string ExtraData{};  /* = 12 */
    bool has_ExtraData = false;
    int32_t LastPing{};  /* = 13 */
    bool has_LastPing = false;
    int32_t Province{};  /* = 14 */
    bool has_Province = false;
    int32_t Isp{};  /* = 15 */
    bool has_Isp = false;
    int32_t GameLabel{};  /* = 16 */
    bool has_GameLabel = false;
    int64_t OWID{};  /* = 17 */
    bool has_OWID = false;
    int32_t RegionIp{};  /* = 18 */
    bool has_RegionIp = false;
    int32_t ApiID{};  /* = 19 */
    bool has_ApiID = false;
    std::string RoomIP{};  /* = 20 */
    bool has_RoomIP = false;
    int32_t RoomPort{};  /* = 21 */
    bool has_RoomPort = false;
    int32_t GameEnv{};  /* = 22 */
    bool has_GameEnv = false;
    std::string MapType{};  /* = 23 */
    bool has_MapType = false;
    int32_t ConnectMode{};  /* = 24 */
    bool has_ConnectMode = false;
};

struct PB_LoginRoomServerReq {
    int32_t Uin{};  /* = 1 */
    bool has_Uin = false;
};

struct PB_SvrUrlInfo {
    std::string Url{};  /* = 1 */
    bool has_Url = false;
    int32_t Port{};  /* = 2 */
    bool has_Port = false;
};

struct PB_LoginRoomServerRes {
    int32_t ResultCode{};  /* = 1 */
    bool has_ResultCode = false;
    int32_t MaxVersion{};  /* = 2 */
    bool has_MaxVersion = false;
    int32_t MinVersion{};  /* = 3 */
    bool has_MinVersion = false;
    int32_t LatestVersion{};  /* = 4 */
    bool has_LatestVersion = false;
    int32_t Province{};  /* = 5 */
    bool has_Province = false;
    int32_t Isp{};  /* = 6 */
    bool has_Isp = false;
    int32_t UseProxy{};  /* = 7 */
    bool has_UseProxy = false;
};

struct PB_GetRoomsInfoRes {
    int32_t ResultCode{};  /* = 1 */
    bool has_ResultCode = false;
    std::vector<PB_RoomInfo> RoomInfoList;  /* = 2 */
};

struct PB_JoinRoomRes {
    int32_t Result{};  /* = 1 */
    bool has_Result = false;
};

}  /* namespace immirror::net::gen */
