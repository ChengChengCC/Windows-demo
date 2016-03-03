#include "stdafx.h"
#include "CHandleException.h"
#include "AVLTree.h"
#include <math.h>

list<CCPointInfo*> g_ptList;                   //�ϵ��б�
c_tree<CCCode>     g_Avl_Tree;
list<CCPointPage*> g_PointPageList;              //�ڴ�ϵ��ҳ���ձ�
list<CCCommand*>   g_UserInputList;              //�����û�����ĺϷ����������
list<CCDllInfo*>   g_DllList;                    //ģ����Ϣ�б�
list<CCResetMemBp*> g_ResetMemBp;              //��Ҫ�ָ����ڴ�ϵ�
list<CCPageInfo*>  g_PageList;                 //��ҳ��Ϣ����


HANDLE			CHandleException::m_hProcess = 0; 
DWORD			CHandleException::m_ProcessId = 0; 
HANDLE			CHandleException::m_hThread = 0;
LPVOID			CHandleException::m_lpOepAddr = 0;
int				CHandleException::m_nOrdPtFlag = 0;   //�ϵ����
DEBUG_EVENT		CHandleException::m_CCDbgEvent = {0};
EXCEPTION_DEBUG_INFO CHandleException::m_DbgInfo = {0};
BOOL			CHandleException::m_bIsStart = TRUE;
CONTEXT         CHandleException::m_Context = {0};
CCPointInfo*   CHandleException::m_pFindPoint = NULL;           //�ҵ��Ķϵ�ָ��
list<CCPointInfo*>::iterator m_itFind;      //�ҵ��Ķϵ��������еĵ�����λ��
LPVOID          CHandleException::m_Eip = 0;                  //���Գ����EIPֵ
BOOL             CHandleException::m_isStepRecordMode = FALSE;     //�Ƿ񵥲���¼ģʽ
int              CHandleException::m_nCount = 0;               //ָ��ִ��ʱ���±�ֵ������
BOOL             CHandleException::m_IsSystemInt3 = TRUE;
LPVOID           CHandleException::m_lpDisAsmAddr = NULL;         //��������ʼ��ַ
BOOL			 CHandleException::m_isUserInputStep = FALSE;
CCCommand       CHandleException::m_UserCmd = {0};
BOOL             CHandleException::m_isNeedResetPoint = FALSE;
int              CHandleException::m_nHardPtNum = 0;     //�Ѿ�����Ӳ���ϵ�����
BOOL             CHandleException::m_isNeedResetHardPoint = FALSE;
int              CHandleException::m_nNeedResetHardPoint = 0;  //�ڵ�������Ҫ�����Ӳ���ϵ�Ĵ���


//ȫ������-�������ձ�
CCCmdNode g_aryCmd[] = {
	ADD_COMMAND("T",    CHandleException::StepInto)
 	ADD_COMMAND("P",    CHandleException::StepOver)
 	ADD_COMMAND("G",    CHandleException::Run)
 	ADD_COMMAND("U",    CHandleException::ShowMulAsmCode)
 	//ADD_COMMAND("D",    CHandleException::ShowData)
 	ADD_COMMAND("R",    CHandleException::ShowRegValue)
 	ADD_COMMAND("BP",   CHandleException::SetOrdPoint)
 	ADD_COMMAND("BPL",  CHandleException::ListOrdPoint)
 	ADD_COMMAND("BPC",  CHandleException::ClearOrdPoint)
 	ADD_COMMAND("BH",   CHandleException::SetHardPoint)
 	ADD_COMMAND("BHL",  CHandleException::ListHardPoint)
 	ADD_COMMAND("BHC",  CHandleException::ClearHardPoint)
 	ADD_COMMAND("BM",   CHandleException::SetMemPoint)
 	ADD_COMMAND("BML",  CHandleException::ListMemPoint) 	
	ADD_COMMAND("BMC",  CHandleException::ClearMemPoint)
	// 	ADD_COMMAND("LS",   CDoException::LoadScript)
	// 	ADD_COMMAND("ES",   CDoException::ExportScript)
	// 	ADD_COMMAND("SR",   CDoException::StepRecord)
	// 	ADD_COMMAND("H",    CDoException::ShowHelp)
	{"", NULL}     //���һ������
};




CHandleException::CHandleException(void)
{


}


