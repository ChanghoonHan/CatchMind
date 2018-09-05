#pragma once
#include <WinSock2.h>
#include <string>
#include "../packetHeader.h"

using std::string;

class UserInfo
{
	SOCKET m_socket;
	string m_strName;
	int m_dScore;
	bool m_bTurn;
	bool m_bMaster;

public:
	void YourName(string& name);
	void YourName(char* name);
	void IncreaseScore(int score);
	void InitScore();
	void SetTurn(bool turn);
	bool IsTrun();
	bool IsSameName(string name);
	bool IsMaster();
	void SetMaster(bool master);
	void InitUser(SOCKET sock, string name);
	void InitUserRoomInfo();
	void GetRoomUserInfo(GAMEUSERINFO* InfoTemp);
	SOCKET GetSock();

	UserInfo();
	~UserInfo();
};

