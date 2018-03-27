// PongClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GUIController.h"
#include "NetController.h"
#include "INIReader.h"


int main(int argc, char** argv)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	INIReader iniReader("config.ini");
	if (iniReader.ParseError() < 0)
	{
		std::cout << "config.ini not found\n";
		return -2;
	}

	const std::string& ipAddr = iniReader.Get("config", "SERVER_IP", "127.0.0.1");
	int portNum = iniReader.GetInteger("config", "PORT_NUM", 9001);
	

	GGuiController.reset(new GUIController());
	GNetController.reset(new NetController());

	GGuiController->Initialize(&argc, argv);

	if (false == GNetController->Connect(ipAddr, portNum))
	{
		std::cout << "Server Connect Error\n";
		return -3;
	}

	GGuiController->DoEventLoop();

	WSACleanup();
    return 0;
}

