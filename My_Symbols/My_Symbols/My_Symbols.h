
// My_Symbols.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号
#include "IniFile.h"

// CMy_SymbolsApp:
// 有关此类的实现，请参阅 My_Symbols.cpp
//

class CMy_SymbolsApp : public CWinApp
{
public:
	CMy_SymbolsApp();

// 重写
public:
	virtual BOOL InitInstance();
	CIniFile m_IniFile;                //该成员是进行我们的ini设置文件的读写
// 实现

	DECLARE_MESSAGE_MAP()
};

extern CMy_SymbolsApp theApp;