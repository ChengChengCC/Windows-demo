
// My_Symbols.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������
#include "IniFile.h"

// CMy_SymbolsApp:
// �йش����ʵ�֣������ My_Symbols.cpp
//

class CMy_SymbolsApp : public CWinApp
{
public:
	CMy_SymbolsApp();

// ��д
public:
	virtual BOOL InitInstance();
	CIniFile m_IniFile;                //�ó�Ա�ǽ������ǵ�ini�����ļ��Ķ�д
// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CMy_SymbolsApp theApp;