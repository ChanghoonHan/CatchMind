#pragma once
#include <string>
#include <WinSock2.h>
#include <map>
#include <vector>
#include "../packetHeader.h"
#include "UserInfo.h"

using std::string;
using std::map;
using std::vector;

class RoomInfo
{
	int m_roomNum;
	string m_roomName;
	string m_answer;
	SOCKET m_turnUserSock;
	SOCKET m_roomMaster;
	int m_round;
	int m_iUserCount;
	CMGAMESTATE m_gameState;
	map<SOCKET, UserInfo*> m_mapRoomUsers;

public:
	map<SOCKET, UserInfo*>* GetRoomUsers();
	void SetGameState(CMGAMESTATE gameState);
	bool CheckCorrectAnswer(string answer);
	bool IsSameState(CMGAMESTATE gameState);
	string CreateAnswer(vector<string>& words);
	SOCKET NowTurn();
	void InitRoom(int roomNum, SOCKET roomMaster, string roomName);
	void DecreaseRound();
	void InitRound();
	int GetRound();
	CMGAMESTATE GetGameState();
	void IncreaseRound();
	int GetRoomNum();
	void GetRoomName(string& roomName);
	void GetRoomName(char* roomName);
	UserInfo* SetTurnUserSock();
	void AddRoomUser(UserInfo* user);
	void DeleteRoomUser(UserInfo* user);

	void SendPacketInRoomUsers(char* packet, int packetLen);

	RoomInfo();
	~RoomInfo();
};

