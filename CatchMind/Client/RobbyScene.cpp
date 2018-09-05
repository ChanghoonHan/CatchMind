#include "RobbyScene.h"
#include "../../HCHEngine/DrawManager.h"
#include "../../../packetHeader.h"
#include "NetWork.h"
#include <process.h>

void err_quit(char *msg);
void err_display(char *msg);
unsigned int WINAPI RecvThread(LPVOID arg);
void roomButtonInit(ROOM_BUTTON* roomButton);
vector<string> g_threadVecStr;
string g_threadStrChatBoxBuf = "";

int RobbyScene::MakeRoomOkButtonFunc()
{
	int len;
	char buf[BUFSIZE] = "";

	GetWindowText(m_hRoomNameEditBox, buf, BUFSIZE);

	if (strlen(buf) != 0)
	{
		len = strlen(buf);
		if (buf[len - 1] == '\n')
		{
			buf[len - 1] = '\0';
		}

		PACKET_MAKE_ROOM pmr;
		pmr.header.type = PACKET_TYPE_MAKE_ROOM;
		pmr.header.packetLen = sizeof(PACKET_HEADER) + len + 1;
		strcpy(pmr.roomName, buf);

		NetWork::GetInstance()->SendPacket((char*)&pmr, pmr.header.packetLen);
	}
	
	DestroyWindow(m_hRoomNameEditBox);
	m_hRoomNameEditBox = 0;

	return 0;
}

int RobbyScene::MakeRoomCancelButtonFunc()
{
	DestroyWindow(m_hRoomNameEditBox);

	m_hRoomNameEditBox = 0;

	return 0;
}

int RobbyScene::MakeRoomFunc()
{
	m_hRoomNameEditBox = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		310, 274, 190, 20, DrawManager::GetInstnace()->GetHWnd(), NULL, NULL, NULL);

	return 0;
}

int RobbyScene::RoomButtonFunc(int* roomNum)
{
	PACKET_WANT_GO_ROOM pwgr;
	pwgr.header.type = PACKET_TYPE_WANT_GO_ROOM;
	pwgr.header.packetLen = sizeof(PACKET_HEADER) + sizeof(int);
	pwgr.roomNum = *roomNum;
	
	if (*roomNum != 0)
	{
		NetWork::GetInstance()->SendPacket((char*)&pwgr, pwgr.header.packetLen);
	}
	
	return 0;
}

void RobbyScene::Init()
{
	m_bIsSceneChange = false;

	m_hEditBox = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		39, 515, 443, 25, DrawManager::GetInstnace()->GetHWnd(), NULL, NULL, NULL);
	m_hChatBox = CreateWindow("static", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		39, 395, 443, 120, DrawManager::GetInstnace()->GetHWnd(), NULL, NULL, NULL);
	m_hUserListBox = CreateWindow("static", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		525, 89, 202, 243, DrawManager::GetInstnace()->GetHWnd(), NULL, NULL, NULL);

	m_hRoomNameEditBox = 0;
	m_backBitMap = DrawManager::GetInstnace()->GetBitMap("MainBack1.bmp");
	m_makeRoomWindowBitMap = DrawManager::GetInstnace()->GetBitMap("MakeRoomWindow.bmp");
	m_makeRoomOkButton.Init("MakeRoomOkButton1.bmp", "MakeRoomOkButton2.bmp", 284, 305, 55, 19, bind(&RobbyScene::MakeRoomOkButtonFunc, this));
	m_makeRoomCancelButton.Init("MakeRoomCancelButton1.bmp", "MakeRoomCancelButton2.bmp", 429, 305, 55, 19, bind(&RobbyScene::MakeRoomCancelButtonFunc, this));
	m_makeRoomButton.Init("MakeRoomButton1.bmp", "MakeRoomButton2.bmp", 233, 21, 64, 27, bind(&RobbyScene::MakeRoomFunc, this));

	for (size_t i = 0; i < 5; i++)
	{
		m_roomButton[i].roomNum = 0;
		m_roomButton[i].roomName = "";
		m_roomButton[i].userNum = 0;
		m_roomButton[i].button.Init("RoomButton1.bmp", "RoomButton2.bmp", 40, 90 + (i*53), 441, 53, bind(&RobbyScene::RoomButtonFunc, this, &m_roomButton[i].roomNum));
	}

	OperatorManager::GetInstance()->KeyClear();
	OperatorManager::GetInstance()->RegisterKey(13);
	OperatorManager::GetInstance()->RegisterKey(VK_LBUTTON);
	m_strUserName = "";
	m_vecUser.clear();

	m_threadArg.bSceneChange = &m_bIsSceneChange;
	m_threadArg.chatHWnd = m_hChatBox;
	m_threadArg.sock = NetWork::GetInstance()->GetSock();
	m_threadArg.pVecUser = &m_vecUser;
	m_threadArg.pUserName = &m_strUserName;
	m_threadArg.pRoomButton = m_roomButton;
	m_hThread = _beginthreadex(NULL, 0, RecvThread, (LPVOID)&m_threadArg, 0, 0);
}

