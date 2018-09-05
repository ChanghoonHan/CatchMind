#include "GameScene.h"
#include "NetWork.h"
#include <mutex>
#include <math.h>
#include <process.h>

void err_quit(char *msg);
void err_display(char *msg);
unsigned int WINAPI GameSceneRecvThread(LPVOID arg);
mutex lineMutex;
mutex gameUsersMutex;

int GameScene::BlackPenButtonFunc()
{
	m_curColor = BLACKPEN;

	return 0;
}

int GameScene::RedPenButtonFunc()
{
	m_curColor = REDPEN;

	return 0;
}

int GameScene::GreenPenButtonFunc()
{
	m_curColor = GREENPEN;

	return 0;
}

int GameScene::BluePenButtonFunc()
{
	m_curColor = BLUEPEN;

	return 0;
}

int GameScene::YellowPenButtonFunc()
{
	m_curColor = YELLOWPEN;

	return 0;
}

int GameScene::WhitePenButtonFunc()
{
	m_curColor = WHITEPEN;

	return 0;
}

int GameScene::EraserButtonFunc()
{
	m_curColor = WHITEPEN;

	return 0;
}

int GameScene::AllClearButtonFunc()
{
	m_mylines.clear();

	PACKET_ERASE pe;
	pe.header.type = PACKET_TYPE_ERASE;
	pe.header.packetLen = sizeof(pe);

	NetWork::GetInstance()->SendPacket((char*)&pe, pe.header.packetLen);

	return 0;
}

int GameScene::ExitButtonFunc()
{
	PACKET_WANT_GO_ROOM pwgr;
	pwgr.header.type = PACKET_TYPE_WANT_GO_ROOM;
	pwgr.header.packetLen = sizeof(PACKET_HEADER) + sizeof(int);
	pwgr.roomNum = 0;

	NetWork::GetInstance()->SendPacket((char*)&pwgr, pwgr.header.packetLen);

	GameManager::GetInstance()->LoadScene(1);

	return 0;
}

int GameScene::GameStartButtonFunc()
{
	PACKET_GAME_START pgs;
	pgs.header.type = PACKET_TYPE_GAME_START;
	pgs.header.packetLen = sizeof(pgs);

	NetWork::GetInstance()->SendPacket((char*)&pgs, sizeof(pgs));
	return 0;
}

