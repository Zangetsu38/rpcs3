#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUCallback.h"

#include "sceNp.h"
#include "sceNp2.h"
#include "cellSysutil.h"


LOG_CHANNEL(sceNp2);

struct np2_context_t
{
	static const u32 id_base = 1;
	static const u32 id_step = 1;
	static const u32 id_count = 1023;

	SceNpMatching2RequestOptParam optParam;

	std::string name;
};

s32 sceNp2Init(u32 poolsize, vm::ptr<void> poolptr)
{
	sceNp2.warning("sceNp2Init(poolsize=0x%x, poolptr=*0x%x)", poolsize, poolptr);

	if (poolsize == 0)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}
	else if (poolsize < 128 * 1024)
	{
		return SCE_NP_ERROR_INSUFFICIENT_BUFFER;
	}

	if (!poolptr)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

s32 sceNpMatching2Init(u32 poolsize, s32 priority)
{
	sceNp2.todo("sceNpMatching2Init(poolsize=0x%x, priority=%d)", poolsize, priority);

	return CELL_OK;
}

s32 sceNpMatching2Init2(u32 poolsize, s32 priority, vm::ptr<SceNpMatching2UtilityInitParam> param)
{
	sceNp2.todo("sceNpMatching2Init2(poolsize=0x%x, priority=%d, param=*0x%x)", poolsize, priority, param);

	// TODO:
	// 1. Create an internal thread
	// 2. Create heap area to be used by the NP matching 2 utility
	// 3. Set maximum lengths for the event data queues in the system

	return CELL_OK;
}

s32 sceNp2Term()
{
	sceNp2.warning("sceNp2Term()");

	return CELL_OK;
}

s32 sceNpMatching2Term(ppu_thread& ppu)
{
	sceNp2.warning("sceNpMatching2Term()");

	return CELL_OK;
}

s32 sceNpMatching2Term2()
{
	sceNp2.warning("sceNpMatching2Term2()");

	return CELL_OK;
}

s32 sceNpMatching2DestroyContext(ppu_thread& ppu, u16 contextId)
{
	sceNp2.warning("sceNpMatching2DestroyContext(%d)", contextId);
	const auto ctxt = idm::get<np2_context_t>(contextId);
	if (!ctxt)
	{
		sceNp2.warning("sceNpMatching2DestroyContext(%d) oh noes", contextId);
	}
	idm::remove<np2_context_t>(contextId);

	return CELL_OK;
}

s32 sceNpMatching2LeaveLobby()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2RegisterLobbyMessageCallback()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetWorldInfoList(ppu_thread& ppu, u16 contextId, vm::cptr<SceNpMatching2GetWorldInfoListRequest> request, vm::cptr<SceNpMatching2RequestOptParam> optParam, u32 assignedReqId)
{
	sceNp2.error("sceNpMatching2GetWorldInfoList(contextId=0x%x, reqParam=*0x%x, optParam=*0x%x, assignedReqId=%d)", contextId, request, optParam, assignedReqId);

	const auto ctxt = idm::get<np2_context_t>(contextId);
	assignedReqId = 4000;
	ctxt->optParam.cbFunc(ppu, contextId, assignedReqId, SCE_NP_MATCHING2_REQUEST_EVENT_GetWorldInfoList, 0, 0, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetWorldInfoList, ctxt->optParam.cbFuncArg);



	return CELL_OK;
}