void RobbyScene::InputOperator(float fElapsedTime)
{
	if (OperatorManager::GetInstance()->IsKeyDown(13) && m_strUserName != "")
	{
		int len;
		char buf[BUFSIZE];

		GetWindowText(m_hEditBox, buf, BUFSIZE);
		if (strlen(buf) == 0)
		{
			return;
		}

		SetWindowText(m_hEditBox, "");

		len = strlen(buf);
		if (buf[len - 1] == '\n')
		{
			buf[len - 1] = '\0';
		}
		PACKET_CHAT pc;
		pc.header.type = PACKET_TYPE_CHAT;
		pc.header.packetLen = sizeof(PACKET_HEADER) + 24 +len + 1;
		strcpy(pc.name, m_strUserName.c_str());
		strcpy(pc.buf, buf);

		NetWork::GetInstance()->SendPacket((char*)&pc, pc.header.packetLen);
	}
}

void RobbyScene::Update(float fElapsedTime)
{
	if (m_bIsSceneChange)
	{
		GameManager::GetInstance()->LoadScene(2);
		return;
	}

	if (m_hRoomNameEditBox == 0)
	{
		for (size_t i = 0; i < 5; i++)
		{
			m_roomButton[i].button.Update();
		}
		m_makeRoomButton.Update();
	}
	else
	{
		m_makeRoomOkButton.Update();
		m_makeRoomCancelButton.Update();
	}
}

bool RobbyScene::Draw()
{
	m_backBitMap->Draw(0, 0);

	string userList = "";

	for (auto iter = m_vecUser.begin(); iter != m_vecUser.end(); iter++)
	{
		if (iter != m_vecUser.begin())
		{
			userList += "\n";
		}
		userList += *iter;
	}
	SetWindowText(m_hUserListBox, userList.c_str());

	DrawManager::GetInstnace()->PutText(m_strUserName, 640, 406, 700, 426, DT_CENTER);
	m_makeRoomButton.Draw();
	
	for (size_t i = 0; i < 5; i++)
	{
		char roomInfo[128] = "";
		m_roomButton[i].button.Draw();
		if (m_roomButton[i].roomNum == 0)
		{
			strcpy(roomInfo, "★★★★★★★★★★★★빈방★★★★★★★★★★★★");
		}
		else
		{
			sprintf(roomInfo, "%d 번방   ||   %s   ||   인원 %d / 8", m_roomButton[i].roomNum, m_roomButton[i].roomName.c_str(), m_roomButton[i].userNum);
		}

		DrawManager::GetInstnace()->PutText(roomInfo, 53, 107 + (i * 53), 40 + 441, 90 + (i * 53) + 53, DT_LEFT);
	}

	if (m_hRoomNameEditBox != 0)
	{
		m_makeRoomWindowBitMap->Draw(249, 241);
		m_makeRoomOkButton.Draw();
		m_makeRoomCancelButton.Draw();
	}

	return true;
}