void GameScene::Init()
{
	m_pBackBitMap = DrawManager::GetInstnace()->GetBitMap("GameBack2.bmp");
	m_pNoPalatte = DrawManager::GetInstnace()->GetBitMap("NoPalette.bmp");
	m_pRoundWindow = DrawManager::GetInstnace()->GetBitMap("RoundWindow.bmp");
	m_pCorrectWindow = DrawManager::GetInstnace()->GetBitMap("CorrectWindow.bmp");
	m_pRecordsWindow = DrawManager::GetInstnace()->GetBitMap("RecordsWindow.bmp");
	m_pTimeOverWindow = DrawManager::GetInstnace()->GetBitMap("TimeOver.bmp");

	m_exitButton.Init("RoomExitButton1.bmp", "RoomExitButton2.bmp", 622, 21, 64, 27, bind(&GameScene::ExitButtonFunc, this));
	m_gameStartButton.Init("StartButton1.bmp", "StartButton2.bmp", 420, 395, 64, 32, bind(&GameScene::GameStartButtonFunc, this));

	m_allClearButton.Init("AllClearButton1.bmp", "AllClearButton2.bmp", 420, 367, 64, 23, bind(&GameScene::AllClearButtonFunc, this));
	m_eraseButton.Init("EraserButton1.bmp", "EraserButton2.bmp", 360, 367, 32, 19, bind(&GameScene::EraserButtonFunc, this));
	m_blackPenButton.Init("BlackButton1.bmp", "BlackButton2.bmp", 200, 367, 23, 19, bind(&GameScene::BlackPenButtonFunc, this));
	m_redPenButton.Init("RedButton1.bmp", "RedButton2.bmp", 224, 364, 23, 21, bind(&GameScene::RedPenButtonFunc, this));
	m_bluePenButton.Init("BlueButton1.bmp", "BlueButton2.bmp", 248, 365, 24, 22, bind(&GameScene::BluePenButtonFunc, this));
	m_greenPenButton.Init("GreenButton1.bmp", "GreenButton2.bmp", 272, 366, 25, 21, bind(&GameScene::GreenPenButtonFunc, this));
	m_yellowPenButton.Init("YellowButton1.bmp", "YellowButton2.bmp", 297, 366, 22, 21, bind(&GameScene::YellowPenButtonFunc, this));
	m_whitePenButton.Init("WhiteButton1.bmp", "WhiteButton2.bmp", 321, 365, 24, 21, bind(&GameScene::WhitePenButtonFunc, this));

	m_hChatEditBox = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		 373, 455, 215, 20, DrawManager::GetInstnace()->GetHWnd(), NULL, NULL, NULL);
	m_curColor = BLACKPEN;
	RoomUser::GlobalInit();
	m_mylines.clear();
	m_lines.clear();

	m_fTime = 140;

	m_bTurn = false;
	m_gameState = GAMESTATE_READY;

	m_bIsLeftDrawing = false;
	m_bIsRightDrawing = false;
	m_rectPaper.SetRect(190, 106, 577, 353);
	OperatorManager::GetInstance()->KeyClear();
	OperatorManager::GetInstance()->RegisterKey(13);
	OperatorManager::GetInstance()->RegisterKey(VK_LBUTTON);
	OperatorManager::GetInstance()->RegisterKey(VK_RBUTTON);
	m_bIsSceneChange = false;
	m_threadArg.bSceneChange = &m_bIsSceneChange;
	m_threadArg.sock = NetWork::GetInstance()->GetSock();
	m_threadArg.name = &m_myName;
	m_threadArg.pUsers = m_Users;
	m_threadArg.userNameTemp = m_userNameTemp;
	m_threadArg.bTrun = &m_bTurn;
	m_threadArg.lines = &m_lines;
	m_threadArg.myLines = &m_mylines;
	m_threadArg.bIsMaster = &m_bMaster;
	m_threadArg.pGameState = &m_gameState;
	m_threadArg.round = &m_round;
	m_threadArg.answer = &m_answer;
	m_threadArg.time = &m_fTime;

	m_round = 0;
	for (size_t i = 0; i < 8; i++)
	{
		m_threadArg.pUsers[i] = NULL;
	}
	m_hThread = _beginthreadex(NULL, 0, GameSceneRecvThread, (LPVOID)&m_threadArg, 0, 0);
}