s32 sceNpMatching2RegisterLobbyEventCallback()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetLobbyMemberDataInternalList()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SearchRoom()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingGetConnectionStatus()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SetUserInfo()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetClanLobbyId()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetLobbyMemberDataInternal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2ContextStart()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2CreateServerContext()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetMemoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2LeaveRoom()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SetRoomDataExternal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingGetConnectionInfo()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SendRoomMessage()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2JoinLobby()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetRoomMemberDataExternalList()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2AbortRequest()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetServerInfo(ppu_thread& ppu, u16 contextId, vm::cptr<SceNpMatching2GetServerInfoRequest> reqParam, vm::cptr<SceNpMatching2RequestOptParam> optParam, u32 assignedReqId)
{
	/*sceNp2.error("sceNpMatching2GetServerInfo(contextId=0x%x, reqParam=*0x%x, optParam=*0x%x, assignedReqId=%d)", contextId, reqParam, optParam, assignedReqId);
	sceNp2.error("sceNpMatching2GetServerInfo serverID = %d", reqParam->serverId);
	assignedReqId = 1338;

	const auto ctxt = idm::get<np2_context_t>(contextId);
	ctxt->optParam.cbFunc(ppu, contextId, assignedReqId, SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo, 0, 0, SCE_NP_MATCHING2_EVENT_DATA_MAX_SIZE_GetServerInfo, ctxt->optParam.cbFuncArg);
*/

	//	sysutil_send_system_cmd(SCE_NP_MATCHING2_REQUEST_EVENT_GetServerInfo, 1);

	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetEventData(ppu_thread& ppu, vm::ptr<SceNpMatching2ContextId> contextId, vm::ptr<SceNpMatching2EventKey> eventKey, vm::ptr<SceNpMatching2GetServerInfoResponse> buf, u32 bufLen)
{
	sceNp2.error("sceNpMatching2GetEventData contextId=%d, eventKey=*0x%x, buf=*0x%x, bufLen=%d", contextId, eventKey, buf, bufLen);

	//buf is a multitype

	buf->server.serverId = 1337;
	buf->server.status = SCE_NP_MATCHING2_SERVER_STATUS_AVAILABLE;


	return CELL_OK;
}

s32 sceNpMatching2GetRoomSlotInfoLocal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SendLobbyChatMessage()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2AbortContextStart()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetRoomMemberIdListLocal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2JoinRoom()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetRoomMemberDataInternalLocal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetCbQueueInfo()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2KickoutRoomMember()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2ContextStartAsync()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SetSignalingOptParam()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2RegisterContextCallback()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SendRoomChatMessage()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SetRoomDataInternal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetRoomDataInternal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingGetPingInfo()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetServerIdListLocal(vm::ptr<SceNpMatching2ContextId> contextId, vm::ptr<SceNpMatching2ServerId> serverId, u32 maxServers)
{//Return num of servers
	UNIMPLEMENTED_FUNC(sceNp2);
	//sceNp2.error("sceNpMatching2GetServerIdListLocal(contextId=0x%x, serverId=*0x%x, maxServers=%d)", contextId, serverId, maxServers);

	//*serverId = 1337;
	//return 1;
	return CELL_OK;
}

s32 sceNpUtilBuildCdnUrl()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GrantRoomOwner()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

void deleteTerminateChar2(char* myStr, char _char) {

	char *del = &myStr[strlen(myStr)];

	while (del > myStr && *del != _char)
		del--;

	if (*del == _char)
		*del = '\0';

	return;
}

s32 sceNpMatching2CreateContext(vm::cptr<SceNpId> npId, vm::cptr<SceNpCommunicationId> commId, vm::cptr<SceNpCommunicationPassphrase> passphrase, vm::ptr<SceNpMatching2ContextId> contextId, u32 option)
{
	sceNp2.error("sceNpMatching2CreateContext(npId=%s, commId=%s, passphrase=%s, contextId=%d, option=%d)", npId->handle.data, commId->data, passphrase->data, contextId, option);
	sceNp2.error("todo sceNpMatching2CreateContext(npId=%s, commId=%d, passphrase=%s, contextId=%d, option=%d)", npId->handle.data, commId->num, passphrase->data, contextId, option);

	if (!contextId)
	{
		return SCE_NP_ERROR_INVALID_ARGUMENT;//might be wrong
	}

	std::string name;
	if (commId->term)
	{
		char trimchar[10] = { 0 };
		memcpy(trimchar, commId->data, sizeof(trimchar) - 1);
		deleteTerminateChar2(trimchar, commId->term);
		name = fmt::format("%s_%02d", trimchar, commId->num);
	}
	else
	{
		name = fmt::format("%s_%02d", commId->data, commId->num);
	}
	sceNp2.error("name: %s", name);

	const auto ctxt = idm::make_ptr<np2_context_t>();

	*contextId = idm::last_id();

	return CELL_OK;
}

s32 sceNpMatching2GetSignalingOptParamLocal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2RegisterSignalingCallback()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2ClearEventData()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetUserInfoList()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetRoomMemberDataInternal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SetRoomMemberDataInternal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2JoinProhibitiveRoom()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingSetCtxOpt()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2DeleteServerContext()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SetDefaultRequestOptParam(ppu_thread& ppu, u16 contextId, vm::cptr<SceNpMatching2RequestOptParam> optParam)
{
	sceNp2.error("sceNpMatching2SetDefaultRequestOptParam(contextId=0x%x, optParam=*0x%x)", contextId, optParam);
	sceNp2.error("sceNpMatching2SetDefaultRequestOptParam appReqId=%d, cbFuncArg=*0x%x, timeout=%d", optParam->appReqId, optParam->cbFuncArg, optParam->timeout);

	const auto ctxt = idm::get<np2_context_t>(contextId);
	ctxt->optParam = *optParam;


	return CELL_OK;
}

s32 sceNpMatching2RegisterRoomEventCallback()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetRoomPasswordLocal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetRoomDataExternalList()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2CreateJoinRoom()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingGetCtxOpt()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetLobbyInfoList()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2GetLobbyMemberIdListLocal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SendLobbyInvitation()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2ContextStop()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SetLobbyMemberDataInternal()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2RegisterRoomMessageCallback()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingCancelPeerNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingGetLocalNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingGetPeerNetInfo()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpMatching2SignalingGetPeerNetInfoResult()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpAuthOAuthInit()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpAuthOAuthTerm()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpAuthCreateOAuthRequest()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpAuthDeleteOAuthRequest()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpAuthAbortOAuthRequest()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpAuthGetAuthorizationCode()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

s32 sceNpAuthGetAuthorizationCode2()
{
	UNIMPLEMENTED_FUNC(sceNp2);
	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNp2)("sceNp2", []()
{
	REG_FUNC(sceNp2, sceNpMatching2DestroyContext);
	REG_FUNC(sceNp2, sceNpMatching2LeaveLobby);
	REG_FUNC(sceNp2, sceNpMatching2RegisterLobbyMessageCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetWorldInfoList);
	REG_FUNC(sceNp2, sceNpMatching2RegisterLobbyEventCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberDataInternalList);
	REG_FUNC(sceNp2, sceNpMatching2SearchRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetConnectionStatus);
	REG_FUNC(sceNp2, sceNpMatching2SetUserInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetClanLobbyId);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2ContextStart);
	REG_FUNC(sceNp2, sceNpMatching2CreateServerContext);
	REG_FUNC(sceNp2, sceNpMatching2GetMemoryInfo);
	REG_FUNC(sceNp2, sceNpMatching2LeaveRoom);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomDataExternal);
	REG_FUNC(sceNp2, sceNpMatching2Term2);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetConnectionInfo);
	REG_FUNC(sceNp2, sceNpMatching2SendRoomMessage);
	REG_FUNC(sceNp2, sceNpMatching2JoinLobby);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataExternalList);
	REG_FUNC(sceNp2, sceNpMatching2AbortRequest);
	REG_FUNC(sceNp2, sceNpMatching2Term);
	REG_FUNC(sceNp2, sceNpMatching2GetServerInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetEventData);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomSlotInfoLocal);
	REG_FUNC(sceNp2, sceNpMatching2SendLobbyChatMessage);
	REG_FUNC(sceNp2, sceNpMatching2Init);
	REG_FUNC(sceNp2, sceNp2Init);
	REG_FUNC(sceNp2, sceNpMatching2AbortContextStart);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberIdListLocal);
	REG_FUNC(sceNp2, sceNpMatching2JoinRoom);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataInternalLocal);
	REG_FUNC(sceNp2, sceNpMatching2GetCbQueueInfo);
	REG_FUNC(sceNp2, sceNpMatching2KickoutRoomMember);
	REG_FUNC(sceNp2, sceNpMatching2ContextStartAsync);
	REG_FUNC(sceNp2, sceNpMatching2SetSignalingOptParam);
	REG_FUNC(sceNp2, sceNpMatching2RegisterContextCallback);
	REG_FUNC(sceNp2, sceNpMatching2SendRoomChatMessage);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetPingInfo);
	REG_FUNC(sceNp2, sceNpMatching2GetServerIdListLocal);
	REG_FUNC(sceNp2, sceNpUtilBuildCdnUrl);
	REG_FUNC(sceNp2, sceNpMatching2GrantRoomOwner);
	REG_FUNC(sceNp2, sceNpMatching2CreateContext);
	REG_FUNC(sceNp2, sceNpMatching2GetSignalingOptParamLocal);
	REG_FUNC(sceNp2, sceNpMatching2RegisterSignalingCallback);
	REG_FUNC(sceNp2, sceNpMatching2ClearEventData);
	REG_FUNC(sceNp2, sceNp2Term);
	REG_FUNC(sceNp2, sceNpMatching2GetUserInfoList);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2SetRoomMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2JoinProhibitiveRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingSetCtxOpt);
	REG_FUNC(sceNp2, sceNpMatching2DeleteServerContext);
	REG_FUNC(sceNp2, sceNpMatching2SetDefaultRequestOptParam);
	REG_FUNC(sceNp2, sceNpMatching2RegisterRoomEventCallback);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomPasswordLocal);
	REG_FUNC(sceNp2, sceNpMatching2GetRoomDataExternalList);
	REG_FUNC(sceNp2, sceNpMatching2CreateJoinRoom);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetCtxOpt);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyInfoList);
	REG_FUNC(sceNp2, sceNpMatching2GetLobbyMemberIdListLocal);
	REG_FUNC(sceNp2, sceNpMatching2SendLobbyInvitation);
	REG_FUNC(sceNp2, sceNpMatching2ContextStop);
	REG_FUNC(sceNp2, sceNpMatching2Init2);
	REG_FUNC(sceNp2, sceNpMatching2SetLobbyMemberDataInternal);
	REG_FUNC(sceNp2, sceNpMatching2RegisterRoomMessageCallback);
	REG_FUNC(sceNp2, sceNpMatching2SignalingCancelPeerNetInfo);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetLocalNetInfo);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetPeerNetInfo);
	REG_FUNC(sceNp2, sceNpMatching2SignalingGetPeerNetInfoResult);

	REG_FUNC(sceNp2, sceNpAuthOAuthInit);
	REG_FUNC(sceNp2, sceNpAuthOAuthTerm);
	REG_FUNC(sceNp2, sceNpAuthCreateOAuthRequest);
	REG_FUNC(sceNp2, sceNpAuthDeleteOAuthRequest);
	REG_FUNC(sceNp2, sceNpAuthAbortOAuthRequest);
	REG_FUNC(sceNp2, sceNpAuthGetAuthorizationCode);
	REG_FUNC(sceNp2, sceNpAuthGetAuthorizationCode2);
});