CHandleException::~CHandleException(void)
{


}





//�����쳣����������ֵΪ TRUE, �������������쳣������ĵ���״̬Ϊ������
//����ֵΪ FALSE������״̬Ϊ��ʽ��û�д����쳣��
BOOL CHandleException::HandleException()
{
	m_DbgInfo = m_CCDbgEvent.u.Exception;

	//���߳�
	PFNOpenThreadFun MyOpenThread;
	MyOpenThread = (PFNOpenThreadFun)GetProcAddress(LoadLibrary("Kernel32.dll"), 
		"OpenThread");
	CHandleException::m_hThread = MyOpenThread(THREAD_ALL_ACCESS, FALSE,
		m_CCDbgEvent.dwThreadId);
	if (CHandleException::m_hThread == NULL)
	{
		printf("OpenThread failed!");
		return 1;
	}

	//�����쳣�Ŀ��
	switch (m_DbgInfo.ExceptionRecord.ExceptionCode) 
	{
		//�����쳣
	case EXCEPTION_ACCESS_VIOLATION:   //((DWORD   )0xC0000005L)
		return HandleAccessException();
		break;
		//int3�쳣
	case EXCEPTION_BREAKPOINT:         //((DWORD   )0x80000003L)
		return HandleInt3Exception();
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: 
		break;
		//�����Ĵ���
	case EXCEPTION_SINGLE_STEP:         //((DWORD   )0x80000004L)
		/*
		��TFλΪ1ʱ��CPUÿִ����һ��ָ��ͻ����һ�������쳣��#DB��,
		�жϵ������쳣������򣬵����쳣��������Ϊ1����80386��ʼ����Ӳ���ϵ㷢��ʱ
		Ҳ����������쳣������1�ŷ�������
		*/
		return HandleSingleStepException();
		break;
	case DBG_CONTROL_C: 
		break;
		// Handle other exceptions. 
	} 
	return TRUE;
}




BOOL CHandleException::HandleAccessException()//��������쳣����  �ڴ�ϵ�
{
	BOOL                    bRet;
	DWORD                   dwAccessAddr;       //��д��ַ
	DWORD                   dwAccessFlag;       //��д��־
	BOOL                    isExceptionFromMemPoint = FALSE;    //�쳣�Ƿ����ڴ�ϵ���������Ĭ��Ϊ��
	CCPointInfo*            pPointInfo = NULL;                  //���еĶϵ�
	BOOL                    isHitMemPoint = FALSE;

	//EXCEPTION_ACCESS_VIOLATION
	//If this value is zero, the thread attempted to read the inaccessible data. 
	//If this value is 1, the thread attempted to write to an inaccessible address. 
	//If this value is 8, the thread causes a user-mode data execution prevention (DEP) violation.

	dwAccessFlag = m_DbgInfo.ExceptionRecord.ExceptionInformation[0];
	dwAccessAddr = m_DbgInfo.ExceptionRecord.ExceptionInformation[1];
	//���� ���ʵ�ַ �����ϵ�-��ҳ����ȥ����
	//ͬһ���ڴ��ҳ�����ж���ϵ�
	//���û���ڡ��ϵ�-��ҳ���в��ҵ�����˵������쳣���Ƕϵ������
	list<CCPointPage*>::iterator it = g_PointPageList.begin();
	int nSize = g_PointPageList.size();

	//����������ÿ���ڵ㣬��ÿ��ƥ��ġ��ϵ�-��ҳ��¼������ӵ�g_ResetMemBp������
	for ( int i = 0; i < nSize; i++ )
	{
		CCPointPage* pPointPage = *it;
		//����ڡ��ϵ�-��ҳ���в��ҵ�
		//�ٸ��ݶϵ������Ϣ�ж��Ƿ�����û����¶ϵ���Ϣ
		if (pPointPage->dwPageAddr == (dwAccessAddr & 0xfffff000))//�жϴ����쳣�ĵ�ַ�ڲ��ڶϵ����
		{
			CCResetMemBp *p = new CCResetMemBp;
			p->dwAddr = pPointPage->dwPageAddr;
			p->nID = pPointPage->nPtNum;                
			g_ResetMemBp.push_back(p);

			//��ʱ�ָ��ڴ�ҳԭ��������
			BOOL bDoOnce = FALSE;
			if (!bDoOnce)
			{
				//��Щ����ֻ��Ҫִ��һ��
				bDoOnce = TRUE;
				isExceptionFromMemPoint = TRUE;
				TempResumePageProp(pPointPage->dwPageAddr); //��ʱ�ָ�ҳ������
				//���õ������ڵ����н��ϵ����
				UpdateContextFromThread();
				m_Context.EFlags |= TF;
				UpdateContextToThread();
			}

			//���ҵ��ϵ���Ŷ�Ӧ�Ķϵ�
			list<CCPointInfo*>::iterator it2 = g_ptList.begin();
			for ( int j = 0; j < g_ptList.size(); j++ )
			{
				pPointInfo = *it2;
				if (pPointInfo->nPtNum == pPointPage->nPtNum)
				{
					break;
				}
				it2++;
			}

			//���ж��Ƿ�����û����¶ϵ���Ϣ���ϵ����ͺͶϵ㷶Χ�����
			if (isHitMemPoint == FALSE)
			{
				if (dwAccessAddr >= (DWORD)pPointInfo->lpPointAddr && 
					dwAccessAddr < (DWORD)pPointInfo->lpPointAddr +
					pPointInfo->dwPointLen)
				{
					if ( pPointInfo->ptAccess == ACCESS || 
						(pPointInfo->ptAccess == WRITE && dwAccessFlag == 1) )
					{
						isHitMemPoint = TRUE;
						//                             break;
					}
				}
			}
		}
		it++;
	}

	//����쳣�������ڴ�ϵ����������������������
	if (isExceptionFromMemPoint == FALSE)
	{
		return FALSE;
	}

	//��������ڴ�ϵ㣬����ͣ����ʾ�����Ϣ���ȴ��û�����
	if (isHitMemPoint)
	{
		ShowBreakPointInfo(pPointInfo);
		//��ʾ��������
		m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
		ShowAsmCode();
		ShowRegValue(NULL);

		//�ȴ��û�����
		bRet = FALSE;
		while (bRet == FALSE)
		{
			bRet = WaitForUserInput();
		}
	}
	return TRUE;
}




