
#include "stdafx.h"
#include "Log.h"
#include "SharedStruct.h"
#include "Utils.h"
#include "ClientSession.h"
#include "ClientManager.h"
#include "GameManager.h"
#include "Scheduler.h"

std::unique_ptr<ClientManager> GClientManager(nullptr);


bool ClientManager::Initialize(int& listenPort)
{
	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket < 0)
		return false;

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	/// bind
	sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(sockaddr_in));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(listenPort);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int ret = bind(mListenSocket, (sockaddr*)&serveraddr, sizeof(serveraddr));
	if (ret < 0)
		return false;

	/// listen
	ret = listen(mListenSocket, SOMAXCONN);
	if (ret < 0)
		return false;

	mEpollFd = epoll_create(MAX_CONNECTION);
	if (mEpollFd == nullptr)
		return false;

	epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.sock = mListenSocket;
	ret = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mListenSocket, &ev);
	if (ret < 0)
		return false;

	memset(&serveraddr, 0, sizeof(sockaddr_in));
	int len = sizeof(serveraddr);
	ret = getsockname(mListenSocket, (sockaddr*)&serveraddr, &len);
	if (ret < 0)
		return false;

	listenPort = ntohs(serveraddr.sin_port);

	return true;

}

void ClientManager::EventLoop()
{
	epoll_event events[MAX_CONNECTION];

	int64_t nextUpdateTick = GScheduler->GetCurrentTick();

	while (true)
	{
		int nfd = epoll_wait(mEpollFd, events, MAX_CONNECTION, UPDATE_INTERVAL/2);

		for (int i = 0; i < nfd; ++i)
		{
			if (events[i].data.sock == mListenSocket)
			{
				sockaddr_in clientAddr;
				int clientAddrLen = sizeof(sockaddr_in);

				SOCKET client = accept(mListenSocket, (sockaddr *)&clientAddr, &clientAddrLen);
				if (client < 0) 
				{
					GConsoleLog->PrintOut(true, "accept error: IP=%s, PORT=%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
					continue;
				}

				auto newClient = CreateClient(client);
				newClient->OnConnect(&clientAddr);

				epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.events = EPOLLIN;
				ev.data.sock = client;
				epoll_ctl(mEpollFd, EPOLL_CTL_ADD, client, &ev);
			}
			else
			{
				SOCKET client = events[i].data.sock;
				auto it = mClientList.find(client);
				if (it != mClientList.end())
				{
					it->second->OnReceive();
				}
		
			}
		}

		/// scheduled jobs
		GScheduler->DoTasks();

		/// game logic update
		while (true)
		{
			int64_t currTick = GScheduler->GetCurrentTick();

			if (currTick >= nextUpdateTick)
			{
				GGameManager->OnFrameUpdate();

				nextUpdateTick = currTick + UPDATE_INTERVAL;
			}
			else
			{
				break;
			}
		}

		/// packet broadcast
		FlushClientSend();
	}

}

std::shared_ptr<ClientSession> ClientManager::CreateClient(SOCKET sock)
{
	auto client = std::make_shared<ClientSession>(sock);
	client->mUniqueId = ++mCurrentIssuedId;
	mClientList.insert(ClientList::value_type(sock, client)) ;

	return client ;
}

void ClientManager::DeleteClient(std::shared_ptr<ClientSession> client)
{
	mClientList.erase(client->mSocket);
}

void ClientManager::BroadcastPacket(PacketHeader* pkt)
{
	for (auto it : mClientList)
	{
		it.second->SendRequest(pkt);
	}
}

void ClientManager::FlushClientSend()
{
	for (auto it : mClientList)
	{
		it.second->SendFlush();
	}
}

void ClientManager::DelayedBroadcastGameStatus(uint32_t delay, const GameStatusBroadcast& stat)
{
	for (auto it : mClientList)
	{
		CallFuncAfter(delay, it.second, &ClientSession::SendGameStatus, stat);
	}
	
}