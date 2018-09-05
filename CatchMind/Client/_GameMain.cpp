#include "../../HCHEngine/GameEngineMain.h"
#include "../../HCHEngine/GameManager.h"
#include "../../HCHEngine/Scene.h"

#include "LoginScene.h"
#include "RobbyScene.h"
#include "GameScene.h"
#include "NetWork.h"

using namespace HCHEngine;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtDumpMemoryLeaks();
	//_CrtSetBreakAlloc(1620);
	
	if (NetWork::GetInstance()->StartNetWork(9000) == false)
	{
		return -1;
	}

	GameEngineMain engine("Catch Mind", 768, 576);

	GameManager::GetInstance()->RegisterScene(new LoginScene);
	GameManager::GetInstance()->RegisterScene(new RobbyScene);//차례대로 SceneIndex 0부터..
	GameManager::GetInstance()->RegisterScene(new GameScene);//차례대로 SceneIndex 0부터..

	int engineRetval = engine.StartEngine(hInstance);

	NetWork::GetInstance()->Release();
	NetWork::Destroy();

	return engineRetval;
}