BOOL CHandleException::HandleInt3Exception()//����INT3�쳣
{
	BOOL                    bRet;
	CCPointInfo             tempPointInfo;
	CCPointInfo*            pResultPointInfo = NULL;
	char                    CodeBuf[24] = {0};
	/*
	���˳�ʼ�ϵ㣬���½��̵ĳ�ʼ�߳����Լ����������г�ʼ��ʱ����Ϊ���̳�ʼ����һ�����裬
	ntdll.dll�е�LdrpInitializeProcess�����������ڳ�ʼ���Ľ����Ƿ��ڱ�����״̬�����
	PEB��BeingDebugged�ֶΣ�������ǣ��������DbgBreakPoint()����һ���ϵ��쳣��
	Ŀ�����жϵ�����������Ϊ��ʼ�ϵ㡣
	*/
	//�ȹ���ϵͳ��INT3�ϵ�
	if (m_IsSystemInt3 == TRUE)
	{
		m_IsSystemInt3 = FALSE;
		return TRUE;
	}

	//�����������ͣ��OEP��
	if(m_DbgInfo.ExceptionRecord.ExceptionAddress == m_lpOepAddr
		&& m_bIsStart == TRUE)
	{
		m_bIsStart = FALSE;
		//ö��Ŀ����̵�ģ�飬��д�� g_DllList
		EnumDestMod();
	}

	memset(&tempPointInfo, 0, sizeof(CCPointInfo));
	tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	tempPointInfo.ptType = ORD_POINT;

	if(!FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
	{
		//û���ҵ���Ӧ�ϵ㣬��˵�������û����ߵ������Ķϵ�
		//����FALSE������������������ϵͳ���������쳣
		return FALSE;   
	}
	else    //�ҵ��˶ϵ�
	{
		ShowBreakPointInfo(pResultPointInfo);
		BOOL bRet = WriteProcessMemory(m_hProcess, //д��ԭ�����ֽ�
			m_pFindPoint->lpPointAddr, 
			&(m_pFindPoint->u.chOldByte), 1, NULL);

		if (bRet == FALSE)
		{
			printf("WriteProcessMemory error!\r\n");
			return FALSE;
		}
		//��ȡ��ǰ�̵߳�Context
		UpdateContextFromThread();
		//0xcc ��ǰ��ȥ
		m_Context.Eip--;
		m_Eip = (LPVOID)m_Context.Eip;

		if (m_pFindPoint->isOnlyOne == TRUE)    //��һ���Զϵ�
		{
			g_ptList.remove(m_pFindPoint);
			delete m_pFindPoint;
			m_pFindPoint = nullptr;
		} 

		//
		else //����һ���Զϵ㣬�ͻ����û����õĶϵ㣬
			//��Ҫ���õ����������Խ��������оͻᴥ�������쳣�����������쳣��ȥ����
		{
			//���õ���#define TF 0x100 ��־�Ĵ���TFλ
			m_Context.EFlags |= TF;
			m_isNeedResetPoint = TRUE;   //
		}
	}

	//�����̵߳�Context
	UpdateContextToThread();

	//�Ƿ��ǵ�����¼ģʽ��Trace����
	if (m_isStepRecordMode == TRUE)  
	{
		bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
		if (bRet == FALSE)
		{
			printf("ReadProcessMemory error!");
			return FALSE;
		}
		//��¼ָ��
		RecordCode(m_Context.Eip, CodeBuf);
		return TRUE;
	}

	//��ʾ��������
	m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	ShowAsmCode();
	ShowRegValue(NULL);

	//�ȴ��û�����
	bRet = FALSE;
	while (bRet == FALSE)
	{
		bRet = WaitForUserInput();
	}
	return TRUE;
}


BOOL CHandleException::HandleSingleStepException()//�������쳣
{
	BOOL                    bRet;
	DWORD                   dwDr6 = 0;
	DWORD                   dwDr6Low = 0;
	CCPointInfo             tempPointInfo;
	CCPointInfo*            pResultPointInfo = NULL;
	char                    CodeBuf[24] = {0};
	DWORD                   dwOldProtect;
	DWORD                   dwNoUseProtect;

	UpdateContextFromThread();

	//��Ҫ����CC�ϵ㣬�����û����õ�CC�ϵ㱻�����Ժ�,�ָ���ԭ�����ڴ�����ݣ������˵����쳣
	//���������CC�ϵ����õĵ�������ô����Ҫ��0xcc����д��Ŀ���ַ
	if (m_isNeedResetPoint == TRUE)
	{
		m_isNeedResetPoint = FALSE;
		char chCC = (char)0xcc;
		VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
			1, PAGE_READWRITE, &dwOldProtect);
		bRet = WriteProcessMemory(m_hProcess, m_pFindPoint->lpPointAddr, 
			&chCC, 1, NULL);
		VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
			1, dwOldProtect, &dwNoUseProtect);
		if (bRet == FALSE)
		{
			printf("WriteProcessMemory error!\r\n");
			return FALSE;
		}
	}
