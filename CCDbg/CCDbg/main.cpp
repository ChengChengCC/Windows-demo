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
		NULL, &StartupInfo, &pInfo);  //创建调试进程
	if (isCreateSucess == FALSE)
	{
		printf("Create debug process failed!\r\n");
		return 1;
	}
	//帮助菜单
	ShowHelp(NULL);

	//进入调试循环
	BOOL        isContinue = TRUE;
	DEBUG_EVENT CCDbgEvent = {0};
	DWORD       dwContinueStatus;
	BOOL        bRet;
	while (isContinue)
	 {
		 dwContinueStatus = DBG_CONTINUE;
		 bRet = WaitForDebugEvent(&CCDbgEvent, INFINITE); //等待被调试进程发生调试事件
		 if (!bRet)
		 {
			 printf("WaitForDebugEvent error!");
			 return 1;
		 }
		CHandleException::m_CCDbgEvent = CCDbgEvent;
		switch (CCDbgEvent.dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:     //1
			//处理异常
			/*
			在调试过程出现了异常，就会产生该调试事件
			*/
			bRet = CHandleException::HandleException();
			if (bRet == FALSE)
			{
		    	 dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;//调试器未处理，由系统继续进行分发
			} 
			break;
		case CREATE_THREAD_DEBUG_EVENT:   //2
	     	 //创建线程
			/*
			在调试进程中创建一个新的进程或者调试器开始调试已激活的进程时，
			就会生成这个调试事件。要注意的是当调试的主线程被创建时不会收的该通知
			*/
	    	 break;
		 case CREATE_PROCESS_DEBUG_EVENT:  //3
			 //创建进程
			 /*
			 进程被创建。当调试的进程刚被创建（还未运行）或调试器开始调试已激活的进程时，
			 就会产生这个事件
			 */
			 //在OEP处设置一次性断点
			 {
				 CHandleException::m_ProcessId = CCDbgEvent.dwProcessId;   //进程ID
				 CHandleException::m_hProcess = CCDbgEvent.u.CreateProcessInfo.hProcess;   //进程句柄
				 CHandleException::m_lpOepAddr = CCDbgEvent.u.CreateProcessInfo.lpStartAddress; //OEP
				 CCCommand CCCmd = {0};
				 wsprintf(CCCmd.chParam1, "%p", 
					 CCDbgEvent.u.CreateProcessInfo.lpStartAddress);
				 wsprintf(CCCmd.chParam2, "%s", "once");//一次性断点
					 //在进程的OEP设置一次性断点,在加载时断下
				 CHandleException::SetOrdPoint(&CCCmd);
					 //关闭句柄，防止资源泄露
				 bRet = CloseHandle(CCDbgEvent.u.CreateProcessInfo.hThread);
				 if (!bRet)
				 {
					 printf("CloseHandle error!");
					 return 1;
				 }
			 }
				 break;
		 case EXIT_THREAD_DEBUG_EVENT:  //4
			 //退出线程
			 /*
			 调试的线程退出时事件发生，调试的主线程退出时
			 不会收到该通知
			 */
			 break;
		 case EXIT_PROCESS_DEBUG_EVENT:  //5
			 //退出进程
			 /*
			 当退出这个进程的最后一个线程时，产生这个事件。
			 */
			 isContinue = FALSE;
			 break;
		 case LOAD_DLL_DEBUG_EVENT:		 //6
			 //加载DLL
			 /*
			 每当调试进程装载DLL文件时，就生成这个事件。当PE加载器第一次解析出与DLL文件有关的链接时，
			 将收到这一事件。调试进程使用了LoadLibrary时也会发生。每当DLL文件装载到地址空间时，都会调用
			 该调试事件
			 */
			 break;
		 case UNLOAD_DLL_DEBUG_EVENT:	 //7
			 /*
			 每当调试进程使用FreeLibrary函数卸载DLL文件时，就会生成该调试事件。
			 仅当最后一次从进程的地址空间卸载DLL文件时，才出现该调试事件（也就是说
			 DLL文件的使用次数为0时）。
			 */
			 //卸载DLL
			 break;
		 case OUTPUT_DEBUG_STRING_EVENT:  //8
			 /*
			 当调试进程调用DebugOutputString()函数向程序发送消息字符串时，该事件发生。
			 */
			 break;
		 }	

		 bRet = ContinueDebugEvent(CCDbgEvent.dwProcessId, 
			 CCDbgEvent.dwThreadId, dwContinueStatus);   //恢复刚刚由调试事件挂起的线程

		 if (!bRet)
		 {
			 printf("ContinueDebugEvent error!");
			 return 1;
		 }

	 }

	system("pause");
	return 0;
}