#pragma once
#pragma pack(1)

#define BUFSIZE 2048
#define WM_SOCKET (WM_USER+1)

enum CMGAMESTATE
{
	GAMESTATE_READY,
	GAMESTATE_STAND,
	GAMESTATE_CORRECT,
	GAMESTATE_RECORD,
	GAMESTATE_GAME,
	GAMESTATE_TIME_OVER,
};

struct POINTFORPACKET
{
	int x;
	int y;
};

struct LINEINFO
{
	POINTFORPACKET points[2];
	unsigned int color;
	int width;
};

struct ROOMINFOFORPACKET
{
	char roomName[40];
	int roomNum;
	int userNum;
};

struct GAMEUSERINFO
{
	int score;
	bool master;
	bool turn;
	char name[24];
};

enum PACKET_TYPE
{
	PACKET_TYPE_CHAT,
	PACKET_TYPE_LOGIN,
	PACKET_TYPE_LOGIN_RESULT,
	PACKET_TYPE_USER_LIST,
	PACKET_TYPE_MAKE_ROOM,
	PACKET_TYPE_CHANGE_ROOM,
	PACKET_TYPE_ROOM_LIST,
	PACKET_TYPE_WANT_GO_ROOM,
	PACKET_TYPE_ROOM_USER_INFO_LIST,
	PACKET_TYPE_GAME_CHAT,
	PACKET_TYPE_LINE,
	PACKET_TYPE_ERASE,
	PACKET_TYPE_GAME_START,
	PACKET_TYPE_TURN_INFO,
	PACKET_TYPE_TURN_START,
	PACKET_TYPE_CORRECT,
	PACKET_TYPE_END_GAME,
	PACKET_TYPE_READY_GAME,
	PACKET_TYPE_TIME_OVER,
};

struct PACKET_HEADER
{
	PACKET_TYPE type;
	int packetLen;
};

struct PACKET_CHAT
{
	PACKET_HEADER header;
	char name[24];
	char buf[BUFSIZE];
};

struct PACKET_ERASE
{
	PACKET_HEADER header;
};

struct PACKET_GAME_CHAT
{
	PACKET_HEADER header;
	char name[24];
	char buf[BUFSIZE];
};

struct PACKET_LINE
{
	PACKET_HEADER header;
	LINEINFO line;
};

struct PACKET_LOGIN
{
	PACKET_HEADER header;
	char name[24];
};

struct PACKET_LOGIN_RESULT
{
	PACKET_HEADER header;
	char name[24];
};

struct PACKET_USER_LIST
{
	PACKET_HEADER header;
	int iUserCount;
	char userList[100][24];
};

struct PACKET_MAKE_ROOM
{
	PACKET_HEADER header;
	char roomName[64];
};

struct PACKET_CHANGE_ROOM
{
	PACKET_HEADER header;
};

struct PACKET_ROOM_LIST
{
	PACKET_HEADER header;
	int roomCount;
	ROOMINFOFORPACKET room[20];
};

struct PACKET_WANT_GO_ROOM
{
	PACKET_HEADER header;
	int roomNum;
};

struct PACKET_ROOM_USER_INFO_LIST
{
	PACKET_HEADER header;
	int userNum;
	CMGAMESTATE gameState;
	GAMEUSERINFO user[8];
};

struct PACKET_GAME_START
{
	PACKET_HEADER header;
};

struct PACKET_TURN_INFO
{
	PACKET_HEADER header;
	int round;
	char answer[24];
	char curTurnUserName[24];
	char nextTurnUserName[24];
};

struct PACKET_TURN_START
{
	PACKET_HEADER header;
};

struct PACKET_CORRECT
{
	PACKET_HEADER header;
	char answererName[24];
};

struct PACKET_END_GAME
{
	PACKET_HEADER header;
};

struct PACKET_READY_GAME
{
	PACKET_HEADER header;
};

struct PACKET_TIME_OVER
{
	PACKET_HEADER header;
};

#pragma pack()