// 
	//��Ҫ����Ӳ���ϵ�  
	if (m_isNeedResetHardPoint == TRUE)
	{
		m_Context.Dr7 |= (int)pow((double)4, m_nNeedResetHardPoint);
		UpdateContextToThread();
		m_isNeedResetHardPoint = FALSE;
	}

	dwDr6 = m_Context.Dr6;
	dwDr6Low = dwDr6 & 0xf; //ȡ��4λ���ֱ��ӦDr0~Dr3

	//�������Ӳ���ϵ㴥���ĵ�������Ҫ�û�������ܼ���
	//���⣬�����Ӳ��ִ�жϵ㣬����Ҫ����ʱȡ���ϵ㣬���õ������´��ٻָ��ϵ�
	if (dwDr6Low != 0)  
	{
		ShowHardwareBreakpoint(dwDr6Low);  
		m_nNeedResetHardPoint = log((double)dwDr6Low)/log((double)2)+0.5;//��0.5��Ϊ����������
		//�ж��� dwDr6Low ָ����DRX�Ĵ������Ƿ���ִ�жϵ�
		if((m_Context.Dr7 << (14 - (m_nNeedResetHardPoint * 2))) >> 30 == 0)
		{
			switch (m_nNeedResetHardPoint) //ȡ��Ӳ���ϵ�
			{
			case 0:
				m_Context.Dr7 &= 0xfffffffe;  //1110 ȡ��Dr0
				break;
			case 1:
				m_Context.Dr7 &= 0xfffffffb;  //1011 ȡ��Dr1
				break;
			case 2:
				m_Context.Dr7 &= 0xffffffef;  //1110 1111 ȡ��Dr2
				break;
			case 3:
				m_Context.Dr7 &= 0xffffffbf;  //1011 1111 ȡ��Dr3
				break;
			default:
				printf("Error!\r\n");
			}
			m_Context.EFlags |= TF;  //���õ���
			UpdateContextToThread();
			m_isNeedResetHardPoint = TRUE;
		}

		m_isUserInputStep = TRUE; //�������ֻ��Ϊ���ܹ��ȴ��û�����
	}

	if (m_isUserInputStep == FALSE)
	{
		//�����ڴ�ϵ�
		ResetMemBp();
		return TRUE;
	}

	//���´������û�����Ϊ "T" �����Ӳ���ϵ㴥��ʱִ��
	//����˴���INT3�ϵ㣬����Ҫ����ʱɾ��INT3�ϵ�
	//��������Ϊ�����û����롰T�������Ӳ���ϵ㴥��ʱ���Ե�INT3�ϵ�
	//������һ���ط�ͣ������
	memset(&tempPointInfo, 0, sizeof(CCPointInfo));
	tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	tempPointInfo.ptType = ORD_POINT;
	if (FindPointInList(tempPointInfo, &pResultPointInfo, TRUE)) //��ǰ���µĵ�ַ�Ƿ����CC�ϵ�
	{
		//��һ���Զϵ㣬����Ҫ����ϵ�
		if (pResultPointInfo->isOnlyOne == FALSE)
		{
			m_Context.EFlags |= TF;  //�û����õĶϵ㣬�������õ����쳣
			UpdateContextToThread();
			m_isNeedResetPoint = TRUE;
		}
		else//һ���Զϵ㣬����������ɾ��
		{
			delete[] m_pFindPoint;
			g_ptList.erase(m_itFind);
		}
		//��ʱȥ��0xCC���ָ�ԭ����ָ��
		VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
			1, PAGE_READWRITE, &dwOldProtect);
		bRet = WriteProcessMemory(m_hProcess, m_pFindPoint->lpPointAddr, 
			&(m_pFindPoint->u.chOldByte), 1, NULL);
		if (bRet == FALSE)
		{
			printf("WriteProcessMemory error!\r\n");
			return FALSE;
		}
		VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
			1, dwOldProtect, &dwNoUseProtect);
	}

	m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	m_isUserInputStep = FALSE;

	//��ȡ�߳�Context
	UpdateContextFromThread();

	if (m_isStepRecordMode == FALSE)
	{
		ShowAsmCode();
		ShowRegValue(NULL);
	}

	//�����ڴ�ϵ�
	ResetMemBp();

	//�Ƿ��ǵ�����¼ģʽ  Trace
	if (m_isStepRecordMode == TRUE)
	{
		bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
		if (bRet == FALSE)
		{
			printf("ReadProcessMemory error!\r\n");
			return FALSE;
		}
		//��¼ָ��
		RecordCode(m_Context.Eip, CodeBuf);
		return TRUE;
	}

	bRet = FALSE;
	while (bRet == FALSE)
	{
		bRet = WaitForUserInput();
	}
	return TRUE;
}

