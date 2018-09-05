#pragma once

class NetWork
{
	static NetWork* S;
	int sock;

public:
	bool StartNetWork(int port);
	void Release();
	void SendPacket(char* packet, int size);
	int GetSock();

	static NetWork* GetInstance();
	static void Destroy();

	NetWork();
	~NetWork();
};

