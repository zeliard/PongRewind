#pragma once
#include <string>
#include "CircularBuffer.h"

#define BUF_SIZE	32768

class NetController
{
public:
	NetController();
	~NetController();

	bool Connect(const std::string& serverAddr, int port);
	void Disconnect();

	/// For GameServer
	void RequestGameStart();
	void RequestRacket(float posDiff, bool shoot, uint32_t recentWF, uint32_t inputFrame, uint64_t hashval);
	void RequestGiveUp();

    void NetworkProcess();
private:

	bool Send(const char* data, int length);
	
	void ProcessPacket();

private:

	bool mConnected;

	SOCKET			mSocket;
	CircularBuffer	mRecvBuffer;
	
	std::string		mServerAddr;
	int				mPortNum;

};

extern std::unique_ptr<NetController> GNetController;
