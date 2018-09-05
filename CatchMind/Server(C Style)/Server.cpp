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

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);//���� ���� ����
	if (listen_sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	SOCKADDR_IN serveraddr;//�ڱ� ���� ����(���ε�)
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("bind()");
	}

	retval = listen(listen_sock, SOMAXCONN);//�������·� �ٲ���
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

		printf("\n[TCP ����] Ŭ���̾�Ʈ ���� : IP �ּ� = %s, ��Ʈ��ȣ = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		HANDLE hResult = CreateIoCompletionPort((HANDLE)client_sock, hcp, (DWORD)client_sock, 0);

		if (hResult == NULL)
		{
			return -1;
		}

		SOCKETINFO *ptr = new SOCKETINFO;
		if (ptr == NULL)
		{
			printf("[����] �޸𸮰� �����մϴ�!\n");
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

		if (retval == 0 || cbTransferred == 0)//����޼��� ����
		{
			if (retval == 0)
			{
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &(ptr->overlapped), &temp1, FALSE, &temp2);
				err_display("WSAGetOverlappedResult()");
			}
	
			ptr->pServer->EreaseUser(ptr->sock);


			printf("[TCP ����] Ŭ���̾�Ʈ ���� : IP �ּ� = %s, ��Ʈ ��ȣ = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
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

		ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));//�ٽ�recv�ɾ���
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
		for (auto iter = room->second->m_mapRoomUser.begin(); iter != room->second->m_mapRoomUser.end(); iter++)
		{
			retval = send(iter->first, (char*)&TimeOverPacket, TimeOverPacket.header.packetLen, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				EreaseUser(iter->first);
				return;
			}
		}
		g_mutex.unlock();
		room->second->gameState = GAMESTATE_STAND;
		Sleep(2000);

		SendRoundPacket(pInfo);
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
		for (auto iter = room->second->m_mapRoomUser.begin(); iter != room->second->m_mapRoomUser.end(); iter++)
		{
			retval = send(iter->first, (char*)&gameChatPacket, gameChatPacket.header.packetLen, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display("send()");
				EreaseUser(iter->first);
				return;
			}
		}
		g_mutex.unlock();

		if (room->second->answer == gameChatPacket.buf && room->second->gameState == GAMESTATE_GAME)
		{
			PACKET_CORRECT correctPacket;
			correctPacket.header.packetLen = sizeof(correctPacket);
			correctPacket.header.type = PACKET_TYPE_CORRECT;
			strcpy(correctPacket.answererName, pInfo->name.c_str());
			room->second->gameState = GAMESTATE_CORRECT;

			g_mutex.lock();
			for (auto iter = room->second->m_mapRoomUser.begin(); iter != room->second->m_mapRoomUser.end(); iter++)
			{
				send(iter->first, (char*)&correctPacket, correctPacket.header.packetLen, 0);
			}
			g_mutex.unlock();

			Sleep(2000);
			auto user = room->second->m_mapRoomUser.find(pInfo->sock);
			user->second.score++;
			user = room->second->m_mapRoomUser.find(room->second->turnUserSock);
			user->second.score++;
			room->second->gameState = GAMESTATE_STAND;

			SendRoundPacket(pInfo);
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
		for (auto iter = room->second->m_mapRoomUser.begin(); iter != room->second->m_mapRoomUser.end(); iter++)
		{
			retval = send(iter->first, (char*)&linePacket, linePacket.header.packetLen, 0);
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

	case PACKET_TYPE_ERASE:
	{
		PACKET_ERASE erasePacket;
		memcpy(&erasePacket, pInfo->bufForByteStream, sizeof(erasePacket));
		int retval;

		g_mutex.lock();
		auto room = m_mapRoomInfo.find(pInfo->roomNum);
		for (auto iter = room->second->m_mapRoomUser.begin(); iter != room->second->m_mapRoomUser.end(); iter++)
		{
			retval = send(iter->first, (char*)&erasePacket, erasePacket.header.packetLen, 0);
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

	case PACKET_TYPE_LOGIN:
	{
		PACKET_LOGIN loginPacket;
		g_mutex.lock();
		memcpy(&loginPacket, pInfo->bufForByteStream, sizeof(loginPacket));
		for (auto iter = m_mapSockInfo.begin(); iter != m_mapSockInfo.end(); iter++)
		{
			if (iter->second->name == loginPacket.name)
			{
				int namelen = strlen(loginPacket.name);
				strcat(&loginPacket.name[namelen], "1");
				break;
			}
		}
		g_mutex.unlock();
		printf("%s�� �κ�� ����\n", loginPacket.name);
		pInfo->name = loginPacket.name;

		USERINFO* newUser = new USERINFO;
		strcpy(newUser->name, loginPacket.name);
		newUser->socket = pInfo->sock;
		m_mapRobbyUser.insert(std::make_pair(pInfo->sock, newUser));
		
		int retval;

		PACKET_LOGIN_RESULT loginResultPacket;
		loginResultPacket.header.type = PACKET_TYPE_LOGIN_RESULT;
		loginResultPacket.header.packetLen = sizeof(PACKET_LOGIN_RESULT);
		strcpy(loginResultPacket.name, loginPacket.name);

		Sleep(10);
	
		retval = send(pInfo->sock, (char*)&loginResultPacket, loginResultPacket.header.packetLen, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("send()");
			EreaseUser(pInfo->sock);
			return;
		}

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
		printf("%s ���� �������\n", makeRoomPacket.roomName);
		m_iRoomNum++;

		ROOMINFO* newRoom = new ROOMINFO;
		newRoom->iUserCount = 1;
		newRoom->roomNum = m_iRoomNum;
		newRoom->roomMaster = pInfo->sock;
		newRoom->gameState = GAMESTATE_READY;
		newRoom->roomName = makeRoomPacket.roomName;
		newRoom->round = 0;

		ROOMUSERINFO roomUserInfo;
		roomUserInfo.score = 0;
		roomUserInfo.turn = false;
		roomUserInfo.master = true;
		roomUserInfo.user = m_mapRobbyUser.find(pInfo->sock)->second;


		m_mapRobbyUser.erase(pInfo->sock);
		printf("%s���� �κ񿡼� ����\n", pInfo->name);
		SendRobbyList();

		pInfo->roomNum = newRoom->roomNum;
		newRoom->m_mapRoomUser.insert(make_pair(pInfo->sock, roomUserInfo));
		g_mutex.lock();
		m_mapRoomInfo.insert(make_pair(newRoom->roomNum, newRoom));
		g_mutex.unlock();
		
		PACKET_CHANGE_ROOM pcr;
		pcr.header.packetLen = sizeof(pcr);
		pcr.header.type = PACKET_TYPE_CHANGE_ROOM;

		Sleep(10);
		send(pInfo->sock, (char*)&pcr, pcr.header.packetLen, 0);

		PACKET_LOGIN_RESULT loginResultPacket;
		loginResultPacket.header.type = PACKET_TYPE_LOGIN_RESULT;
		loginResultPacket.header.packetLen = sizeof(PACKET_LOGIN_RESULT);
		strcpy(loginResultPacket.name, pInfo->name.c_str());

		Sleep(10);
		send(pInfo->sock, (char*)&loginResultPacket, loginResultPacket.header.packetLen, 0);


		Sleep(50);

		SendRoomUserInfo(newRoom->roomNum);

		Sleep(10);// ���ķ� �� ���� ����

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
			auto roomIter = m_mapRoomInfo.find(lastRoomNum);
			g_mutex.lock();
			m_mapRobbyUser.insert(make_pair(pInfo->sock, roomIter->second->m_mapRoomUser.find(pInfo->sock)->second.user));
			
			bool isTurn = roomIter->second->m_mapRoomUser.find(pInfo->sock)->second.turn;

			auto nextUser = roomIter->second->m_mapRoomUser.find(pInfo->sock);
			nextUser++;
			if (nextUser == roomIter->second->m_mapRoomUser.end())
			{
				nextUser = roomIter->second->m_mapRoomUser.begin();
			}
			SOCKETINFO* sockInfo = m_mapSockInfo.find(nextUser->first)->second;

			roomIter->second->m_mapRoomUser.erase(pInfo->sock);
			roomIter->second->iUserCount--;
			g_mutex.unlock();

			if (roomIter->second->iUserCount == 1)//���� ������¸� ������
			{
				PACKET_READY_GAME readyGamePacket;
				roomIter->second->gameState = GAMESTATE_READY;
				readyGamePacket.header.type = PACKET_TYPE_READY_GAME;
				readyGamePacket.header.packetLen = sizeof(readyGamePacket);

				g_mutex.lock();
				for (auto iter = roomIter->second->m_mapRoomUser.begin(); iter != roomIter->second->m_mapRoomUser.end(); iter++)
				{
					iter->second.score = 0;
					roomIter->second->round = 0;
					iter->second.turn = false;
					send(iter->first, (char*)&readyGamePacket, readyGamePacket.header.packetLen, 0);
				}
				g_mutex.unlock();
			}
			else if(roomIter->second->iUserCount != 0)
			{
				if (isTurn)
				{
					roomIter->second->round--;
					SendRoundPacket(sockInfo);
				}
			}

			if (roomIter->second->iUserCount == 0)
			{
				g_mutex.lock();
				delete m_mapRoomInfo.find(lastRoomNum)->second;
				m_mapRoomInfo.erase(lastRoomNum);
				g_mutex.unlock();
			}
			else
			{
				if (pInfo->sock == roomIter->second->roomMaster)
				{
					roomIter->second->roomMaster = roomIter->second->m_mapRoomUser.begin()->second.user->socket;
					roomIter->second->m_mapRoomUser.begin()->second.master = true;
				}
				Sleep(50);
				SendRoomUserInfo(lastRoomNum);
			}

			PACKET_LOGIN_RESULT loginResultPacket;
			loginResultPacket.header.type = PACKET_TYPE_LOGIN_RESULT;
			loginResultPacket.header.packetLen = sizeof(PACKET_LOGIN_RESULT);
			strcpy(loginResultPacket.name, pInfo->name.c_str());

			Sleep(10);
			send(pInfo->sock, (char*)&loginResultPacket, loginResultPacket.header.packetLen, 0);
		}
		else
		{
			ROOMUSERINFO roomUserInfo;
			roomUserInfo.score = 0;
			roomUserInfo.turn = false;
			roomUserInfo.master = false;
			roomUserInfo.user = m_mapRobbyUser.find(pInfo->sock)->second;
			
			g_mutex.lock();
			m_mapRobbyUser.erase(pInfo->sock);
			pInfo->roomNum = wantGoRoomPacket.roomNum;
			m_mapRoomInfo.find(pInfo->roomNum)->second->m_mapRoomUser.insert(make_pair(pInfo->sock, roomUserInfo));
			m_mapRoomInfo.find(pInfo->roomNum)->second->iUserCount++;
			g_mutex.unlock();

			PACKET_CHANGE_ROOM pcr;
			pcr.header.packetLen = sizeof(pcr);
			pcr.header.type = PACKET_TYPE_CHANGE_ROOM;
			Sleep(10);
			send(pInfo->sock, (char*)&pcr, pcr.header.packetLen, 0);

			PACKET_LOGIN_RESULT loginResultPacket;
			loginResultPacket.header.type = PACKET_TYPE_LOGIN_RESULT;
			loginResultPacket.header.packetLen = sizeof(PACKET_LOGIN_RESULT);
			strcpy(loginResultPacket.name, pInfo->name.c_str());

			Sleep(10);
			send(pInfo->sock, (char*)&loginResultPacket, loginResultPacket.header.packetLen, 0);
			Sleep(50);
			SendRoomUserInfo(wantGoRoomPacket.roomNum);
		}

		Sleep(10);
		SendRobbyList();

		Sleep(10);// ���ķ� �� ���� ����
		SendRoomList();
	}
	break;

	case PACKET_TYPE_GAME_START:
	{
		PACKET_GAME_START gameStartPacket;

		memcpy(&gameStartPacket, pInfo->bufForByteStream, sizeof(gameStartPacket));

		g_mutex.lock();
		auto roomInfoIter = m_mapRoomInfo.find(pInfo->roomNum);
		if (roomInfoIter->second->gameState != GAMESTATE_READY || roomInfoIter->second->iUserCount < 2)
		{
			break;
		}
		g_mutex.unlock();
		roomInfoIter->second->gameState = GAMESTATE_GAME;
		SendRoomList();
		SendRoundPacket(pInfo);
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
			delete(robbyIter->second);
			m_mapRobbyUser.erase(robbyIter);
			g_mutex.unlock();
			SendRobbyList();
		}
	}
	else
	{
		auto roomIter = m_mapRoomInfo.find(sockInfoIter->second->roomNum);
		roomIter->second->iUserCount--;
	
		bool isTurn = roomIter->second->m_mapRoomUser.find(sock)->second.turn;
		auto nextUser = roomIter->second->m_mapRoomUser.find(sock);
		nextUser++;
		if (nextUser == roomIter->second->m_mapRoomUser.end())
		{
			nextUser = roomIter->second->m_mapRoomUser.begin();
		}
		SOCKETINFO* sockInfo = m_mapSockInfo.find(nextUser->first)->second;

		g_mutex.lock();
		roomIter->second->m_mapRoomUser.erase(sock);
		g_mutex.unlock();
		SendRoomUserInfo(sockInfoIter->second->roomNum);

		if (roomIter->second->iUserCount == 1)//���� ������¸� ������
		{
			PACKET_READY_GAME readyGamePacket;
			roomIter->second->round = 0;
			roomIter->second->gameState = GAMESTATE_READY;
			readyGamePacket.header.type = PACKET_TYPE_READY_GAME;
			readyGamePacket.header.packetLen = sizeof(readyGamePacket);

			for (auto iter = roomIter->second->m_mapRoomUser.begin(); iter != roomIter->second->m_mapRoomUser.end(); iter++)
			{
				iter->second.score = 0;
				iter->second.turn = false;
				send(iter->first, (char*)&readyGamePacket, readyGamePacket.header.packetLen, 0);
			}
		}
		else if (roomIter->second->iUserCount != 0)
		{
			if (isTurn)
			{
				roomIter->second->round--;
				SendRoundPacket(sockInfo);
			}
		}

		if (roomIter->second->iUserCount == 0)//��� ����
		{
			g_mutex.lock();
			delete(m_mapRoomInfo.find(sockInfoIter->second->roomNum)->second);
			m_mapRoomInfo.erase(sockInfoIter->second->roomNum);
			g_mutex.unlock();
		}
		else
		{
			if (sock == roomIter->second->roomMaster)
			{
				roomIter->second->roomMaster = roomIter->second->m_mapRoomUser.begin()->second.user->socket;
				roomIter->second->m_mapRoomUser.begin()->second.master = true;
			}
			Sleep(10);
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
	userListPacket.header.packetLen = sizeof(PACKET_HEADER) + sizeof(int) + (sizeof(USERINFO) * m_mapRobbyUser.size());
	userListPacket.iUserCount = m_mapRobbyUser.size();
	int i = 0;
	g_mutex.lock();
	for (auto iter = m_mapRobbyUser.begin(); iter != m_mapRobbyUser.end(); iter++)
	{
		strcpy(userListPacket.userList[i], iter->second->name);
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
		if (iter->second->gameState != GAMESTATE_READY)
		{
			packetRoomList.header.packetLen -= sizeof(ROOMINFOFORPACKET);
			packetRoomList.roomCount--;
			continue;
		}

		strcpy(packetRoomList.room[i].roomName, iter->second->roomName.c_str());
		packetRoomList.room[i].roomNum = iter->first;
		packetRoomList.room[i].userNum = iter->second->iUserCount;
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
	int x = sizeof(PACKET_HEADER) + sizeof(int) + (sizeof(GAMEUSERINFO) * iter->second->m_mapRoomUser.size());
	packetRoomUserInfoList.header.packetLen = x;
	packetRoomUserInfoList.userNum = iter->second->m_mapRoomUser.size();
	g_mutex.lock();
	int i = 0;
	for (auto iter2 = iter->second->m_mapRoomUser.begin(); iter2 != iter->second->m_mapRoomUser.end(); iter2++)
	{
		strcpy(packetRoomUserInfoList.user[i].name, iter2->second.user->name);
		packetRoomUserInfoList.user[i].master = iter2->second.master;
		packetRoomUserInfoList.user[i].score = iter2->second.score;
		packetRoomUserInfoList.user[i].turn = iter2->second.turn;
		packetRoomUserInfoList.gameState = iter->second->gameState;
		i++;
	}

	for (auto iter2 = iter->second->m_mapRoomUser.begin(); iter2 != iter->second->m_mapRoomUser.end(); iter2++)
	{
		send(iter2->first, (char*)&packetRoomUserInfoList, packetRoomUserInfoList.header.packetLen, 0);
	}
	g_mutex.unlock();
}

void Server::SendRoundPacket(SOCKETINFO* pInfo)
{
	auto roomInfoIter = m_mapRoomInfo.find(pInfo->roomNum);

	if (roomInfoIter->second->round == 4)
	{
		PACKET_END_GAME endGamePacket;
		roomInfoIter->second->gameState = GAMESTATE_RECORD;
		roomInfoIter->second->round = 0;
		endGamePacket.header.type = PACKET_TYPE_END_GAME;
		endGamePacket.header.packetLen = sizeof(endGamePacket);
		g_mutex.lock();
		for (auto iter = roomInfoIter->second->m_mapRoomUser.begin(); iter != roomInfoIter->second->m_mapRoomUser.end(); iter++)
		{
			send(iter->first, (char*)&endGamePacket, endGamePacket.header.packetLen, 0);
		}

		Sleep(2000);

		PACKET_READY_GAME readyGamePacket;
		roomInfoIter->second->gameState = GAMESTATE_READY;
		readyGamePacket.header.type = PACKET_TYPE_READY_GAME;
		readyGamePacket.header.packetLen = sizeof(readyGamePacket);

		for (auto iter = roomInfoIter->second->m_mapRoomUser.begin(); iter != roomInfoIter->second->m_mapRoomUser.end(); iter++)
		{
			iter->second.score = 0;
			iter->second.turn = false;
			send(iter->first, (char*)&readyGamePacket, readyGamePacket.header.packetLen, 0);
		}
		g_mutex.unlock();

		Sleep(10);
		SendRoomUserInfo(pInfo->roomNum);

		return;
	}

	PACKET_TURN_INFO turnInfoPacket;
	turnInfoPacket.header.type = PACKET_TYPE_TURN_INFO;
	turnInfoPacket.header.packetLen = sizeof(turnInfoPacket);

	roomInfoIter->second->round++;

	turnInfoPacket.round = roomInfoIter->second->round;

	int answerIndex = rand() % m_words.size();

	strcpy(turnInfoPacket.answer, m_words[answerIndex].c_str());
	roomInfoIter->second->answer = turnInfoPacket.answer;
	roomInfoIter->second->gameState = GAMESTATE_STAND;
	
	g_mutex.lock();
	for (auto iter = roomInfoIter->second->m_mapRoomUser.begin(); iter != roomInfoIter->second->m_mapRoomUser.end(); iter++)
	{
		iter->second.turn = false;
	}
	g_mutex.unlock();

	if(roomInfoIter->second->round == 1)
	{
		roomInfoIter->second->turnUserSock = pInfo->sock;
		strcpy(turnInfoPacket.curTurnUserName, pInfo->name.c_str());
		auto userIterForTurn = roomInfoIter->second->m_mapRoomUser.find(pInfo->sock);
		userIterForTurn->second.turn = true;
	}
	else
	{
		auto userIterForTurn = roomInfoIter->second->m_mapRoomUser.find(roomInfoIter->second->turnUserSock);
		if (userIterForTurn == roomInfoIter->second->m_mapRoomUser.end())
		{
			userIterForTurn = roomInfoIter->second->m_mapRoomUser.find(pInfo->sock);
		}
		else
		{
			userIterForTurn++;
			if (userIterForTurn == roomInfoIter->second->m_mapRoomUser.end())
			{
				userIterForTurn = roomInfoIter->second->m_mapRoomUser.begin();
			}
		}
		
		roomInfoIter->second->turnUserSock = userIterForTurn->second.user->socket;
		strcpy(turnInfoPacket.curTurnUserName, userIterForTurn->second.user->name);
		userIterForTurn->second.turn = true;
	}

	g_mutex.lock();
	auto userIter = roomInfoIter->second->m_mapRoomUser.find(roomInfoIter->second->turnUserSock);
	userIter++;
	if (userIter == roomInfoIter->second->m_mapRoomUser.end())
	{
		userIter = roomInfoIter->second->m_mapRoomUser.begin();
	}
	strcpy(turnInfoPacket.nextTurnUserName, userIter->second.user->name);
	g_mutex.unlock();
	SendRoomUserInfo(pInfo->roomNum);
	
	Sleep(10);

	for (auto iter = roomInfoIter->second->m_mapRoomUser.begin(); iter != roomInfoIter->second->m_mapRoomUser.end(); iter++)
	{
		send(iter->first, (char*)&turnInfoPacket, turnInfoPacket.header.packetLen, 0);
	}

	Sleep(2000);
	PACKET_TURN_START turnStartPacket;
	turnStartPacket.header.packetLen = sizeof(turnStartPacket);
	turnStartPacket.header.type = PACKET_TYPE_TURN_START;

	roomInfoIter->second->gameState = GAMESTATE_GAME;

	g_mutex.lock();
	for (auto iter = roomInfoIter->second->m_mapRoomUser.begin(); iter != roomInfoIter->second->m_mapRoomUser.end(); iter++)
	{
		send(iter->first, (char*)&turnStartPacket, turnStartPacket.header.packetLen, 0);
	}
	g_mutex.unlock();
}

void err_quit(char *msg)//���� ��� �� ���� //�ɰ��� ���� �߻���
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

void err_display(char *msg)//�������
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