#pragma once

#include "Server.h"

mutex g_mutex;

void err_quit(char *msg);
void err_display(char *msg);
unsigned int WINAPI WorkerThread(LPVOID arg);

int Server::StartServer(int port)
{
	m_iRoomNum = 0;

	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}

	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (hcp == NULL)
	{
		return -1;
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	unsigned int hThread;
	unsigned int ThreadId;
	for (size_t i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	{
		hThread = _beginthreadex(NULL, 0, WorkerThread, hcp, 0, &ThreadId);

		if (hThread == NULL)
		{
			return -1;
		}

		CloseHandle((HANDLE)hThread);
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);//접속 대기용 소켓
	if (listen_sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	SOCKADDR_IN serveraddr;//자기 정보 셋팅(바인드)
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("bind()");
	}

	retval = listen(listen_sock, SOMAXCONN);//리슨상태로 바꿔줌
	if (retval == SOCKET_ERROR)
	{
		err_quit("listen()");
	}

	while (1)
	{
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);

		SOCKET client_sock;
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display("accept()");
			continue;
		}

		printf("\n[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		HANDLE hResult = CreateIoCompletionPort((HANDLE)client_sock, hcp, (DWORD)client_sock, 0);

		if (hResult == NULL)
		{
			return -1;
		}

		SOCKETINFO *ptr = new SOCKETINFO;
		if (ptr == NULL)
		{
			printf("[오류] 메모리가 부족합니다!\n");
			break;
		}

		ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));
		ZeroMemory(&(ptr->bufForByteStream), sizeof(ptr->bufForByteStream));
		ptr->recvBytesForByteStream = 0;
		ptr->sock = client_sock;
		ptr->recvbytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;
		ptr->pServer = this;
		ptr->roomNum = 0;

		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(client_sock, &(ptr->wsabuf), 1, &recvbytes, &flags, &(ptr->overlapped), NULL);
		g_mutex.lock();
		m_mapSockInfo.insert(std::make_pair(ptr->sock, ptr));
		g_mutex.unlock();

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				err_display("WSARecv()");
			}
			continue;
		}
	}

	WSACleanup();
	return 0;
}

unsigned int WINAPI WorkerThread(LPVOID arg)
{
	srand(GetTickCount());

	HANDLE hcp = (HANDLE)arg;
	int retval;

	while (true)
	{
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO *ptr;
		retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (LPDWORD)&client_sock, (LPOVERLAPPED *)&ptr, INFINITE);

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)&clientaddr, &addrlen);

		if (retval == 0 || cbTransferred == 0)//종료메세지 도착
		{
			if (retval == 0)
			{
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &(ptr->overlapped), &temp1, FALSE, &temp2);
				err_display("WSAGetOverlappedResult()");
			}
	
			ptr->pServer->EreaseUser(ptr->sock);


			printf("[TCP 서버] 클라이언트 종료 : IP 주소 = %s, 포트 번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			continue;
		}

		while (true)
		{
			ptr->recvbytes = cbTransferred;

			ptr->pServer->ProcessPacket(ptr);

			if (ptr->recvBytesForByteStream >= sizeof(PACKET_HEADER))
				continue;

			if (ptr->recvBytesForByteStream == 0)
				break;
		}

		ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));//다시recv걸어줌
		ptr->recvbytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(client_sock, &(ptr->wsabuf), 1, &recvbytes, &flags, &(ptr->overlapped), NULL);

		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				err_display("WSARecv()");
			}
			continue;
		}

	}

	return 0;
}

