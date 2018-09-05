#pragma once
#include "../../HCHEngine/Scene.h"
#include "../../HCHEngine/BitMap.h"
#include "../../HCHEngine/Button.h"

struct ROOM_BUTTON
{
	Button button;
	string roomName;
	int roomNum;
	int userNum;
};


struct THREAD_ROBBY_ARG
{
	bool* bSceneChange;
	SOCKET sock;
	HWND chatHWnd;
	vector<string>* pVecUser;
	string* pUserName;
	ROOM_BUTTON* pRoomButton;
};

class RobbyScene :
	public Scene
{
	bool m_bIsSceneChange;

	THREAD_ROBBY_ARG m_threadArg;

	string m_strUserName;
	vector<string> m_vecUser;
	ROOM_BUTTON m_roomButton[5];

	Button m_makeRoomButton;
	BitMap* m_backBitMap;

	BitMap* m_makeRoomWindowBitMap;
	Button m_makeRoomOkButton;
	Button m_makeRoomCancelButton;

	HWND m_hChatBox;
	HWND m_hEditBox;
	HWND m_hUserListBox;
	HWND m_hRoomNameEditBox;
	unsigned int m_hThread;

	int MakeRoomFunc();
	int MakeRoomOkButtonFunc();
	int MakeRoomCancelButtonFunc();
	int RoomButtonFunc(int* roomNum);

public:
	virtual void Init();
	virtual void InputOperator(float fElapsedTime);
	virtual void Update(float fElapsedTime);
	virtual bool Draw();
	virtual void Release();

	RobbyScene();
	virtual ~RobbyScene();
};

