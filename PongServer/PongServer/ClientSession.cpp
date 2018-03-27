#include "stdafx.h"
#include "Exception.h"
#include "Config.h"
#include "SharedStruct.h"
#include "Log.h"
#include "Scheduler.h"
#include "ClientSession.h"
#include "ClientManager.h"
#include "GameManager.h"

ClientSession::~ClientSession()
{
	closesocket(mSocket);
}

void ClientSession::OnConnect(sockaddr_in* addr)
{
	memcpy(&mClientAddr, addr, sizeof(sockaddr_in));

	/// make socket non-blocking
	u_long arg = 1;
	ioctlsocket(mSocket, FIONBIO, &arg);

	/// nagle off
	int opt = 1;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int));

	GConsoleLog->PrintOut(true, "Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
	
	mConnected = true;
}

void ClientSession::OnDisconnect()
{
}

void ClientSession::Disconnect()
{
	if (!IsConnected())
		return;
	
	linger lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(linger)) < 0)
	{
		GConsoleLog->PrintOut(true, "setsockopt linger option error\n");
		return;
	}

	mConnected = false;

	OnDisconnect();

	GConsoleLog->PrintOut(true, "Client Disconnected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GClientManager->DeleteClient(shared_from_this());
}



void ClientSession::OnReceive()
{
	if (!IsConnected())
		return;

	while (true)
	{
		int nread = recv(mSocket, mRecvBuffer.GetBuffer(), mRecvBuffer.GetFreeSpaceSize(), 0);

		if (nread < 0)
		{
			/// no more data
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				break;

			/// if error
			Disconnect();
			return;
		}

		mRecvBuffer.Commit(nread);

		
#ifdef ECHO_MODE
		EchoBack();
#else
		DispatchPacket();
#endif

	}
}

void ClientSession::EchoBack()
{

	size_t len = mRecvBuffer.GetContiguiousBytes();

	if (len == 0)
		return;

	/// buffering first, then flushing
	if (false == mSendBuffer.Write((char*)mRecvBuffer.GetBufferStart(), len))
	{
		Disconnect();
		return;
	}

	mRecvBuffer.Remove(len);

}

bool ClientSession::SendRequest(PacketHeader* pkt)
{
	if ( !IsConnected() )
		return false;

	/// buffering first, then flushing
	if ( false == mSendBuffer.Write((char*)pkt, pkt->mSize) )
	{
		/// insufficient capacity
		Disconnect();
		return false;
	}

	return true;
}

void ClientSession::SendFlush()
{
	if (!IsConnected())
		return;

	/// no more send-data
	if (mSendBuffer.GetContiguiousBytes() == 0)
		return;

	while (mSendBuffer.GetContiguiousBytes() > 0)
	{
		int sent = send(mSocket, mSendBuffer.GetBufferStart(), mSendBuffer.GetContiguiousBytes(), 0);
		if (sent < 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				return;
			
			/// send error
			Disconnect();
			return;
		}

		/// remove data sent
		mSendBuffer.Remove(sent);
	}
}


void ClientSession::OnTick()
{
	if (!IsConnected())
		return;

	/// Periodic work here
	// ...

	
	CallFuncAfter(PLAYER_HEART_BEAT, shared_from_this(), &ClientSession::OnTick);
}


void ClientSession::RacketAction(RacketRequest req)
{
	GGameManager->RacketAction(req);
}

void ClientSession::SendGameStatus(GameStatusBroadcast stat)
{
	SendRequest(&stat);
}