//��ʾ�Ĵ�������
BOOL CHandleException::ShowRegValue(CCCommand* pCmd)
{
	printf("-----------------------------------------------------------------------------\r\n");
	printf("EAX=%p EBX=%p ECX=%p EDX=%p ESI=%p EDI=%p\r\n", 
		m_Context.Eax, m_Context.Ebx, m_Context.Ecx, m_Context.Edx,
		m_Context.Esi, m_Context.Edi);
	printf("EIP=%p ESP=%p EBP=%p                OF DF IF SF ZF AF PF CF\r\n", 
		m_Context.Eip, m_Context.Esp, m_Context.Ebp);
	printf("CS=%0.4X SS=%0.4X DS=%0.4X ES=%0.4X FS=%0.4X GS=%0.4X",
		m_Context.SegCs, m_Context.SegSs, m_Context.SegDs, m_Context.SegEs,
		m_Context.SegFs, m_Context.SegGs);
	printf("       %d  %d  %d  %d  %d  %d  %d  %d\r\n", 
		(bool)(m_Context.EFlags & 0x0800),		//OF
		(bool)(m_Context.EFlags & 0x0400),		//DF
		(bool)(m_Context.EFlags & 0x0200),		//IF
		(bool)(m_Context.EFlags & 0x0080),		//SF
		(bool)(m_Context.EFlags & 0x0040),		//ZF
		(bool)(m_Context.EFlags & 0x0010),		//AF
		(bool)(m_Context.EFlags & 0x0004),		//PF
		(bool)(m_Context.EFlags & 0x0001)		//CF
		);
	printf("-----------------------------------------------------------------------------\r\n");
	return FALSE;
}

