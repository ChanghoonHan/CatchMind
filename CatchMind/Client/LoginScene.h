#pragma once
#include "../../HCHEngine/Scene.h"
#include "../../HCHEngine/BitMap.h"

class LoginScene :
	public Scene
{
	HWND m_hEditBox;

public:
	virtual void Init();
	virtual void InputOperator(float fElapsedTime);
	virtual void Update(float fElapsedTime);
	virtual bool Draw();
	virtual void Release();

	LoginScene();
	virtual ~LoginScene();
};