void Server::ProcessPacket(SOCKETINFO* pInfo)
{
	memcpy(&pInfo->bufForByteStream[pInfo->recvBytesForByteStream], pInfo->buf, pInfo->recvbytes);
	pInfo->recvBytesForByteStream += pInfo->recvbytes;

	if (pInfo->recvBytesForByteStream < sizeof(PACKET_HEADER))
		return;

	PACKET_HEADER header;
	memcpy(&header, pInfo->bufForByteStream, sizeof(header));

	if (pInfo->recvBytesForByteStream < header.packetLen)
		return;

	switch (header.type)
	{
	case PACKET_TYPE_CHAT:
	{
		PACKET_CHAT chatPacket;
		memcpy(&chatPacket, pInfo->bufForByteStream, sizeof(chatPacket));
		printf("%s : %s\n", chatPacket.name, chatPacket.buf);
		int retval;

		g_mutex.lock();
		for (auto iter = m_mapRobbyUser.begin(); iter != m_mapRobbyUser.end(); iter++)
		{
			retval = send(iter->first, (char*)&chatPacket, chatPacket.header.packetLen, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				EreaseUser(iter->first);
				return;
			}
		}
		g_mutex.unlock();
	}
	break;

	case PACKET_TYPE_TIME_OVER:
	{
		PACKET_TIME_OVER TimeOverPacket;
		memcpy(&TimeOverPacket, pInfo->bufForByteStream, sizeof(TimeOverPacket));
		int retval;

		g_mutex.lock();
		auto room = m_mapRoomInfo.find(pInfo->roomNum);
		room->second->SendPacketInRoomUsers((char*)&TimeOverPacket, TimeOverPacket.header.packetLen);
		g_mutex.unlock();
		room->second->SetGameState(GAMESTATE_STAND);
		Sleep(2000);

		SendRoundPacket(room->second->GetRoomNum());
	}
	break;

	case PACKET_TYPE_GAME_CHAT:
	{
		PACKET_GAME_CHAT gameChatPacket;
		memcpy(&gameChatPacket, pInfo->bufForByteStream, sizeof(gameChatPacket));
		printf("%s : %s\n", gameChatPacket.name, gameChatPacket.buf);
		int retval;
		
		g_mutex.lock();
		auto room = m_mapRoomInfo.find(pInfo->roomNum);
		room->second->SendPacketInRoomUsers((char*)&gameChatPacket, gameChatPacket.header.packetLen);
		g_mutex.unlock();

		if (room->second->CheckCorrectAnswer(gameChatPacket.buf) && room->second->IsSameState(GAMESTATE_GAME))
		{
			PACKET_CORRECT correctPacket;
			correctPacket.header.packetLen = sizeof(correctPacket);
			correctPacket.header.type = PACKET_TYPE_CORRECT;
			pInfo->user.YourName(correctPacket.answererName);
			room->second->SetGameState(GAMESTATE_CORRECT);

			g_mutex.lock();
			room->second->SendPacketInRoomUsers((char*)&correctPacket, correctPacket.header.packetLen);
			g_mutex.unlock();

			Sleep(2000);
			auto user = room->second->GetRoomUsers()->find(pInfo->sock);
			user->second->IncreaseScore(1);
			user = room->second->GetRoomUsers()->find(room->second->NowTurn());
			user->second->IncreaseScore(1);
			room->second->SetGameState(GAMESTATE_STAND);

			SendRoundPacket(room->second->GetRoomNum());
		}

	}
	break;

	case PACKET_TYPE_LINE:
	{
		PACKET_LINE linePacket;
		memcpy(&linePacket, pInfo->bufForByteStream, sizeof(linePacket));
		int retval;

		g_mutex.lock();
		auto room = m_mapRoomInfo.find(pInfo->roomNum);
		room->second->SendPacketInRoomUsers((char*)&linePacket, linePacket.header.packetLen);
		g_mutex.unlock();
	}
	break;

	case PACKET_TYPE_ERASE:
	{
		PACKET_ERASE erasePacket;
		memcpy(&erasePacket, pInfo->bufForByteStream, sizeof(erasePacket));
		int retval;

		g_mutex.lock();
		auto room = m_mapRoomInfo.find(pInfo->roomNum);
		room->second->SendPacketInRoomUsers((char*)&erasePacket, erasePacket.header.packetLen);
		g_mutex.unlock();
	}
	break;

	case PACKET_TYPE_LOGIN:
	{
		PACKET_LOGIN loginPacket;
		g_mutex.lock();
		memcpy(&loginPacket, pInfo->bufForByteStream, sizeof(loginPacket));
		for (auto iter = m_mapSockInfo.begin(); iter != m_mapSockInfo.end(); iter++)
		{
			if (iter->second->user.IsSameName(loginPacket.name))
			{
				int namelen = strlen(loginPacket.name);
				strcat(&loginPacket.name[namelen], "1");
				break;
			}
		}
		g_mutex.unlock();
		printf("%s님 로비로 접속\n", loginPacket.name);

		pInfo->user.InitUser(pInfo->sock, loginPacket.name);

		g_mutex.lock();
		m_mapRobbyUser.insert(std::make_pair(pInfo->sock, &pInfo->user));
		g_mutex.unlock();
		
		int retval;

		PACKET_LOGIN_RESULT loginResultPacket;
		loginResultPacket.header.type = PACKET_TYPE_LOGIN_RESULT;
		loginResultPacket.header.packetLen = sizeof(PACKET_LOGIN_RESULT);
		strcpy(loginResultPacket.name, loginPacket.name);

		Sleep(10);
	
		send(pInfo->sock, (char*)&loginResultPacket, loginResultPacket.header.packetLen, 0);

		Sleep(10);
		SendRoomList();

		Sleep(10);
		SendRobbyList();
	}
	break;

	case PACKET_TYPE_MAKE_ROOM:
	{
		PACKET_MAKE_ROOM makeRoomPacket;
		memcpy(&makeRoomPacket, pInfo->bufForByteStream, sizeof(makeRoomPacket));
		printf("%s 방이 만들어짐\n", makeRoomPacket.roomName);
		m_iRoomNum++;

		RoomInfo* newRoom = new RoomInfo;
		newRoom->InitRoom(m_iRoomNum, pInfo->sock, makeRoomPacket.roomName);

		m_mapRobbyUser.erase(pInfo->sock);

		string name;
		pInfo->user.YourName(name);
		printf("%s님이 로비에서 나감\n", name);
		SendRobbyList();

		pInfo->roomNum = m_iRoomNum;
		g_mutex.lock();
		newRoom->AddRoomUser(&pInfo->user);
		m_mapRoomInfo.insert(make_pair(m_iRoomNum, newRoom));
		g_mutex.unlock();
		
		PACKET_CHANGE_ROOM pcr;
		pcr.header.packetLen = sizeof(pcr);
		pcr.header.type = PACKET_TYPE_CHANGE_ROOM;

		Sleep(10);
		send(pInfo->sock, (char*)&pcr, pcr.header.packetLen, 0);

		PACKET_LOGIN_RESULT loginResultPacket;
		loginResultPacket.header.type = PACKET_TYPE_LOGIN_RESULT;
		loginResultPacket.header.packetLen = sizeof(PACKET_LOGIN_RESULT);
		pInfo->user.YourName(loginResultPacket.name);

		Sleep(10);
		send(pInfo->sock, (char*)&loginResultPacket, loginResultPacket.header.packetLen, 0);

		Sleep(50);

		SendRoomUserInfo(m_iRoomNum);

		Sleep(10);// 이후로 룸 정보 보냄

		SendRoomList();
	}
	break;

	case PACKET_TYPE_WANT_GO_ROOM:
	{
		PACKET_WANT_GO_ROOM wantGoRoomPacket;
		
		memcpy(&wantGoRoomPacket, pInfo->bufForByteStream, sizeof(wantGoRoomPacket));

		if (wantGoRoomPacket.roomNum == 0)
		{
			int lastRoomNum = pInfo->roomNum;
			pInfo->roomNum = 0;

			g_mutex.lock();
			auto roomIter = m_mapRoomInfo.find(lastRoomNum);
			m_mapRobbyUser.insert(make_pair(pInfo->sock, &pInfo->user));
			g_mutex.unlock();

			g_mutex.lock();
			bool isTurn = roomIter->second->GetRoomUsers()->find(pInfo->sock)->second->IsTrun();
			roomIter->second->DeleteRoomUser(&pInfo->user);
			g_mutex.unlock();


			if (roomIter->second->GetRoomUsers()->size() == 1)//게임 레디상태를 보내줌
			{
				PACKET_READY_GAME readyGamePacket;
				roomIter->second->SetGameState(GAMESTATE_READY);
				readyGamePacket.header.type = PACKET_TYPE_READY_GAME;
				readyGamePacket.header.packetLen = sizeof(readyGamePacket);

				g_mutex.lock();
				roomIter->second->InitRound();
				for (auto iter = roomIter->second->GetRoomUsers()->begin(); iter != roomIter->second->GetRoomUsers()->end(); iter++)
				{
					iter->second->InitUserRoomInfo();
					iter->second->SetMaster(true);
					send(iter->first, (char*)&readyGamePacket, readyGamePacket.header.packetLen, 0);
				}
				
				g_mutex.unlock();
			}
			else if(roomIter->second->GetRoomUsers()->size() != 0)
			{
				if (isTurn)
				{
					roomIter->second->DecreaseRound();
					SendRoundPacket(roomIter->second->GetGameState());
				}
			}

			if (roomIter->second->GetRoomUsers()->size() == 0)
			{
				g_mutex.lock();
				m_mapRoomInfo.erase(lastRoomNum);
				g_mutex.unlock();
			}
			else
			{
				SendRoomUserInfo(lastRoomNum);
			}

			PACKET_LOGIN_RESULT loginResultPacket;
			loginResultPacket.header.type = PACKET_TYPE_LOGIN_RESULT;
			loginResultPacket.header.packetLen = sizeof(PACKET_LOGIN_RESULT);
			pInfo->user.YourName(loginResultPacket.name);

			Sleep(10);
			send(pInfo->sock, (char*)&loginResultPacket, loginResultPacket.header.packetLen, 0);
		}
		else
		{
			UserInfo* userInfo = m_mapRobbyUser.find(pInfo->sock)->second;
			userInfo->InitUserRoomInfo();

			g_mutex.lock();
			m_mapRobbyUser.erase(pInfo->sock);
			pInfo->roomNum = wantGoRoomPacket.roomNum;
			m_mapRoomInfo.find(pInfo->roomNum)->second->AddRoomUser(userInfo);
			g_mutex.unlock();

			PACKET_CHANGE_ROOM pcr;
			pcr.header.packetLen = sizeof(pcr);
			pcr.header.type = PACKET_TYPE_CHANGE_ROOM;
			Sleep(10);
			send(pInfo->sock, (char*)&pcr, pcr.header.packetLen, 0);

			PACKET_LOGIN_RESULT loginResultPacket;
			loginResultPacket.header.type = PACKET_TYPE_LOGIN_RESULT;
			loginResultPacket.header.packetLen = sizeof(PACKET_LOGIN_RESULT);
			pInfo->user.YourName(loginResultPacket.name);

			Sleep(10);
			send(pInfo->sock, (char*)&loginResultPacket, loginResultPacket.header.packetLen, 0);
			Sleep(50);
			SendRoomUserInfo(wantGoRoomPacket.roomNum);
		}

		Sleep(10);
		SendRobbyList();

		Sleep(10);// 이후로 룸 정보 보냄
		SendRoomList();
	}
	break;

	case PACKET_TYPE_GAME_START:
	{
		PACKET_GAME_START gameStartPacket;

		memcpy(&gameStartPacket, pInfo->bufForByteStream, sizeof(gameStartPacket));

		g_mutex.lock();
		auto roomInfoIter = m_mapRoomInfo.find(pInfo->roomNum);
		if (!roomInfoIter->second->IsSameState(GAMESTATE_READY) || roomInfoIter->second->GetRoomUsers()->size() < 2)
		{
			g_mutex.unlock();
			break;
		}
		g_mutex.unlock();

		roomInfoIter->second->SetGameState(GAMESTATE_GAME);
		SendRoomList();
		SendRoundPacket(roomInfoIter->second->GetRoomNum());
	}
	break;
	}

	memcpy(pInfo->bufForByteStream, &pInfo->bufForByteStream[pInfo->recvBytesForByteStream - header.packetLen],
		pInfo->recvBytesForByteStream - header.packetLen);
	pInfo->recvBytesForByteStream -= header.packetLen;

	return;
}