//ö��ģ��
void CHandleException::EnumDestMod()
{
	HANDLE hmodule = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_ProcessId);

	if (hmodule == INVALID_HANDLE_VALUE)
	{
		printf("CreateToolhelp32Snapshot error!\r\n");
	}

	MODULEENTRY32 me;
	me.dwSize = sizeof(MODULEENTRY32);

	if (::Module32First(hmodule, &me))
	{
		do 
		{
			CCDllInfo* pDllInfo = new CCDllInfo;
			pDllInfo->dwDllAddr = (DWORD)me.modBaseAddr;
			pDllInfo->dwModSize = me.modBaseSize;
			strcpy_s(pDllInfo->szDllName, me.szModule);
			g_DllList.push_back(pDllInfo);
		} while (::Module32Next(hmodule, &me));
	}

	CloseHandle(hmodule);

}





BOOL CHandleException::FindPointInList(IN CCPointInfo PointInfo, 
	OUT CCPointInfo** ppResultPointInfo,
	BOOL isNeedSave)
/*
�ڶϵ��б��в����Ƿ���ƥ��PointInfo�ϵ���Ϣ�Ķϵ㡣
ֻƥ�� �ϵ���ʼ��ַ���ϵ����ͣ��ϵ�������͡�
����isNeedSave����ʾ�Ƿ���Ҫ����
����ֵΪTRUE��ʾ�ҵ���FALSE��ʾû���ҵ���
�ҵ��Ķϵ�ָ�룬���� ppResultPointInfo ������
*/
{
	list<CCPointInfo*>::iterator it = g_ptList.begin();
	for ( int i = 0; i < g_ptList.size(); i++ )
	{
		CCPointInfo* tempPointInfo = *it;
		if (tempPointInfo->lpPointAddr == PointInfo.lpPointAddr &&   //��ַƥ��
			tempPointInfo->ptType == PointInfo.ptType &&             //����ƥ�� 
			tempPointInfo->ptAccess == PointInfo.ptAccess)
		{
			*ppResultPointInfo = tempPointInfo;
			if (isNeedSave == TRUE)
			{
// 				m_itFind = it;   ������
 				m_pFindPoint = tempPointInfo;
			}
			return TRUE;
		}
		it++;
	}

	return FALSE;

}



BOOL CHandleException::DeletePointInList(int nPtNum, BOOL isNeedResetProtect)
{
	//�ȸ�������ҵ���Ӧ�ϵ�
	list<CCPointInfo*>::iterator it = g_ptList.begin();
	for ( int i = 0; i < g_ptList.size(); i++ )
	{
		CCPointInfo* pPointInfo = *it;
		if (pPointInfo->nPtNum == nPtNum)//�ҵ���
		{
			//���ж��ǲ����ڴ�ϵ㣬������Ǿ��Ǵ����
			if (pPointInfo->ptType != MEM_POINT)
			{
				printf("Only allow memony breakpoint reach here!\r\n");
				return FALSE;
			}

			//���ڴ�ϵ㣬Ҫɾ���ϵ���� �� �ϵ�-��ҳ����
			delete [] pPointInfo;
			g_ptList.erase(it);

			//ɾ���ϵ�-��ҳ����
			//DeleteRecordInPointPageList(nPtNum, isNeedResetProtect);   ������

			return TRUE; 
		}
		it++;
	}
	return FALSE;
}