void GameScene::InputOperator(float fElapsedTime)
{
	if (m_gameState == GAMESTATE_READY || (m_gameState == GAMESTATE_GAME && m_bTurn))
	{
		if (OperatorManager::GetInstance()->IsKeyDown(VK_LBUTTON) && !m_bIsRightDrawing)
		{
			Point* mpos = OperatorManager::GetInstance()->GetMousePos();
			if (m_rectPaper.collWithPoint(*mpos))
			{
				m_bIsLeftDrawing = true;
				LINEINFO LS;
				LS.color = m_curColor;
				LS.width = 3;
				LS.points[0].x = mpos->iX;
				LS.points[0].y = mpos->iY;
				LS.points[1].x = mpos->iX;
				LS.points[1].y = mpos->iY;

				m_mylines.push_back(LS);
			}
		}

		if (OperatorManager::GetInstance()->IsKeyUp(VK_LBUTTON) && m_bIsLeftDrawing)
		{
			auto lineInfo = m_mylines.end();
			lineInfo--;
			SendLine(*lineInfo);
			m_bIsLeftDrawing = false;
		}

		if (OperatorManager::GetInstance()->IsKeyPress(VK_LBUTTON) && m_bIsLeftDrawing)
		{
			Point* mpos = OperatorManager::GetInstance()->GetMousePos();
			if (m_rectPaper.collWithPoint(*mpos))
			{
				auto lineInfo = m_mylines.end();
				lineInfo--;

				if (!((mpos->iX == lineInfo->points[0].x) && (mpos->iY == lineInfo->points[0].y)))
				{
					lineInfo->points[1].x = mpos->iX;
					lineInfo->points[1].y = mpos->iY;
					SendLine(*lineInfo);

					LINEINFO LS;
					LS.color = m_curColor;
					LS.width = 3;
					LS.points[0].x = mpos->iX;
					LS.points[0].y = mpos->iY;
					LS.points[1].x = mpos->iX;
					LS.points[1].y = mpos->iY;
					m_mylines.push_back(LS);
				}
			}
			else
			{
				auto lineInfo = m_mylines.end();
				lineInfo--;
				190, 106, 577, 353;
				if (mpos->iX > 577)
				{
					mpos->iX = 577;
				}

				if (mpos->iX < 190)
				{
					mpos->iX = 190;
				}

				if (mpos->iY > 353)
				{
					mpos->iY = 353;
				}

				if (mpos->iY < 106)
				{
					mpos->iY = 106;
				}

				lineInfo->points[1].x = mpos->iX;
				lineInfo->points[1].y = mpos->iY;
				SendLine(*lineInfo);
				m_bIsLeftDrawing = false;
			}
		}
		///////////////////오른쪽 마우스

		if (OperatorManager::GetInstance()->IsKeyDown(VK_RBUTTON) && !m_bIsLeftDrawing)
		{
			Point* mpos = OperatorManager::GetInstance()->GetMousePos();
			if (m_rectPaper.collWithPoint(*mpos))
			{
				m_bIsRightDrawing = true;
				LINEINFO LS;
				LS.color = m_curColor;
				LS.width = 20;
				LS.points[0].x = mpos->iX;
				LS.points[0].y = mpos->iY;
				LS.points[1].x = mpos->iX;
				LS.points[1].y = mpos->iY;

				m_mylines.push_back(LS);
			}
		}

		if (OperatorManager::GetInstance()->IsKeyUp(VK_RBUTTON) && m_bIsRightDrawing)
		{
			auto lineInfo = m_mylines.end();
			lineInfo--;
			SendLine(*lineInfo);
			m_bIsRightDrawing = false;
		}

		if (OperatorManager::GetInstance()->IsKeyPress(VK_RBUTTON) && m_bIsRightDrawing)
		{
			Point* mpos = OperatorManager::GetInstance()->GetMousePos();
			if (m_rectPaper.collWithPoint(*mpos))
			{
				auto lineInfo = m_mylines.end();
				lineInfo--;

				if (!((mpos->iX == lineInfo->points[0].x) && (mpos->iY == lineInfo->points[0].y)))
				{
					lineInfo->points[1].x = mpos->iX;
					lineInfo->points[1].y = mpos->iY;
					SendLine(*lineInfo);

					LINEINFO LS;
					LS.color = m_curColor;
					LS.width = 20;
					LS.points[0].x = mpos->iX;
					LS.points[0].y = mpos->iY;
					LS.points[1].x = mpos->iX;
					LS.points[1].y = mpos->iY;
					m_mylines.push_back(LS);
				}
			}
			else
			{
				auto lineInfo = m_mylines.end();
				lineInfo--;
				190, 106, 577, 353;
				if (mpos->iX > 577)
				{
					mpos->iX = 577;
				}

				if (mpos->iX < 190)
				{
					mpos->iX = 190;
				}

				if (mpos->iY > 353)
				{
					mpos->iY = 353;
				}

				if (mpos->iY < 106)
				{
					mpos->iY = 106;
				}

				lineInfo->points[1].x = mpos->iX;
				lineInfo->points[1].y = mpos->iY;
				SendLine(*lineInfo);
				m_bIsRightDrawing = false;
			}
		}
	}

	
		if (OperatorManager::GetInstance()->IsKeyDown(13))
		{
			int len;
			char buf[BUFSIZE];

			GetWindowText(m_hChatEditBox, buf, BUFSIZE);
			if (strlen(buf) == 0)
			{
				return;
			}

			SetWindowText(m_hChatEditBox, "");

			if (m_gameState == GAMESTATE_READY || (m_gameState == GAMESTATE_GAME && !m_bTurn))
			{
				len = strlen(buf);
				if (buf[len - 1] == '\n')
				{
					buf[len - 1] = '\0';
				}
				PACKET_GAME_CHAT pc;
				pc.header.type = PACKET_TYPE_GAME_CHAT;
				pc.header.packetLen = sizeof(PACKET_HEADER) + 24 + len + 1;
				strcpy(pc.name, m_myName.c_str());
				strcpy(pc.buf, buf);

				NetWork::GetInstance()->SendPacket((char*)&pc, pc.header.packetLen);
			}
		}
}

