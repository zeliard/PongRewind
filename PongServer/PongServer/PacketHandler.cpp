#include "stdafx.h"
#include "Log.h"
#include "Scheduler.h"
#include "SharedStruct.h"
#include "ClientSession.h"
#include "GameManager.h"


//@{ Handler Helper

typedef std::shared_ptr<ClientSession> ClientSessionPtr;

typedef void(*HandlerFunc)(ClientSessionPtr session);

static HandlerFunc HandlerTable[PKT_MAX];

static void DefaultHandler(ClientSessionPtr session)
{
	GConsoleLog->PrintOut(true, "Invalid packet handler");
	session->Disconnect();
}

struct InitializeHandlers
{
	InitializeHandlers()
	{
		for (int i = 0; i < PKT_MAX; ++i)
			HandlerTable[i] = DefaultHandler;
	}
} _init_handlers_;

struct RegisterHandler
{
	RegisterHandler(int pktType, HandlerFunc handler)
	{
		HandlerTable[pktType] = handler;
	}
};

#define REGISTER_HANDLER(PKT_TYPE)	\
	static void Handler_##PKT_TYPE(ClientSessionPtr session); \
	static RegisterHandler _register_##PKT_TYPE(PKT_TYPE, Handler_##PKT_TYPE); \
	static void Handler_##PKT_TYPE(ClientSessionPtr session)

//@}

///////////////////////////////////////////////////////////

void ClientSession::DispatchPacket()
{
	/// packet parsing
	while (true)
	{
		/// read packet header
		PacketHeader header;
		if (false == mRecvBuffer.Peek((char*)&header, sizeof(PacketHeader)))
			return;

		/// packet completed? 
		if (mRecvBuffer.GetStoredSize() < (size_t)header.mSize)
			return;

		if (header.mType >= PKT_MAX || header.mType <= PKT_NONE)
		{
			GConsoleLog->PrintOut(true, "Invalid packet type\n");
			Disconnect();
			return;
		}

		/// packet dispatch...
		HandlerTable[header.mType](shared_from_this());
	}
}

/////////////////////////////////////////////////////////
REGISTER_HANDLER(PKT_CS_START)
{
	StartRequest inPacket;
	if (false == session->ParsePacket(inPacket))
	{
		GConsoleLog->PrintOut(true, "packet parsing error, Type: %d\n", inPacket.mType);
		return;
	}

	auto playerType = GGameManager->StartGame(session->GetMyUid());
	if (playerType == PlayerType::PLAYER_NONE)
		return;
	
	StartResult outPacket;
	outPacket.mYourPlayerType = playerType;
	session->SendRequest(&outPacket);
}


REGISTER_HANDLER(PKT_CS_EXIT)
{
	ExitRequest inPacket;
	if (false == session->ParsePacket(inPacket))
	{
		GConsoleLog->PrintOut(true, "packet parsing error: %d\n", inPacket.mType);
		return;
	}

	if (inPacket.mPlayerType != GGameManager->GetPlayerType(session->GetMyUid()))
	{
		GConsoleLog->PrintOut(true, "[CS_EXIT] Player Type Mismatch\n");
		return;
	}

	GGameManager->RequestGiveUp(inPacket.mPlayerType);
}

REGISTER_HANDLER(PKT_CS_RACKET)
{
	RacketRequest inPacket;
	if (false == session->ParsePacket(inPacket))
	{
		GConsoleLog->PrintOut(true, "packet parsing error, Type: %d\n", inPacket.mType);
		return;
	}

	if (inPacket.mPlayerType != GGameManager->GetPlayerType(session->GetMyUid()))
	{
		GConsoleLog->PrintOut(true, "[CS_MOVE_RACKET] Player Type Mismatch\n");
		return;
	}

	
	//TEST: PacketQueueing... 패킷 임의로 늦추기 실험
	uint32_t delay = rand() % 200; // ms
	CallFuncAfter(delay, session, &ClientSession::RacketAction, inPacket);

	//session->RacketAction(inPacket);
	
}




