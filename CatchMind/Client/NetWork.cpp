#include "NetWork.h"
#include "../../../packetHeader.h"
#include <WinSock2.h>
#include <stdio.h>
#include <process.h>

void err_quit(char *msg);
void err_display(char *msg);

NetWork* NetWork::S = NULL;

bool NetWork::StartNetWork(int port)
{
	int retval;
	SOCKET client_sock;
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	client_sock = socket(AF_INET, SOCK_STREAM, 0);//접속용 소켓
	if (client_sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	retval = connect(client_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("connect()");
		return false;
	}

	sock = client_sock;

	return true;
}

void NetWork::SendPacket(char* packet, int size)
{
	int retval;

	retval = send(sock, packet, size, 0);
	if (retval == SOCKET_ERROR)
	{
		err_display("send()");
		return;
	}

	return;
}

int NetWork::GetSock()
{
	return sock;
}

void NetWork::Release()
{
	closesocket(sock);

	WSACleanup();
}

NetWork* NetWork::GetInstance()
{
	if (S == NULL)
	{
		S = new NetWork;
	}

	return S;
}

void NetWork::Destroy()
{
	if (S != NULL)
	{
		delete S;
		S = NULL;
	}
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
	printf("[%s] %s", msg, lpMsgBuf);
	LocalFree(lpMsgBuf);
}

NetWork::NetWork()
{
}


NetWork::~NetWork()
{
}