void GameScene::Update(float fElapsedTime)
{	
	gameUsersMutex.lock();
	for (size_t i = 0; i < 8; i++)
	{
		if (m_Users[i] == NULL)
		{
			continue;
		}
		m_Users[i]->Update(fElapsedTime);
	}
	gameUsersMutex.unlock();
	m_exitButton.Update();

	if (m_gameState == GAMESTATE_GAME)
	{
		m_fTime -= fElapsedTime;
		if (m_fTime < 0)
		{
			m_fTime = 0;
			if (m_bTurn)
			{
				PACKET_TIME_OVER pto;
				pto.header.type = PACKET_TYPE_TIME_OVER;
				pto.header.packetLen = sizeof(PACKET_HEADER);

				lineMutex.lock();
				m_lines.clear();
				m_mylines.clear();
				lineMutex.unlock();

				m_gameState = GAMESTATE_TIME_OVER;

				NetWork::GetInstance()->SendPacket((char*)&pto, pto.header.packetLen);
			}
		}
	}

	if (m_gameState == GAMESTATE_READY || (m_gameState == GAMESTATE_GAME && m_bTurn))
	{
		m_allClearButton.Update();
		m_eraseButton.Update();
		m_blackPenButton.Update();
		m_redPenButton.Update();
		m_greenPenButton.Update();
		m_bluePenButton.Update();
		m_yellowPenButton.Update();
		m_whitePenButton.Update();
	}

	if (m_gameState == GAMESTATE_READY && m_bMaster)
	{
		m_gameStartButton.Update();
	}
}

bool GameScene::Draw()
{
	m_pBackBitMap->Draw(0, 0);
	m_exitButton.Draw();

	if (m_gameState == GAMESTATE_TIME_OVER)
	{
		m_pTimeOverWindow->Draw(313, 150);

		char roundBuf[20];
		sprintf(roundBuf, "ROUND %d", m_round);
		DrawManager::GetInstnace()->PutText(roundBuf, 180, 70, 260, 90, DT_LEFT);
	}

	if (m_gameState == GAMESTATE_STAND)
	{
		m_pRoundWindow->Draw(258, 150);

		DrawManager::GetInstnace()->PutText(m_userNameTemp[0], 275, 160, 343, 180, DT_CENTER);
		DrawManager::GetInstnace()->PutText(m_userNameTemp[1], 275, 182, 343, 202, DT_CENTER);

		char roundBuf[20];
		sprintf(roundBuf, "ROUND %d", m_round);
		DrawManager::GetInstnace()->PutText(roundBuf, 180, 70, 260, 90, DT_LEFT);
	}


	if (m_gameState == GAMESTATE_CORRECT)
	{
		m_pCorrectWindow->Draw(275, 150);

		DrawManager::GetInstnace()->PutText(m_userNameTemp[0], 355, 160, 423, 180, DT_CENTER);
		DrawManager::GetInstnace()->PutText(m_userNameTemp[1], 355, 182, 423, 202, DT_CENTER);

		DrawManager::GetInstnace()->PutText("1", 450, 160, 470, 180, DT_LEFT);
		DrawManager::GetInstnace()->PutText("1", 450, 182, 470, 202, DT_LEFT);

		char roundBuf[20];
		sprintf(roundBuf, "ROUND %d", m_round);
		DrawManager::GetInstnace()->PutText(roundBuf, 180, 70, 260, 90, DT_LEFT);
		DrawManager::GetInstnace()->PutText(m_answer, 284, 70, 484, 90, DT_CENTER);
	}

	if (m_gameState == GAMESTATE_GAME)
	{
		char roundBuf[20];
		sprintf(roundBuf, "ROUND %d", m_round);

		char timeBuf[20];
		sprintf(timeBuf, "%02d : %02d : %02d", (int)(m_fTime / 60), ((int)m_fTime % 60), (int)((m_fTime - (int)m_fTime) * 100));
		DrawManager::GetInstnace()->PutText(roundBuf, 180, 70, 260, 90, DT_LEFT);
		DrawManager::GetInstnace()->PutText(timeBuf, 207, 450, 307, 470, DT_LEFT);
		if (m_bTurn)
		{
			DrawManager::GetInstnace()->PutText(m_answer, 284, 70, 484, 90, DT_CENTER);
		}
	}

	if (m_gameState == GAMESTATE_RECORD)
	{
		m_pRecordsWindow->Draw(274, 70);
		vector<RECORD_DATA> rd;
		GetRecored(rd);

		char scoreBuf[4];
		int i = 0;
		for (auto iter = rd.begin(); iter != rd.end(); iter++, i++)
		{
			DrawManager::GetInstnace()->PutText(iter->name, 315, 147 + (i * 22), 390, 167 + (i * 22), DT_CENTER);
			sprintf(scoreBuf, "%d", iter->score);
			DrawManager::GetInstnace()->PutText(scoreBuf, 400, 147 + (i * 22), 460, 167 + (i * 22), DT_CENTER);
		}
	}

	if (m_gameState == GAMESTATE_READY || (m_gameState == GAMESTATE_GAME && m_bTurn))
	{
		m_allClearButton.Draw();
		m_blackPenButton.Draw();
		m_redPenButton.Draw();
		m_greenPenButton.Draw();
		m_bluePenButton.Draw();
		m_yellowPenButton.Draw();
		m_whitePenButton.Draw();
		m_eraseButton.Draw();
	}
	else
	{
		m_pNoPalatte->Draw(165, 361);
	}

	lineMutex.lock();
	for (auto iter = m_lines.begin(); iter != m_lines.end(); iter++)
	{
		DrawManager::GetInstnace()->DrawLine((POINT*)iter->points, iter->width, iter->color);
	}
	lineMutex.unlock();

	gameUsersMutex.lock();
	for (size_t i = 0; i < 8; i++)
	{
		if (m_Users[i] == NULL)
		{
			continue;
		}
		m_Users[i]->Draw();
	}
	gameUsersMutex.unlock();

	if (m_gameState == GAMESTATE_READY && m_bMaster)
	{
		m_gameStartButton.Draw();
	}

	return true;
}

