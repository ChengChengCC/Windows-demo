
// FileRecover.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CFileRecoverApp:
// �йش����ʵ�֣������ FileRecover.cpp
//

class CFileRecoverApp : public CWinApp
{
public:
	CFileRecoverApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CFileRecoverApp theApp;