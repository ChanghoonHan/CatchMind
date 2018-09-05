#pragma once
#include <string>
#include "../../HCHEngine/BitMap.h"

class RoomUser
{
	std::string m_strName;
	std::string m_strChatBuf;

	static BitMap* m_pMasterBackLeft;
	static BitMap* m_pMasterBackRight;
	static BitMap* m_pSmallTalkBallonLeft;
	static BitMap* m_pSmallTalkBallonRight;
	static BitMap* m_pBigTalkBallonLeft;
	static BitMap* m_pBigTalkBallonRight;
	static BitMap* m_pTrunBackLeft;
	static BitMap* m_pTrunBackRight;

	int m_score;
	bool m_master;
	bool m_turn;

	int m_direct;

	bool m_bChat;
	float m_fChatTime;

public:
	static void GlobalInit();

	void Init();
	void InitScore();
	void UserSet(std::string name, bool roomMaster, bool turn, int m_score, int direct);
	void ChangeDirect(int direct);
	bool IsSameName(std::string ohterName);
	void SetTurn(bool turn);
	string GetName();
	int GetScore();
	void Update(float time);
	void Draw();
	void Chat(std::string str);

	RoomUser();
	~RoomUser();
};