unsigned int WINAPI GameSceneRecvThread(LPVOID arg)
{
	THREAD_GAME_ARG* Targ = (THREAD_GAME_ARG*)arg;

	SOCKET sock = Targ->sock;
	char buf[2048];
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
		case PACKET_TYPE_GAME_CHAT:
		{
			PACKET_GAME_CHAT* pgc = (PACKET_GAME_CHAT*)buf;
			for (size_t i = 0; i < 8; i++)
			{
				if (Targ->pUsers[i] == NULL)
				{
					continue;
				}

				if (Targ->pUsers[i]->IsSameName(pgc->name))
				{
					Targ->pUsers[i]->Chat(pgc->buf);
					break;
				}
			}
		}
		break;

		case PACKET_TYPE_LINE:
		{
			lineMutex.lock();
			PACKET_LINE* pl = (PACKET_LINE*)buf;
			Targ->lines->push_back(pl->line);
			lineMutex.unlock();
		}
		break;

		case PACKET_TYPE_ERASE:
		{
			lineMutex.lock();
			Targ->lines->clear();
			Targ->myLines->clear();
			lineMutex.unlock();
		}
		break;

		case PACKET_TYPE_TIME_OVER:
		{
			lineMutex.lock();
			Targ->lines->clear();
			Targ->myLines->clear();
			lineMutex.unlock();
			*Targ->pGameState = GAMESTATE_TIME_OVER;
		}
		break;

		case PACKET_TYPE_ROOM_USER_INFO_LIST:
		{
			PACKET_ROOM_USER_INFO_LIST* pruif = (PACKET_ROOM_USER_INFO_LIST*)buf;
			vector<RoomUser*> vectempUser;
			gameUsersMutex.lock();
			bool IsInList = false;
			for (size_t i = 0; i < pruif->userNum; i++)
			{
				IsInList = false;
				if (strcmp(pruif->user[i].name, Targ->name->c_str()) == 0)
				{
					*Targ->bTrun = pruif->user[i].turn;
					*Targ->bIsMaster = pruif->user[i].master;
					*Targ->pGameState = pruif->gameState;
				}

				for (size_t j = 0; j < 8; j++)
				{
					if (Targ->pUsers[j] == NULL)
					{
						continue;
					}

					if (Targ->pUsers[j]->IsSameName(pruif->user[i].name))
					{
						Targ->pUsers[j]->UserSet(pruif->user[i].name, pruif->user[i].master, pruif->user[i].turn, pruif->user[i].score, j);
						IsInList = true;
						break;
					}
				}

				if (IsInList)
				{
					continue;
				}

				RoomUser* newRoomUser = new RoomUser;
				newRoomUser->Init();
				newRoomUser->UserSet(pruif->user[i].name, pruif->user[i].master, pruif->user[i].turn, pruif->user[i].score, -1);
				vectempUser.push_back(newRoomUser);
			}

			for (size_t i = 0; i < 8; i++)
			{
				if (Targ->pUsers[i] == NULL)
				{
					continue;
				}

				IsInList = false;
				for (size_t j = 0; j < pruif->userNum; j++)
				{
					if (Targ->pUsers[i]->IsSameName(pruif->user[j].name))
					{
						IsInList = true;
						break;
					}
				}

				if (IsInList)
				{
					continue;
				}

				delete Targ->pUsers[i];
				Targ->pUsers[i] = NULL;
			}

			for (auto iter = vectempUser.begin(); iter != vectempUser.end(); iter++)
			{
				for (size_t i = 0; i < 8; i++)
				{
					if (Targ->pUsers[i] != NULL)
					{
						continue;
					}

					Targ->pUsers[i] = *iter;
					(*iter)->ChangeDirect(i);
					break;
				}
			}
			gameUsersMutex.unlock();
		}
		break;

		case PACKET_TYPE_LOGIN_RESULT:
		{
			PACKET_LOGIN_RESULT* plr = (PACKET_LOGIN_RESULT*)buf;
			*Targ->name = plr->name;
		}
		break;

		case PACKET_TYPE_CHANGE_ROOM:
		{
			*Targ->bSceneChange = true;
		}
		break;

		case PACKET_TYPE_END_GAME:
		{
			lineMutex.lock();
			Targ->lines->clear();
			Targ->myLines->clear();
			lineMutex.unlock();

			*Targ->pGameState = GAMESTATE_RECORD;
		}
		break;

		case PACKET_TYPE_READY_GAME:
		{
			lineMutex.lock();
			Targ->lines->clear();
			Targ->myLines->clear();
			lineMutex.unlock();

			*Targ->pGameState = GAMESTATE_READY;
		}
		break;

		case PACKET_TYPE_TURN_INFO:
		{
			lineMutex.lock();
			Targ->lines->clear();
			Targ->myLines->clear();
			lineMutex.unlock();

			PACKET_TURN_INFO* pti = (PACKET_TURN_INFO*)buf;
			Targ->userNameTemp[0] = pti->curTurnUserName;
			Targ->userNameTemp[1] = pti->nextTurnUserName;
			*Targ->answer = pti->answer;

			*Targ->round = pti->round;
			*Targ->pGameState = GAMESTATE_STAND;
		}
		break;

		case PACKET_TYPE_TURN_START:
		{
			*Targ->time = 140;
			*Targ->pGameState = GAMESTATE_GAME;
		}
		break;

		case PACKET_TYPE_CORRECT:
		{
			lineMutex.lock();
			Targ->lines->clear();
			Targ->myLines->clear();
			lineMutex.unlock();

			PACKET_CORRECT* pc = (PACKET_CORRECT*)buf;
			Targ->userNameTemp[1] = pc->answererName;

			*Targ->pGameState = GAMESTATE_CORRECT;
		}
		break;

		default:
			break;
		}
	}

	return 0;
}

