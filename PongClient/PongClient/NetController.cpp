#include "stdafx.h"
#include "SharedStruct.h"
#include "NetController.h"
#include "GUIController.h"

#pragma comment(lib,"ws2_32.lib")

std::unique_ptr<NetController> GNetController;

NetController::NetController() : mRecvBuffer(BUF_SIZE), mSocket(0), mPortNum(-1), mConnected(false)
{
}

NetController::~NetController()
{
	closesocket(mSocket);
}


bool NetController::Connect(const std::string& serverAddr, int port)
{
	if (mConnected)
		return false;

	mServerAddr = serverAddr;
	mPortNum = port;
	mRecvBuffer.BufferReset();

	mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (mSocket == INVALID_SOCKET)
		return false;

	struct hostent* host;
	struct sockaddr_in hostAddr;

	if ((host = gethostbyname(mServerAddr.c_str())) == 0) 
		return false;

	memset(&hostAddr, 0, sizeof(hostAddr));
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_addr.s_addr = ((struct in_addr *)(host->h_addr_list[0]))->s_addr;
	hostAddr.sin_port = htons(static_cast<u_short>(mPortNum));

	if (SOCKET_ERROR == ::connect(mSocket, (struct sockaddr*)&hostAddr, sizeof(hostAddr)))
	{
		std::cout << "CONNECT FAILED\n";
		return false;
	}

    /// make socket non-blocking
	u_long arg = 1;
	ioctlsocket(mSocket, FIONBIO, &arg);

	/// nagle off
	int opt = 1;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int));

	mConnected = true;

  	return true;
}

void NetController::Disconnect()
{
	mConnected = false;

	if (SOCKET_ERROR == ::shutdown(mSocket, SD_BOTH))
	{
		std::cout << "SOCKET SHUTDOWN FAILED " << WSAGetLastError() << std::endl;
	}

}
bool NetController::Send(const char* data, int length)
{
	int count = 0;
	while (count < length) 
	{
		int n = ::send(mSocket, data + count, length, 0);
		if (n == SOCKET_ERROR)
		{
			std::cout << "SEND ERROR\n";
			return false;
		}
		count += n;
		length -= n;
	}

	return true;
}


void NetController::NetworkProcess()
{
    if ( mConnected )
    {
        char inBuf[4096] = { 0, };

        int n = ::recv(mSocket, inBuf, 4096, 0);

        if (n < 1)
        {
            return;
        }

        if (!mRecvBuffer.Write(inBuf, n))
        {

            assert(false);
        }

        ProcessPacket();
    }
}

void NetController::ProcessPacket()
{
	
	while (true)
	{
		PacketHeader header;

		if (false == mRecvBuffer.Peek((char*)&header, sizeof(PacketHeader)))
			break;
			

		if (header.mSize > mRecvBuffer.GetStoredSize())
			break;
	
		switch (header.mType)
		{
	
		case PKT_SC_START:
		{
			StartResult inPacket;
			bool ret = mRecvBuffer.Read((char*)&inPacket, inPacket.mSize);
			assert(ret);

			GGuiController->OnGameStart(inPacket.mYourPlayerType);
				
		}
		break;
			
		case PKT_SC_GAME_STATUS:
		{
			GameStatusBroadcast inPacket;
			bool ret = mRecvBuffer.Read((char*)&inPacket, inPacket.mSize);
			assert(ret);
			
			GGuiController->OnStatusChange(inPacket);
		}
		break;

		default:
			assert(false);
		}

	}
	
}


void NetController::RequestGameStart()
{
	if (!mConnected)
		return;

	StartRequest sendData;
	Send((const char*)&sendData, sizeof(StartRequest));
}

void NetController::RequestRacket(float posDiff, bool shoot, uint32_t recentWF, uint32_t inputFrame, uint64_t hashval)
{
	if (!mConnected)
		return;

	if (GGuiController->GetMyPlayerType() == PlayerType::PLAYER_NONE)
		return;

	RacketRequest sendData;
	sendData.mPlayerType = GGuiController->GetMyPlayerType();
	sendData.mPosDiff = posDiff;
	sendData.mIsShoot = shoot;
	sendData.mRecentWorldFrame = recentWF;
	sendData.mInputFrame = inputFrame;
	sendData.mStatusHash = hashval;
	
	Send((const char*)&sendData, sizeof(RacketRequest));
}

void NetController::RequestGiveUp()
{
	if (!mConnected)
		return;

	if (GGuiController->GetMyPlayerType() == PlayerType::PLAYER_NONE)
		return;

	ExitRequest sendData;
	sendData.mPlayerType = GGuiController->GetMyPlayerType();

	Send((const char*)&sendData, sizeof(ExitRequest));
}