void Server::EreaseUser(SOCKET sock)
{
	closesocket(sock);

	auto sockInfoIter = m_mapSockInfo.find(sock);

	if (sockInfoIter->second->roomNum == 0)
	{
		auto robbyIter = m_mapRobbyUser.find(sock);
		if (robbyIter != m_mapRobbyUser.end())
		{
			g_mutex.lock();
			m_mapRobbyUser.erase(robbyIter);
			g_mutex.unlock();
			SendRobbyList();
		}
	}
	else
	{
		auto roomIter = m_mapRoomInfo.find(sockInfoIter->second->roomNum);

		bool isTurn = roomIter->second->GetRoomUsers()->find(sock)->second->IsTrun();
		auto nextUser = roomIter->second->GetRoomUsers()->find(sock);
		nextUser++;
		if (nextUser == roomIter->second->GetRoomUsers()->end())
		{
			nextUser = roomIter->second->GetRoomUsers()->begin();
		}

		SOCKETINFO* sockInfo = m_mapSockInfo.find(nextUser->first)->second;

		g_mutex.lock();
		roomIter->second->DeleteRoomUser(&m_mapSockInfo.find(sock)->second->user);
		g_mutex.unlock();

		SendRoomUserInfo(sockInfoIter->second->roomNum);

		if (roomIter->second->GetRoomUsers()->size() == 1)//게임 레디상태를 보내줌
		{
			PACKET_READY_GAME readyGamePacket;
			roomIter->second->InitRound();
			roomIter->second->SetGameState(GAMESTATE_READY);
			readyGamePacket.header.type = PACKET_TYPE_READY_GAME;
			readyGamePacket.header.packetLen = sizeof(readyGamePacket);

			g_mutex.lock();
			for (auto iter = roomIter->second->GetRoomUsers()->begin(); iter != roomIter->second->GetRoomUsers()->end(); iter++)
			{
				iter->second->InitUserRoomInfo();
				send(iter->first, (char*)&readyGamePacket, readyGamePacket.header.packetLen, 0);
			}
			g_mutex.unlock();
		}
		else if (roomIter->second->GetRoomUsers()->size() != 0)
		{
			if (isTurn)
			{
				roomIter->second->DecreaseRound();
				SendRoundPacket(roomIter->second->GetRoomNum());
			}
		}

		if (roomIter->second->GetRoomUsers()->size() == 0)//빈방 제거
		{
			g_mutex.lock();
			delete(m_mapRoomInfo.find(sockInfoIter->second->roomNum)->second);
			m_mapRoomInfo.erase(sockInfoIter->second->roomNum);
			g_mutex.unlock();
		}
		else
		{
			SendRoomUserInfo(sockInfoIter->second->roomNum);
		}
		
		SendRoomList();
	}
	

	if (sockInfoIter != m_mapSockInfo.end())
	{
		g_mutex.lock();
		delete(sockInfoIter->second);
		m_mapSockInfo.erase(sockInfoIter);
		g_mutex.unlock();
	}
}

