#pragma once


#include "Config.h"
#include "SharedStruct.h"
#include "CircularBuffer.h"


class ClientSession;
class ClientManager;


class ClientSession : public std::enable_shared_from_this<ClientSession>
{
public:
	ClientSession(SOCKET sock)
		: mConnected(false), mUniqueId(0), mSocket(sock), mSendBuffer(BUFSIZE), mRecvBuffer(BUFSIZE)
	{
		memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
	}

	virtual ~ClientSession();

	void	OnConnect(SOCKADDR_IN* addr);
	void	OnDisconnect();
	void	Disconnect();

	void	OnReceive();

	bool	SendRequest(PacketHeader* pkt);
	void	SendFlush(); 

	int		GetMyUid() const { return mUniqueId; }

	
	void	RacketAction(RacketRequest req);
	void	SendGameStatus(GameStatusBroadcast stat);

public: 
	bool	IsConnected() const { return mConnected; }
	void	OnTick();


	template <class PKT_TYPE>
	bool ParsePacket(PKT_TYPE& pkt)
	{
		return mRecvBuffer.Read((char*)&pkt, pkt.mSize);
	}

	void DispatchPacket();
	void EchoBack();

private:

	bool			mConnected;
	int 			mUniqueId;
	SOCKET			mSocket;

	SOCKADDR_IN		mClientAddr;

	CircularBuffer	mSendBuffer;
	CircularBuffer	mRecvBuffer;

	friend class ClientManager;
};