void CHandleException::ShowBreakPointInfo(CCPointInfo *pPoint)
{
	//����CC�ϵ㲻��ʾ���ǵ������Լ��µ�
	if (pPoint->isOnlyOne == TRUE && pPoint->ptType == ORD_POINT)
	{
		return;
	}
	printf("***********************************************************\r\n");
	printf("Hit the breakpoint: \r\n");
	printf("breakpoint's ID: %d. ", pPoint->nPtNum);
	if (pPoint->ptType == ORD_POINT)
	{
		printf("Ordinary breakpoint(INT3) at 0x%p.\r\n", pPoint->lpPointAddr);
	} 
	else if(pPoint->ptType == MEM_POINT)
	{
		printf("       Memory breakpoint at 0x%p.\r\n", pPoint->lpPointAddr);
		if (pPoint->ptAccess == ACCESS)
		{
			printf("Breakpoint type is ACCESS. ");
		} 
		else if (pPoint->ptAccess == WRITE)
		{
			printf("Breakpoint type is WRITE. ");
		}
		else
		{
			printf("error!");//�������������
		}
		printf("Breakpoint length is %d.\r\n", pPoint->dwPointLen);
	}
	printf("***********************************************************\r\n");
}




void CHandleException::RecordCode(int nEip, char *pCodeBuf)  //OD ��Trace����
{
// 	CCCode* pCCCode = AddInAvlTree(nEip, pCodeBuf);
// 	DWORD  dwBytesWritten, dwPos; 
// 	char   buff[400]; 
// 
// 	//�ó����������
// 	//������Ҫ���� pCCCode �ṹ���ж���Ҫ�������뻹�ǵ�������
// 	ContinueRun(pCCCode);
// 
// 	//��ʾ����
// 	if (m_isShowCode == TRUE)
// 	{
// 		printf("%06d %06d %p    ", pCCCode->m_nID, pCCCode->m_nCount, 
// 			pCCCode->m_nEip);
// 		for (int i = 0; i < pCCCode->m_nCodeLen; i++)
// 		{
// 			printf("%s", &hexVale[pCCCode->m_OpCode[i]]);
// 		}
// 		printf("%s%s", "                          "+pCCCode->m_nCodeLen*2,
// 			pCCCode->m_AsmCode);
// 		if (pCCCode->m_chApiName[0] != 0)
// 		{
// 			printf("(%s)", pCCCode->m_chApiName);
// 		}
// 		printf("\r\n");
// 	}
// 
// 	if (pCCCode->m_nCount == 1)
// 	{
// 		wsprintf(buff, "%06d %p    ", pCCCode->m_nID, pCCCode->m_nEip);
// 
// 		for (int i = 0; i < pCCCode->m_nCodeLen; i++)
// 		{
// 			strcat(buff, hexVale[pCCCode->m_OpCode[i]]);
// 		}
// 
// 		strcat(buff, "                          "+pCCCode->m_nCodeLen*2);
// 		strcat(buff, pCCCode->m_AsmCode);
// 
// 		if (pCCCode->m_chApiName[0] != 0)
// 		{
// 			strcat(buff, "(");
// 			strcat(buff, pCCCode->m_chApiName);
// 			strcat(buff, ")\r\n");
// 		}
// 		else
// 		{
// 			strcat(buff, "\r\n");
// 		}
// 
// 		dwPos = SetFilePointer(m_hAppend, 0, NULL, FILE_END); 
// 		WriteFile(m_hAppend, buff, strlen(buff), &dwBytesWritten, NULL); 
// 	}
}



CCCode* CHandleException::AddInAvlTree(int nEip, char *pCodeBuf)
{
	CCCode stCode;
	stCode.m_nEip = nEip;
	memcpy(stCode.m_OpCode, pCodeBuf, 24);
	node<CCCode>* pNode = g_Avl_Tree.find_data(stCode);
	t_disasm da;

	//û���ҵ�
	if (pNode == NULL)
	{
		int nLen = Disasm(pCodeBuf, 24, 0, &da, DISASM_CODE, nEip);
		stCode.m_nCodeLen = nLen;
		strcpy_s(stCode.m_AsmCode, da.result);
		stCode.m_nID = ++m_nCount;
		stCode.m_nCount = 1;
		pNode = g_Avl_Tree.balance_sort_insert(stCode);
		return &pNode->data;
	}

	//�ҵ�
	pNode->data.m_nCount++;
	return &pNode->data;
}