void Server::SendRobbyList()
{
	PACKET_USER_LIST userListPacket;
	userListPacket.header.type = PACKET_TYPE_USER_LIST;
	userListPacket.header.packetLen = sizeof(PACKET_HEADER) + sizeof(int) + (sizeof(UserInfo) * m_mapRobbyUser.size());
	userListPacket.iUserCount = m_mapRobbyUser.size();
	int i = 0;
	g_mutex.lock();
	for (auto iter = m_mapRobbyUser.begin(); iter != m_mapRobbyUser.end(); iter++)
	{
		iter->second->YourName(userListPacket.userList[i]);
		i++;
	}

	int retval = 0;
	for (auto iter = m_mapRobbyUser.begin(); iter != m_mapRobbyUser.end(); iter++)
	{
		retval = send(iter->first, (char*)&userListPacket, userListPacket.header.packetLen, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			EreaseUser(iter->first);
		}
	}
	g_mutex.unlock();
}

void Server::SendRoomList()
{
	PACKET_ROOM_LIST packetRoomList;
	packetRoomList.header.type = PACKET_TYPE_ROOM_LIST;
	packetRoomList.header.packetLen = sizeof(PACKET_HEADER) + sizeof(int) + (sizeof(ROOMINFOFORPACKET) * m_mapRoomInfo.size());
	packetRoomList.roomCount = m_mapRoomInfo.size();
	int i = 0;
	g_mutex.lock();
	for (auto iter = m_mapRoomInfo.begin(); iter != m_mapRoomInfo.end(); iter++)
	{
		if (!iter->second->IsSameState(GAMESTATE_READY))
		{
			packetRoomList.header.packetLen -= sizeof(ROOMINFOFORPACKET);
			packetRoomList.roomCount--;
			continue;
		}

		iter->second->GetRoomName(packetRoomList.room[i].roomName);
		packetRoomList.room[i].roomNum = iter->first;
		packetRoomList.room[i].userNum = iter->second->GetRoomUsers()->size();
		i++;
	}

	int retval = 0;
	for (auto iter = m_mapRobbyUser.begin(); iter != m_mapRobbyUser.end(); iter++)
	{
		retval = send(iter->first, (char*)&packetRoomList, packetRoomList.header.packetLen, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			EreaseUser(iter->first);
		}
	}
	g_mutex.unlock();
}

