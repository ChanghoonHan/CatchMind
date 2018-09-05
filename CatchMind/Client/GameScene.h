#pragma once
#include "../../HCHEngine/Scene.h"
#include "../../HCHEngine/BitMap.h"
#include "../../HCHEngine/Button.h"
#include "RoomUser.h"
#include "../../../packetHeader.h"

#define BLACKPEN RGB(0, 0, 0)
#define REDPEN RGB(255, 0, 0)
#define GREENPEN RGB(0, 255, 0)
#define BLUEPEN RGB(0, 0, 255)
#define YELLOWPEN RGB(255, 255, 0)
#define WHITEPEN RGB(255, 255, 255)

using namespace std;

struct RECORD_DATA
{
	int score;
	string name;
};

struct THREAD_GAME_ARG
{
	bool* bSceneChange;
	bool* bTrun;
	bool* bIsMaster;
	CMGAMESTATE* pGameState;
	SOCKET sock;
	RoomUser** pUsers;
	vector<LINEINFO>* lines;
	vector<LINEINFO>* myLines;
	string* name;
	string* userNameTemp;
	string* answer;
	float* time;
	int* round;
};

class GameScene :
	public Scene
{
	BitMap* m_pBackBitMap;
	BitMap* m_pNoPalatte;
	BitMap* m_pRoundWindow;
	BitMap* m_pCorrectWindow;
	BitMap* m_pRecordsWindow;
	BitMap* m_pTimeOverWindow;

	Button m_exitButton;
	Button m_gameStartButton;

	Button m_allClearButton;
	Button m_eraseButton;
	Button m_blackPenButton;
	Button m_redPenButton;
	Button m_greenPenButton;
	Button m_bluePenButton;
	Button m_yellowPenButton;
	Button m_whitePenButton;

	string m_userNameTemp[2];
	string m_answer;
	int m_round;

	string m_myName;
	RoomUser* m_myUserInfo;

	COLORREF m_curColor;

	vector<LINEINFO> m_mylines;
	vector<LINEINFO> m_lines;

	Rect m_rectPaper;
	HWND m_hChatEditBox;

	CMGAMESTATE m_gameState;

	bool m_bTurn;
	bool m_bMaster;

	bool m_bIsLeftDrawing;
	bool m_bIsRightDrawing;
	int m_hThread;

	float m_fTime;

	THREAD_GAME_ARG m_threadArg;

	RoomUser* m_Users[8];

	bool m_bIsSceneChange;

	void SendLine(LINEINFO& line);

	void GetRecored(vector<RECORD_DATA>& vecRecord);

	int ExitButtonFunc();
	int AllClearButtonFunc();
	int EraserButtonFunc();
	int BlackPenButtonFunc();
	int RedPenButtonFunc();
	int GreenPenButtonFunc();
	int BluePenButtonFunc();
	int YellowPenButtonFunc();
	int WhitePenButtonFunc();
	int GameStartButtonFunc();
public:
	virtual void Init();
	virtual void InputOperator(float fElapsedTime);
	virtual void Update(float fElapsedTime);
	virtual bool Draw();
	virtual void Release();

	GameScene();
	virtual ~GameScene();
};