unsigned int WINAPI RecvThread(LPVOID arg)
{
	THREAD_ROBBY_ARG* Targ = (THREAD_ROBBY_ARG*)arg;

	g_threadVecStr.clear();
	SOCKET sock = Targ->sock;
	char buf[2048];
	char tepmBuf[2048];
	int retval = 0;
	int recvlen = 0;
	int len = 0;

	while (true)
	{
		retval = recv(sock, &buf[recvlen], 2048, 0);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			break;
		}
		else if (retval == 0)
		{
			break;
		}

		if (retval < sizeof(PACKET_HEADER))
		{
			recvlen = retval;
			continue;
		}

		PACKET_HEADER* ph = (PACKET_HEADER*)buf;
		if (retval < ph->packetLen)
		{
			recvlen = retval;
			continue;
		}

		recvlen = 0;

		switch (ph->type)
		{
		case PACKET_TYPE_CHAT:
		{
			PACKET_CHAT* chatPacket = (PACKET_CHAT*)buf;
			char chatBuf[512];
			chatPacket->buf[40] = '\0';
			sprintf(chatBuf, "%s : %s", chatPacket->name, chatPacket->buf);

			if (g_threadVecStr.size() == 7)
			{
				g_threadVecStr.erase(g_threadVecStr.begin());
			}
			g_threadVecStr.push_back(chatBuf);

			g_threadStrChatBoxBuf = "";
			for (auto iter = g_threadVecStr.begin(); iter != g_threadVecStr.end(); iter++)
			{
				if (iter != g_threadVecStr.begin())
				{
					g_threadStrChatBoxBuf += "\n";
				}
				g_threadStrChatBoxBuf += *iter;
			}

			SetWindowText(Targ->chatHWnd, g_threadStrChatBoxBuf.c_str());
		}
			break;

		case PACKET_TYPE_LOGIN_RESULT:
		{
			PACKET_LOGIN_RESULT* plr = (PACKET_LOGIN_RESULT*)buf;
			*Targ->pUserName = plr->name;
		}
			break;

		case PACKET_TYPE_USER_LIST:
		{
			PACKET_USER_LIST* pul = (PACKET_USER_LIST*)buf;
			Targ->pVecUser->clear();
			for (size_t i = 0; i < pul->iUserCount; i++)
			{
				Targ->pVecUser->push_back(pul->userList[i]);
			}
		}
			break;

		case PACKET_TYPE_CHANGE_ROOM:
		{
			*Targ->bSceneChange = true;
		}
			break;

		case PACKET_TYPE_ROOM_LIST:
		{
			PACKET_ROOM_LIST* prl = (PACKET_ROOM_LIST*)buf;
			roomButtonInit(Targ->pRoomButton);
			for (size_t i = 0; i < prl->roomCount; i++)
			{
				Targ->pRoomButton[i].roomName = prl->room[i].roomName;
				Targ->pRoomButton[i].roomNum = prl->room[i].roomNum;
				Targ->pRoomButton[i].userNum = prl->room[i].userNum;
			}
		}
			break;

		default:
			break;
		}
	}

	return 0;
}

void roomButtonInit(ROOM_BUTTON* roomButton)
{
	for (size_t i = 0; i < 5; i++)
	{
		roomButton[i].roomName = "";
		roomButton[i].roomNum = 0;
		roomButton[i].userNum = 0;
	}
}

void RobbyScene::Release()
{
	DestroyWindow(m_hChatBox);
	DestroyWindow(m_hEditBox);
	DestroyWindow(m_hUserListBox);

	if (m_hRoomNameEditBox != 0)
	{
		DestroyWindow(m_hRoomNameEditBox);
	}

	TerminateThread((HANDLE)m_hThread, 0);
}

RobbyScene::RobbyScene()
{
}


RobbyScene::~RobbyScene()
{
	
}