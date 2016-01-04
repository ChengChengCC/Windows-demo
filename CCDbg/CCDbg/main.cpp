// CCDbg.cpp : �������̨Ӧ�ó������ڵ㡣
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

	//���������Խ���
	char                szDirectoryBuf[MAXBYTE] = {0};
	STARTUPINFO         StartupInfo;
	PROCESS_INFORMATION pInfo;

	GetStartupInfo(&StartupInfo);
	BOOL isCreateSucess =  CreateProcess(szFileName, NULL, NULL, NULL, TRUE,
		DEBUG_PROCESS || DEBUG_ONLY_THIS_PROCESS, NULL,
		NULL, &StartupInfo, &pInfo);  //�������Խ���
	if (isCreateSucess == FALSE)
	{
		printf("Create debug process failed!\r\n");
		return 1;
	}
	//�����˵�
	ShowHelp(NULL);

	//�������ѭ��
	BOOL        isContinue = TRUE;
	DEBUG_EVENT CCDbgEvent = {0};
	DWORD       dwContinueStatus;
	BOOL        bRet;
	while (isContinue)
	 {
		 dwContinueStatus = DBG_CONTINUE;
		 bRet = WaitForDebugEvent(&CCDbgEvent, INFINITE); //�ȴ������Խ��̷��������¼�
		 if (!bRet)
		 {
			 printf("WaitForDebugEvent error!");
			 return 1;
		 }
		CHandleException::m_CCDbgEvent = CCDbgEvent;
		switch (CCDbgEvent.dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:     //1
			//�����쳣
			/*
			�ڵ��Թ��̳������쳣���ͻ�����õ����¼�
			*/
			bRet = CHandleException::HandleException();
			if (bRet == FALSE)
			{
		    	 dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;//������δ������ϵͳ�������зַ�
			} 
			break;
		case CREATE_THREAD_DEBUG_EVENT:   //2
	     	 //�����߳�
			/*
			�ڵ��Խ����д���һ���µĽ��̻��ߵ�������ʼ�����Ѽ���Ľ���ʱ��
			�ͻ�������������¼���Ҫע����ǵ����Ե����̱߳�����ʱ�����յĸ�֪ͨ
			*/
	    	 break;
		 case CREATE_PROCESS_DEBUG_EVENT:  //3
			 //��������
			 /*
			 ���̱������������ԵĽ��̸ձ���������δ���У����������ʼ�����Ѽ���Ľ���ʱ��
			 �ͻ��������¼�
			 */
			 //��OEP������һ���Զϵ�
			 {
				 CHandleException::m_ProcessId = CCDbgEvent.dwProcessId;   //����ID
				 CHandleException::m_hProcess = CCDbgEvent.u.CreateProcessInfo.hProcess;   //���̾��
				 CHandleException::m_lpOepAddr = CCDbgEvent.u.CreateProcessInfo.lpStartAddress; //OEP
				 CCCommand CCCmd = {0};
				 wsprintf(CCCmd.chParam1, "%p", 
					 CCDbgEvent.u.CreateProcessInfo.lpStartAddress);
				 wsprintf(CCCmd.chParam2, "%s", "once");//һ���Զϵ�
					 //�ڽ��̵�OEP����һ���Զϵ�,�ڼ���ʱ����
				 CHandleException::SetOrdPoint(&CCCmd);
					 //�رվ������ֹ��Դй¶
				 bRet = CloseHandle(CCDbgEvent.u.CreateProcessInfo.hThread);
				 if (!bRet)
				 {
					 printf("CloseHandle error!");
					 return 1;
				 }
			 }
				 break;
		 case EXIT_THREAD_DEBUG_EVENT:  //4
			 //�˳��߳�
			 /*
			 ���Ե��߳��˳�ʱ�¼����������Ե����߳��˳�ʱ
			 �����յ���֪ͨ
			 */
			 break;
		 case EXIT_PROCESS_DEBUG_EVENT:  //5
			 //�˳�����
			 /*
			 ���˳�������̵����һ���߳�ʱ����������¼���
			 */
			 isContinue = FALSE;
			 break;
		 case LOAD_DLL_DEBUG_EVENT:		 //6
			 //����DLL
			 /*
			 ÿ�����Խ���װ��DLL�ļ�ʱ������������¼�����PE��������һ�ν�������DLL�ļ��йص�����ʱ��
			 ���յ���һ�¼������Խ���ʹ����LoadLibraryʱҲ�ᷢ����ÿ��DLL�ļ�װ�ص���ַ�ռ�ʱ���������
			 �õ����¼�
			 */
			 break;
		 case UNLOAD_DLL_DEBUG_EVENT:	 //7
			 /*
			 ÿ�����Խ���ʹ��FreeLibrary����ж��DLL�ļ�ʱ���ͻ����ɸõ����¼���
			 �������һ�δӽ��̵ĵ�ַ�ռ�ж��DLL�ļ�ʱ���ų��ָõ����¼���Ҳ����˵
			 DLL�ļ���ʹ�ô���Ϊ0ʱ����
			 */
			 //ж��DLL
			 break;
		 case OUTPUT_DEBUG_STRING_EVENT:  //8
			 /*
			 �����Խ��̵���DebugOutputString()�������������Ϣ�ַ���ʱ�����¼�������
			 */
			 break;
		 }	

		 bRet = ContinueDebugEvent(CCDbgEvent.dwProcessId, 
			 CCDbgEvent.dwThreadId, dwContinueStatus);   //�ָ��ո��ɵ����¼�������߳�

		 if (!bRet)
		 {
			 printf("ContinueDebugEvent error!");
			 return 1;
		 }

	 }

	system("pause");
	return 0;
}