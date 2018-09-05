#pragma once

#include <WinSock2.h>
#include <process.h>
#include <map>
#include <vector>
#include "../packetHeader.h"
#include <mutex>
#include "UserInfo.h"
#include "RoomInfo.h"

class Server;
using std::string;
using std::map;
using std::vector;
using std::make_pair;
using std::mutex;

struct SOCKETINFO
{
	OVERLAPPED overlapped;
	SOCKET sock;
	int roomNum;
	UserInfo user;

	char bufForByteStream[BUFSIZE + 1];
	int recvBytesForByteStream;

	char buf[BUFSIZE + 1];
	int recvbytes;
	WSABUF wsabuf;
	Server* pServer;
};

class Server
{
	int m_iRoomNum;
	map<SOCKET, SOCKETINFO*> m_mapSockInfo;
	map<SOCKET, UserInfo*> m_mapRobbyUser;
	map<int, RoomInfo*> m_mapRoomInfo;
	
	vector<string> m_words;

public:
	void ProcessPacket(SOCKETINFO* pInfo);
	void EreaseUser(SOCKET sock);
	void SendRobbyList();
	void SendRoomList();
	void SendRoomUserInfo(int roomNum);
	void SendRoundPacket(int roomNum);
	void GetWords();
	int StartServer(int port);

	Server();
	~Server();
};

