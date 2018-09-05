#include "LoginScene.h"
#include "../../../packetHeader.h"
#include "NetWork.h"

void LoginScene::Init()
{
	m_hEditBox = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		334, 278, 60, 20, DrawManager::GetInstnace()->GetHWnd(), NULL, NULL, NULL);

	OperatorManager::GetInstance()->KeyClear();
	OperatorManager::GetInstance()->RegisterKey(13);
}

void LoginScene::InputOperator(float fElapsedTime)
{
	if (OperatorManager::GetInstance()->IsKeyDown(13))
	{
		int len;
		char buf[24];

		GetWindowText(m_hEditBox, buf, BUFSIZE);
		SetWindowText(m_hEditBox, "");

		len = strlen(buf);
		if (len > 20)
		{
			buf[20] = '\0';
		}
		else
		{
			buf[len] = '\0';
		}

		PACKET_LOGIN pl;
		pl.header.type = PACKET_TYPE_LOGIN;
		pl.header.packetLen = sizeof(PACKET_HEADER) + len + 1;
		strcpy(pl.name, buf);

		NetWork::GetInstance()->SendPacket((char*)&pl, pl.header.packetLen);

		GameManager::GetInstance()->LoadScene(1);
	}
}

void LoginScene::Update(float fElapsedTime)
{

}

bool LoginScene::Draw()
{
	return true;
}

void LoginScene::Release()
{
	DestroyWindow(m_hEditBox);
}

LoginScene::LoginScene()
{
}


LoginScene::~LoginScene()
{
}
