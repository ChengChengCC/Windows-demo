// CCDbg.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include "CHandleException.h"
using namespace  std;


int _tmain(int argc, _TCHAR* argv[])
{
	char            szFileName[MAX_PATH] = "";	
	OPENFILENAME    ofn;

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize  = sizeof(OPENFILENAME);
	ofn.lpstrFile	 = szFileName;
	ofn.nMaxFile     = MAX_PATH;
	ofn.lpstrFilter  = "Exe Files(*.exe)\0*.exe\0All Files(*.*)\0*.*\0\0";
	ofn.nFilterIndex = 1;
	if( GetOpenFileName(&ofn) == FALSE)
	{
		return 0;
	}

	//创建被调试进程
	char                szDirectoryBuf[MAXBYTE] = {0};
	STARTUPINFO         StartupInfo;
	PROCESS_INFORMATION pInfo;

	GetStartupInfo(&StartupInfo);
	BOOL isCreateSucess =  CreateProcess(szFileName, NULL, NULL, NULL, TRUE,
		DEBUG_PROCESS || DEBUG_ONLY_THIS_PROCESS, NULL,
		NULL, &StartupInfo, &pInfo);
	if (isCreateSucess == FALSE)
	{
		printf("Create debug process failed!\r\n");
		return 1;
	}

	//帮助菜单
	ShowHelp(NULL);

	//进入调试循环
	BOOL        isContinue = TRUE;
	DEBUG_EVENT stuDbgEvent = {0};
	DWORD       dwContinueStatus;
	BOOL        bRet;


	 while (isContinue)
	 {
		 dwContinueStatus = DBG_CONTINUE;
		 bRet = WaitForDebugEvent(&stuDbgEvent, INFINITE);
		 if (!bRet)
		 {
			 printf("WaitForDebugEvent error!");
			 return 1;
		 }

		CHandleException::m_stuDbgEvent = stuDbgEvent;

		switch (stuDbgEvent.dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:     //1
			//处理异常
			bRet = CHandleException::HandleException();
			if (bRet == FALSE)
			{
		    	 dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
			} 
			break;
		case CREATE_THREAD_DEBUG_EVENT:   //2
	     	 //创建线程
	    	 break;
		 case CREATE_PROCESS_DEBUG_EVENT:  //3
			 //创建进程
			 //在OEP处设置一次性断点
			 {
				 CHandleException::m_ProcessId = stuDbgEvent.dwProcessId;
				 CHandleException::m_hProcess = stuDbgEvent.u.CreateProcessInfo.hProcess;
				 CHandleException::m_lpOepAddr = stuDbgEvent.u.CreateProcessInfo.lpStartAddress;
				 StuCommand stuCmd = {0};
				 wsprintf(stuCmd.chParam1, "%p", 
					 stuDbgEvent.u.CreateProcessInfo.lpStartAddress);
				 wsprintf(stuCmd.chParam2, "%s", "once");//一次性断点
					 //在进程的OEP设置一次性断点
				 CHandleException::SetOrdPoint(&stuCmd);
					 //关闭句柄，防止资源泄露
				 bRet = CloseHandle(stuDbgEvent.u.CreateProcessInfo.hThread);
				 if (!bRet)
				 {
					 printf("CloseHandle error!");
					 return 1;
				 }
			 }
				 break;
		 case EXIT_THREAD_DEBUG_EVENT:  //4
			 //退出线程
			 break;
		 case EXIT_PROCESS_DEBUG_EVENT:  //5
			 //退出进程
			 isContinue = FALSE;
			 break;
		 case LOAD_DLL_DEBUG_EVENT:		 //6
			 //加载DLL
			 break;
		 case UNLOAD_DLL_DEBUG_EVENT:	  //7
			 //卸载DLL
			 break;
		 case OUTPUT_DEBUG_STRING_EVENT:	//8
			 break;
		 }	

		 bRet = ContinueDebugEvent(stuDbgEvent.dwProcessId, 
			 stuDbgEvent.dwThreadId, dwContinueStatus);

		 if (!bRet)
		 {
			 printf("ContinueDebugEvent error!");
			 return 1;
		 }

	 }

	system("pause");
	return 0;
}