//�ڶϵ�-��ҳ���в��Ҽ�¼
BOOL CHandleException::FindRecordInPointPageList(DWORD dwPageAddr)
{
	list<CCPointPage*>::iterator it = g_PointPageList.begin();

	for ( int i = 0; i < g_PointPageList.size(); i++ )
	{
		CCPointPage* pPointPage = *it;
		if (pPointPage->dwPageAddr == dwPageAddr)
		{
			return TRUE;
		}
		it++;
	}
	return FALSE;
}




BOOL  CHandleException::StepInto(CCCommand* pCmd)  //��������T
{
	UpdateContextFromThread();
	//���õ���
	m_Context.EFlags |= TF;   //���ñ�־�Ĵ���Ϊ����ģʽ��CPUÿִ��һ��ָ����������쳣
	m_isUserInputStep = TRUE;
	UpdateContextToThread();    
	return TRUE;
}




//��������
BOOL CHandleException::StepOver(CCCommand* pCmd)
{
	char            CodeBuf[20] = {0};
	t_disasm        da;
	int             nCodelen;
	BOOL            bRet;
	BOOL            isNeedResetFirstPage = FALSE;
	BOOL            isNeedResetSecondPage = FALSE;
	DWORD           dwTempProtect1;
	DWORD           dwTempProtect2;
	DWORD           dwOldProtect;

	//�鿴Ҫ��������ĵ�ַ���ڵ��ڴ��ҳ�Ƿ��Ѿ����ڴ�ϵ�
	//����У����޸��ڴ�����ҳΪ�ɶ�������֮���ٸ�Ϊԭ������
	//ע�⣬�����ڴ���ܿ��ҳ
	//��10λ����ҳĿ¼������      0000 0000 00
	//�м�10λ����ҳ�������	  01 0111 1111
	//��12λ����ҳƫ��		    1111 0100 0100
	if (FindRecordInPointPageList((DWORD)m_lpDisAsmAddr & 0xfffff000))
	{
		VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
			1, PAGE_READONLY, &dwTempProtect1);
		isNeedResetFirstPage = TRUE;
	}

	if (FindRecordInPointPageList(((DWORD)m_lpDisAsmAddr + 20) & 0xfffff000))
	{
		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
			1, PAGE_READONLY, &dwTempProtect2);
		isNeedResetSecondPage = TRUE;
	}

	bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 20, NULL);

	//����֮������ϵ㣬����Ҫע�⣬���� m_lpDisAsmAddr �� m_lpDisAsmAddr+20
	//������ͬһ����ҳ�ϣ��� SecondPage �� FirstPage��ͬһ����ҳ
	//�����Ȼָ� SecondPage����ָ� FirstPage
	if (isNeedResetSecondPage)
	{
		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
			1, dwTempProtect2, &dwOldProtect);
		isNeedResetSecondPage = FALSE;
	}

	if (isNeedResetFirstPage)
	{
		VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
			1, dwTempProtect1, &dwOldProtect);
		isNeedResetFirstPage = FALSE;
	}

	if (bRet == FALSE)
	{
		printf("ReadProcessMemory error!\r\n");
		return FALSE;
	}

	nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//���÷��������
	(da.result)[4] = '\0';
	if (strcmp(da.result, "CALL") == 0)
	{
		char buf[20] = {0};
		sprintf_s(buf, "%p", (int)m_Context.Eip + nCodelen);
		strcpy_s(m_UserCmd.chParam1, buf);
	//	Run(&m_UserCmd);
	}
	else
	{
		StepInto(NULL);
	}

	return TRUE;
}



BOOL CHandleException::Run(CCCommand* pCmd)  //����TRUE��ֱ������
{
	LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);
	if (lpAddr != (LPVOID)0)
	{
		wsprintf(pCmd->chParam2, "%s", "once");
		SetOrdPoint(pCmd);
	}
	return TRUE;
}



//��ʱ�ָ��ڴ�����ҳ������
void CHandleException::TempResumePageProp(DWORD dwPageAddr)
{
	list<CCPageInfo*>::iterator it = g_PageList.begin();

	for ( int i = 0; i < g_PageList.size(); i++ )
	{
		CCPageInfo* pPageInfo = *it;
		if (pPageInfo->dwPageAddr == dwPageAddr)
		{
			DWORD dwTempProtect;
			VirtualProtectEx(m_hProcess, (LPVOID)pPageInfo->dwPageAddr,
				1, pPageInfo->dwOldProtect, &dwTempProtect);
			return;
		}
		it++;
	}
}