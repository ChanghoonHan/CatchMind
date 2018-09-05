#include "RoomInfo.h"

void RoomInfo::AddRoomUser(UserInfo* user)
{
	m_mapRoomUsers.insert(std::make_pair(user->GetSock(), user));
	m_iUserCount++;
	if (m_iUserCount == 1)
	{
		user->SetMaster(true);
		m_roomMaster = user->GetSock();
	}
}

void RoomInfo::GetRoomName(string& roomName)
{
	roomName = m_roomName;
}

void RoomInfo::GetRoomName(char* roomName)
{
	strcpy(roomName, m_roomName.c_str());
}

int RoomInfo::GetRound()
{
	return m_round;
}

void RoomInfo::IncreaseRound()
{
	m_round++;
}

CMGAMESTATE RoomInfo::GetGameState()
{
	return m_gameState;
}

int RoomInfo::GetRoomNum()
{
	return m_roomNum;
}

void RoomInfo::SendPacketInRoomUsers(char* packet, int packetLen)
{
	for (auto iter = m_mapRoomUsers.begin(); iter != m_mapRoomUsers.end(); iter++)
	{
		send(iter->first, packet, packetLen, 0);
	}
}

UserInfo* RoomInfo::SetTurnUserSock()
{
	if (m_round == 1)
	{
		UserInfo* Temp = nullptr;
		m_turnUserSock = m_roomMaster;
		Temp = m_mapRoomUsers.find(m_turnUserSock)->second;
		Temp->SetTurn(true);
		return Temp;
	}
	else
	{
		auto userIterForTurn = m_mapRoomUsers.find(m_turnUserSock);

		userIterForTurn++;
		if (userIterForTurn == m_mapRoomUsers.end())
		{
			userIterForTurn = m_mapRoomUsers.begin();
		}

		m_turnUserSock = userIterForTurn->first;
		userIterForTurn->second->SetTurn(true);

		return userIterForTurn->second;
	}
}

void RoomInfo::DeleteRoomUser(UserInfo* user)
{
	auto iter = m_mapRoomUsers.find(user->GetSock());
	iter++;

	if (iter == m_mapRoomUsers.end())
	{
		iter = m_mapRoomUsers.begin();
	}

	if (user->IsMaster())
	{
		m_roomMaster = iter->first;

		iter->second->SetMaster(true);
	}

	m_mapRoomUsers.erase(user->GetSock());
	m_iUserCount--;
}

void RoomInfo::DecreaseRound()
{
	m_round--;
}

void RoomInfo::InitRound()
{
	m_round = 0;
}

void RoomInfo::InitRoom(int roomNum, SOCKET roomMaster, string roomName)
{
	m_iUserCount = 0;
	m_roomNum = roomNum;
	m_roomMaster = roomMaster;
	m_gameState = GAMESTATE_READY;
	m_roomName = roomName;
	m_round = 0;
}

SOCKET RoomInfo::NowTurn()
{
	return m_turnUserSock;
}

bool RoomInfo::IsSameState(CMGAMESTATE gameState)
{
	if (gameState == m_gameState)
	{
		return true;
	}
	else
	{
		return false;
	}
}

string RoomInfo::CreateAnswer(vector<string>& words)
{
	int randNum = rand() % words.size();

	m_answer = words[randNum];

	return m_answer;
}

bool RoomInfo::CheckCorrectAnswer(string answer)
{
	if (m_answer == answer)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void RoomInfo::SetGameState(CMGAMESTATE gameState)
{
	m_gameState = gameState;
}

map<SOCKET, UserInfo*>* RoomInfo::GetRoomUsers()
{
	return &m_mapRoomUsers;
}


RoomInfo::RoomInfo()
{
}

RoomInfo::~RoomInfo()
{
}
