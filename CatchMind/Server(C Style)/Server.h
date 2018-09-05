#pragma once

#include <WinSock2.h>
#include <process.h>
#include <map>
#include <vector>
#include "../../packetHeader.h"
#include <mutex>

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
	string name;

	char bufForByteStream[BUFSIZE + 1];
	int recvBytesForByteStream;

	char buf[BUFSIZE + 1];
	int recvbytes;
	WSABUF wsabuf;
	Server* pServer;
};

struct USERINFO
{
	SOCKET socket;
	char name[24];
};

struct ROOMUSERINFO
{
	USERINFO* user;
	int score;
	bool turn;
	bool master;
};

struct ROOMINFO
{
	int roomNum;
	string roomName;
	string answer;
	SOCKET turnUserSock;
	SOCKET roomMaster;
	int round;
	int iUserCount;
	CMGAMESTATE gameState;
	map<SOCKET, ROOMUSERINFO> m_mapRoomUser;
};

class Server
{
	int m_iRoomNum;
	map<SOCKET, SOCKETINFO*> m_mapSockInfo;
	map<SOCKET, USERINFO*> m_mapRobbyUser;
	map<int, ROOMINFO*> m_mapRoomInfo;
	
	vector<string> m_words;

public:
	void ProcessPacket(SOCKETINFO* pInfo);
	void EreaseUser(SOCKET sock);
	void SendRobbyList();
	void SendRoomList();
	void SendRoomUserInfo(int roomNum);
	void SendRoundPacket(SOCKETINFO* pInfo);
	void GetWords();
	int StartServer(int port);

	Server();
	~Server();
};

