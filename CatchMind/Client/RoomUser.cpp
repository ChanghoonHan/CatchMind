#include "RoomUser.h"
#include "../../HCHEngine/DrawManager.h"

BitMap* RoomUser::m_pMasterBackLeft = NULL;
BitMap* RoomUser::m_pMasterBackRight = NULL;
BitMap* RoomUser::m_pSmallTalkBallonLeft = NULL;
BitMap* RoomUser::m_pSmallTalkBallonRight = NULL;
BitMap* RoomUser::m_pBigTalkBallonLeft = NULL;
BitMap* RoomUser::m_pBigTalkBallonRight = NULL;
BitMap* RoomUser::m_pTrunBackLeft = NULL;
BitMap* RoomUser::m_pTrunBackRight = NULL;

void RoomUser::GlobalInit()
{
	m_pMasterBackLeft = DrawManager::GetInstnace()->GetBitMap("RoomReaderLeft.bmp");
	m_pMasterBackRight = DrawManager::GetInstnace()->GetBitMap("RoomReaderRight.bmp");
	m_pSmallTalkBallonLeft = DrawManager::GetInstnace()->GetBitMap("SmallTalkBallonLeft.bmp");
	m_pSmallTalkBallonRight = DrawManager::GetInstnace()->GetBitMap("SmallTalkBallonRight.bmp");
	m_pBigTalkBallonLeft = DrawManager::GetInstnace()->GetBitMap("BigTalkBallonLeft.bmp");
	m_pBigTalkBallonRight = DrawManager::GetInstnace()->GetBitMap("BigTalkBallonRight.bmp");
	m_pTrunBackLeft = DrawManager::GetInstnace()->GetBitMap("TurnBackLeft.bmp");
	m_pTrunBackRight = DrawManager::GetInstnace()->GetBitMap("TurnBackRight.bmp");
}

void RoomUser::Init()
{
	m_bChat = 0;
	m_fChatTime = 0;
	m_turn = false;
}

void RoomUser::UserSet(std::string name, bool roomMaster, bool turn, int score, int direct)
{
	m_strName = name;
	m_master = roomMaster;
	m_score = score;
	m_turn = turn;
	m_direct = direct;
}

void RoomUser::ChangeDirect(int direct)
{
	m_direct = direct;
}

bool RoomUser::IsSameName(std::string ohterName)
{
	return (m_strName == ohterName);
}

void RoomUser::Update(float time)
{
	if (m_bChat)
	{
		m_fChatTime += time;
		if (m_fChatTime  > 2)
		{
			m_fChatTime = 0;
			m_bChat = false;
		}
	}

	return;
}

void RoomUser::Draw()
{
	if (m_master)
	{
		if (m_direct % 2 == 0)
		{
			m_pMasterBackLeft->Draw(33, 106 + (83 * (m_direct / 2)));
		}
		else
		{
			m_pMasterBackRight->Draw(33 + 579, 106 + (83 * (m_direct / 2)));
		}
	}

	if (m_turn)
	{
		if (m_direct % 2 == 0)
		{
			m_pTrunBackLeft->Draw(29, 99 + (83 * (m_direct / 2)));
		}
		else
		{
			m_pTrunBackRight->Draw(29 + 579, 99 + (83 * (m_direct / 2)));
		}
	}

	char buf[20];

	if (m_direct % 2 == 0)
	{
		DrawManager::GetInstnace()->PutText(m_strName.c_str(), 36, 113 + (83 * (m_direct / 2)), 91, 133 + (83 * (m_direct / 2)), DT_CENTER);
		sprintf(buf, "%d",m_score);
		DrawManager::GetInstnace()->PutText(buf, 36, 152 + (83 * (m_direct / 2)), 91, 172 + (83 * (m_direct / 2)), DT_CENTER);
	}
	else
	{
		DrawManager::GetInstnace()->PutText(m_strName.c_str(), 36 + 641, 113 + (83 * (m_direct / 2)), 91 + 641, 133 + (83 * (m_direct / 2)), DT_CENTER);
		sprintf(buf, "%d", m_score);
		DrawManager::GetInstnace()->PutText(buf, 36 + 641, 152 + (83 * (m_direct / 2)), 91 + 641, 172 + (83 * (m_direct / 2)), DT_CENTER);
	}

	if (m_bChat)
	{
		int ballonX = 0;
		int ballonY = 0;
		int tempDiret = 0;
		SIZE size;
		GetTextExtentPoint(DrawManager::GetInstnace()->GetDC(), m_strChatBuf.c_str(), strlen(m_strChatBuf.c_str()), &size);

		if (m_direct % 2 == 0)
		{
			ballonX = 150;
			ballonY = 113 + (83 * (m_direct / 2));
			if (size.cx > 128)
			{
				m_pBigTalkBallonLeft->Draw(ballonX, ballonY);
			}
			else
			{
				m_pSmallTalkBallonLeft->Draw(ballonX, ballonY);
			}

			DrawManager::GetInstnace()->PutText(m_strChatBuf, ballonX + 25, ballonY + 5, ballonX + 25 + 130, ballonY + 5 + 35, DT_LEFT | DT_WORDBREAK | DT_EDITCONTROL);
		}
		else
		{
			ballonX = 460;
			ballonY = 113 + (83 * (m_direct / 2));
			if (size.cx > 128)
			{
				m_pBigTalkBallonRight->Draw(ballonX, ballonY);
			}
			else
			{
				m_pSmallTalkBallonRight->Draw(ballonX, ballonY);
			}
			DrawManager::GetInstnace()->PutText(m_strChatBuf, ballonX + 5, ballonY + 5, ballonX + 5 + 130, ballonY + 5 + 35, DT_LEFT | DT_WORDBREAK | DT_EDITCONTROL);
		}

	}
}

void RoomUser::SetTurn(bool turn)
{
	m_turn = turn;
}

int RoomUser::GetScore()
{
	return m_score;
}

void RoomUser::InitScore()
{
	m_score = 0;
}

string RoomUser::GetName()
{
	return m_strName;
}

void RoomUser::Chat(std::string str)
{
	m_strChatBuf = str;
	m_bChat = true;
	m_fChatTime = 0;
}

RoomUser::RoomUser()
{
}


RoomUser::~RoomUser()
{
}