void Server::SendRoomUserInfo(int roomNum)
{
	auto iter = m_mapRoomInfo.find(roomNum);
	PACKET_ROOM_USER_INFO_LIST packetRoomUserInfoList;
	packetRoomUserInfoList.header.type = PACKET_TYPE_ROOM_USER_INFO_LIST;
	int x = sizeof(PACKET_HEADER) + sizeof(int) + (sizeof(GAMEUSERINFO) * iter->second->GetRoomUsers()->size());
	packetRoomUserInfoList.header.packetLen = x;
	packetRoomUserInfoList.userNum = iter->second->GetRoomUsers()->size();
	g_mutex.lock();
	int i = 0;
	for (auto iter2 = iter->second->GetRoomUsers()->begin(); iter2 != iter->second->GetRoomUsers()->end(); iter2++)
	{
		iter2->second->GetRoomUserInfo(&packetRoomUserInfoList.user[i]);
		packetRoomUserInfoList.gameState = iter->second->GetGameState();
		i++;
	}

	for (auto iter2 = iter->second->GetRoomUsers()->begin(); iter2 != iter->second->GetRoomUsers()->end(); iter2++)
	{
		send(iter2->first, (char*)&packetRoomUserInfoList, packetRoomUserInfoList.header.packetLen, 0);
	}
	g_mutex.unlock();
}

