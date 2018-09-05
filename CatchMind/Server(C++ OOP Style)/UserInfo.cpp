#include "UserInfo.h"

void UserInfo::GetRoomUserInfo(GAMEUSERINFO* InfoTemp)
{
	InfoTemp->master = m_bMaster;
	strcpy(InfoTemp->name, m_strName.c_str());
	InfoTemp->score = m_dScore;
	InfoTemp->turn = m_bTurn;
}

SOCKET UserInfo::GetSock()
{
	return m_socket;
}

bool UserInfo::IsTrun()
{
	return m_bTurn;
}

bool UserInfo::IsMaster()
{
	return m_bMaster;
}

void UserInfo::SetMaster(bool master)
{
	m_bMaster = master;
}
 
void UserInfo::SetTurn(bool turn)
{
	m_bTurn = turn;
}

void UserInfo::IncreaseScore(int score)
{
	m_dScore += score;
}

void UserInfo::InitScore()
{
	m_dScore = 0;
}

void UserInfo::YourName(string& name)
{
	name = m_strName;
}

void UserInfo::YourName(char* name)
{
	strcpy(name, m_strName.c_str());
}

bool UserInfo::IsSameName(string name)
{
	if (name == m_strName)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void UserInfo::InitUser(SOCKET sock, string name)
{
	m_socket = sock;
	m_strName = name;
	m_bMaster = false;
	m_bTurn = false;
	m_dScore = 0;
}

void UserInfo::InitUserRoomInfo()
{
	m_bTurn = false;
	m_dScore = 0;
	m_bMaster = false;
}

UserInfo::UserInfo()
{
}


UserInfo::~UserInfo()
{
}
