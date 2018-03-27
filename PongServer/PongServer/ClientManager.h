#pragma once

#include "ClientSession.h"

class ClientManager
{
public:
	ClientManager() : mEpollFd(nullptr), mListenSocket(-1), mCurrentIssuedId(0)
	{}

	bool Initialize(int& listenPort);

	std::shared_ptr<ClientSession> CreateClient(SOCKET sock);
	void DeleteClient(std::shared_ptr<ClientSession> client);

	void EventLoop();

	void BroadcastPacket(PacketHeader* pkt);

	void DelayedBroadcastGameStatus(uint32_t delay, const GameStatusBroadcast& stat);

	void FlushClientSend();


private:
	typedef std::map<SOCKET, std::shared_ptr<ClientSession>> ClientList;
	ClientList	mClientList;
	int mCurrentIssuedId;

	HANDLE		mEpollFd;
	SOCKET		mListenSocket;
} ;

extern std::unique_ptr<ClientManager> GClientManager;