void Server::SendRoundPacket(int roomNum)
{
	auto roomInfoIter = m_mapRoomInfo.find(roomNum);

	if (roomInfoIter->second->GetRound() == 4)
	{
		PACKET_END_GAME endGamePacket;
		roomInfoIter->second->SetGameState(GAMESTATE_RECORD);
		roomInfoIter->second->InitRound();
		endGamePacket.header.type = PACKET_TYPE_END_GAME;
		endGamePacket.header.packetLen = sizeof(endGamePacket);
		g_mutex.lock();
		for (auto iter = roomInfoIter->second->GetRoomUsers()->begin(); iter != roomInfoIter->second->GetRoomUsers()->end(); iter++)
		{
			send(iter->first, (char*)&endGamePacket, endGamePacket.header.packetLen, 0);
		}

		Sleep(2000);

		PACKET_READY_GAME readyGamePacket;
		roomInfoIter->second->SetGameState(GAMESTATE_READY);
		readyGamePacket.header.type = PACKET_TYPE_READY_GAME;
		readyGamePacket.header.packetLen = sizeof(readyGamePacket);

		for (auto iter = roomInfoIter->second->GetRoomUsers()->begin(); iter != roomInfoIter->second->GetRoomUsers()->end(); iter++)
		{
			if (iter->second->IsMaster())
			{
				iter->second->InitUserRoomInfo();
				iter->second->SetMaster(true);
			}
			else
			{
				iter->second->InitUserRoomInfo();
			}
			send(iter->first, (char*)&readyGamePacket, readyGamePacket.header.packetLen, 0);
		}
		g_mutex.unlock();

		Sleep(10);
		SendRoomUserInfo(roomInfoIter->second->GetRoomNum());

		return;
	}

	PACKET_TURN_INFO turnInfoPacket;
	turnInfoPacket.header.type = PACKET_TYPE_TURN_INFO;
	turnInfoPacket.header.packetLen = sizeof(turnInfoPacket);

	roomInfoIter->second->IncreaseRound();

	turnInfoPacket.round = roomInfoIter->second->GetRound();

	strcpy(turnInfoPacket.answer, roomInfoIter->second->CreateAnswer(m_words).c_str());
	roomInfoIter->second->SetGameState(GAMESTATE_STAND);
	
	g_mutex.lock();
	for (auto iter = roomInfoIter->second->GetRoomUsers()->begin(); iter != roomInfoIter->second->GetRoomUsers()->end(); iter++)
	{
		iter->second->SetTurn(false);
	}
	g_mutex.unlock();

	UserInfo* turnUserInfo = roomInfoIter->second->SetTurnUserSock();
	turnUserInfo->YourName(turnInfoPacket.curTurnUserName);

	g_mutex.lock();
	auto userIter = roomInfoIter->second->GetRoomUsers()->find(turnUserInfo->GetSock());
	userIter++;
	if (userIter == roomInfoIter->second->GetRoomUsers()->end())
	{
		userIter = roomInfoIter->second->GetRoomUsers()->begin();
	}
	userIter->second->YourName(turnInfoPacket.nextTurnUserName);
	g_mutex.unlock();
	SendRoomUserInfo(roomInfoIter->second->GetRoomNum());
	
	Sleep(10);

	g_mutex.lock();
	for (auto iter = roomInfoIter->second->GetRoomUsers()->begin(); iter != roomInfoIter->second->GetRoomUsers()->end(); iter++)
	{
		send(iter->first, (char*)&turnInfoPacket, turnInfoPacket.header.packetLen, 0);
	}
	g_mutex.unlock();

	Sleep(2000);
	PACKET_TURN_START turnStartPacket;
	turnStartPacket.header.packetLen = sizeof(turnStartPacket);
	turnStartPacket.header.type = PACKET_TYPE_TURN_START;

	roomInfoIter->second->SetGameState(GAMESTATE_GAME);

	g_mutex.lock();
	for (auto iter = roomInfoIter->second->GetRoomUsers()->begin(); iter != roomInfoIter->second->GetRoomUsers()->end(); iter++)
	{
		send(iter->first, (char*)&turnStartPacket, turnStartPacket.header.packetLen, 0);
	}
	g_mutex.unlock();
}

void err_quit(char *msg)//에러 출력 후 종료 //심각한 오류 발생시
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

void err_display(char *msg)//에러출력
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void Server::GetWords()
{
	FILE* file;
	file = fopen("words.txt", "r");

	char buf[24];
	int retval = 0;
	while (retval != EOF)
	{
		retval = fscanf(file, "%s\n", buf);
		m_words.push_back(buf);
	}
	
	fclose(file);
}

Server::Server()
{
	
}


Server::~Server()
{
}