void GameScene::SendLine(LINEINFO& line)
{
	PACKET_LINE pl;
	pl.header.type = PACKET_TYPE_LINE;
	pl.header.packetLen = sizeof(pl);
	pl.line = line;

	NetWork::GetInstance()->SendPacket((char*)&pl, pl.header.packetLen);
}

void GameScene::GetRecored(vector<RECORD_DATA>& vecRecord)
{
	RECORD_DATA temp;
	gameUsersMutex.lock();
	for (size_t i = 0; i < 8; i++)
	{
		if (m_Users[i] == NULL)
		{
			continue;
		}

		temp.name = m_Users[i]->GetName();
		temp.score = m_Users[i]->GetScore();
		vecRecord.push_back(temp);
	}
	gameUsersMutex.unlock();

	if (vecRecord.size() == 0 || vecRecord.size() == 1)
	{
		return;
	}

	for (size_t i = 0; i < vecRecord.size()-1; i++)
	{
		for (size_t j = 0; j < vecRecord.size() - 1; j++)
		{
			if (vecRecord[j].score < vecRecord[j+1].score)
			{
				temp = vecRecord[j];
				vecRecord[j] = vecRecord[j + 1];
				vecRecord[j + 1] = temp;
			}
		}
	}
}

void GameScene::Release()
{
	TerminateThread((HANDLE)m_hThread, 0);
	DestroyWindow(m_hChatEditBox);
}

GameScene::GameScene()
{
}


GameScene::~GameScene()
{
}
