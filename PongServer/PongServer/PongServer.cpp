// PongServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Exception.h"
#include "Config.h"
#include "Log.h"
#include "Scheduler.h"
#include "ClientManager.h"
#include "GameManager.h"

int main(int argc, char* argv[])
{
	SetUnhandledExceptionFilter(ExceptionFilter);

	int portNum = 9001;
	/// listen port override
	if (argc >= 2)
		portNum = atoi(argv[1]);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	GConsoleLog.reset(new ConsoleLog("./logs/serverLog.txt"));

	/// Managers
	GScheduler.reset(new Scheduler);
	GClientManager.reset(new ClientManager);
	GGameManager.reset(new GameManager);

	/// bind and listen
	if (false == GClientManager->Initialize(portNum))
		return -1;

	printf("Server Started...\n");

	GClientManager->EventLoop(); ///< block here
				
	WSACleanup();
    return 0;
}

