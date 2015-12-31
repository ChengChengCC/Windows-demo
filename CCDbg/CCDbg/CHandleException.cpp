#include "stdafx.h"
#include "CHandleException.h"
#include "AVLTree.h"


list<stuPointInfo*> g_ptList;                   //�ϵ��б�
c_tree<StuCode> g_Avl_Tree;
list<stuPointPage*> g_PointPageList;            //�ڴ�ϵ��ҳ���ձ�
list<StuCommand*> g_UserInputList;              //�����û�����ĺϷ����������

HANDLE			CHandleException::m_hProcess = 0; 
DWORD			CHandleException::m_ProcessId = 0; 
HANDLE			CHandleException::m_hThread = 0;
LPVOID			CHandleException::m_lpOepAddr = 0;
int				CHandleException::m_nOrdPtFlag = 0;   //�ϵ����
DEBUG_EVENT		CHandleException::m_stuDbgEvent = {0};
EXCEPTION_DEBUG_INFO CHandleException::m_DbgInfo = {0};
BOOL			CHandleException::m_bIsStart = TRUE;
CONTEXT         CHandleException::m_Context = {0};
stuPointInfo*   CHandleException::m_pFindPoint = NULL;           //�ҵ��Ķϵ�ָ��
list<stuPointInfo*>::iterator m_itFind;  //�ҵ��Ķϵ��������еĵ�����λ��
LPVOID          CHandleException::m_Eip = 0;                  //���Գ����EIPֵ
BOOL             CHandleException::m_isStepRecordMode = FALSE;     //�Ƿ񵥲���¼ģʽ
int              CHandleException::m_nCount = 0;               //ָ��ִ��ʱ���±�ֵ������
BOOL             CHandleException::m_IsSystemInt3 = TRUE;
LPVOID           CHandleException::m_lpDisAsmAddr = NULL;         //��������ʼ��ַ
BOOL			 CHandleException::m_isUserInputStep = FALSE;
StuCommand       CHandleException::m_UserCmd = {0};





//ȫ������-�������ձ�
StuCmdNode g_aryCmd[] = {
	ADD_COMMAND("T",    CHandleException::StepInto)
 	ADD_COMMAND("P",    CHandleException::StepOver)
	// 	ADD_COMMAND("G",    CDoException::Run)
	// 	ADD_COMMAND("U",    CDoException::ShowMulAsmCode)
	// 	ADD_COMMAND("D",    CDoException::ShowData)
	// 	ADD_COMMAND("R",    CDoException::ShowRegValue)
	// 	ADD_COMMAND("BP",   CDoException::SetOrdPoint)
	// 	ADD_COMMAND("BPL",  CDoException::ListOrdPoint)
	// 	ADD_COMMAND("BPC",  CDoException::ClearOrdPoint)
	// 	ADD_COMMAND("BH",   CDoException::SetHardPoint)
	// 	ADD_COMMAND("BHL",  CDoException::ListHardPoint)
	// 	ADD_COMMAND("BHC",  CDoException::ClearHardPoint)
	// 	ADD_COMMAND("BM",   CDoException::SetMemPoint)
	// 	ADD_COMMAND("BML",  CDoException::ListMemPoint)
	// 	ADD_COMMAND("BMC",  CDoException::ClearMemPoint)
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



//����һ��ϵ�
BOOL CHandleException::SetOrdPoint(StuCommand* pCmd)
{
	BOOL    bRet;
	LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);

	if (lpAddr == 0)
	{
		printf("Need valid parameter!\r\n");
		return FALSE;
	}

	//�ڶϵ��б��в����Ƿ��Ѿ����ڴ˴���һ��ϵ�
	stuPointInfo tempPointInfo;
// 	stuPointInfo* pResultPointInfo = NULL;
	memset(&tempPointInfo, 0, sizeof(stuPointInfo));
	tempPointInfo.lpPointAddr = lpAddr;
	tempPointInfo.ptType = ORD_POINT;
	if (stricmp(pCmd->chParam2, "once") == 0)
	{
		tempPointInfo.isOnlyOne = TRUE;
	} 
	else
	{
		tempPointInfo.isOnlyOne = FALSE;
	}
 
// 	if (FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
// 	{
// 		if (tempPointInfo.isOnlyOne == FALSE)//���õ��Ƿ�һ���Զϵ�
// 		{
// 			if (pResultPointInfo->isOnlyOne == FALSE)//���ҵ����Ƿ�һ���Զϵ�
// 			{
// 				printf("This Ordinary BreakPoint is already exist!\r\n");
// 			} 
// 			else
// 			{
// 				pResultPointInfo->isOnlyOne = FALSE;
// 			}
// 		}
// 		return FALSE;
// 	}    

	//���õ�ַ���׵�ַΪ0xcc
	char chOld;
	char chCC = 0xcc;
	DWORD dwOldProtect;
	VirtualProtectEx(m_hProcess, lpAddr, 1, PAGE_READWRITE, &dwOldProtect);
	bRet = ReadProcessMemory(m_hProcess, lpAddr, &chOld, 1, NULL);
	if (bRet == FALSE)
	{
		printf("ReadProcessMemory error! may be is not a valid memory address!\r\n");
		return FALSE;
	}

	bRet = WriteProcessMemory(m_hProcess, lpAddr, &chCC, 1, NULL);
	if (bRet == FALSE)
	{
		printf("WriteProcessMemory error!\r\n");
		return FALSE;
	}

	VirtualProtectEx(m_hProcess, lpAddr, 1, dwOldProtect, &dwOldProtect);

	//�������õĶϵ���뵽�ϵ��б���
	stuPointInfo* NewPointInfo = new stuPointInfo;
	memset(NewPointInfo, 0, sizeof(stuPointInfo));
	NewPointInfo->nPtNum = m_nOrdPtFlag;
	m_nOrdPtFlag++;
	NewPointInfo->ptType = ORD_POINT;
	NewPointInfo->lpPointAddr = lpAddr;
	NewPointInfo->u.chOldByte = chOld;
	NewPointInfo->isOnlyOne = tempPointInfo.isOnlyOne;
	g_ptList.push_back(NewPointInfo);
// 	if (m_isStepRecordMode == FALSE && m_nOrdPtFlag != 1)
// 	{
// 		printf("***Set Ordinary breakpoint(INT3) success!***\r\n");
// 	}

	return FALSE;
}


//�����쳣����������ֵΪ TRUE, �������������쳣������ĵ���״̬Ϊ������
//����ֵΪ FALSE������״̬Ϊ��ʽ��û�д����쳣��
BOOL CHandleException::HandleException()
{
	m_DbgInfo = m_stuDbgEvent.u.Exception;

	//���߳�
	PFNOpenThreadFun MyOpenThread;
	MyOpenThread = (PFNOpenThreadFun)GetProcAddress(LoadLibrary("Kernel32.dll"), 
		"OpenThread");
	CHandleException::m_hThread = MyOpenThread(THREAD_ALL_ACCESS, FALSE,
		m_stuDbgEvent.dwThreadId);
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
	//	return DoAccessException();
		break;
		//int3�쳣
	case EXCEPTION_BREAKPOINT:         //((DWORD   )0x80000003L)
		return HandleInt3Exception();
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: 
		break;
		//�����Ĵ���
	case EXCEPTION_SINGLE_STEP:         //((DWORD   )0x80000004L)  
		return HandleSingleStepException();
		break;
	case DBG_CONTROL_C: 
		break;
		// Handle other exceptions. 
	} 
	return TRUE;
}





BOOL CHandleException::HandleInt3Exception()//����INT3�쳣
{
	BOOL                    bRet;
	stuPointInfo            tempPointInfo;
	stuPointInfo*           pResultPointInfo = NULL;
	char                    CodeBuf[24] = {0};

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

	memset(&tempPointInfo, 0, sizeof(stuPointInfo));
	tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	tempPointInfo.ptType = ORD_POINT;

	if(!FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
	{
		//û���ҵ���Ӧ�ϵ㣬��˵�������û��µĶ϶�
		//����FALSE������������������ϵͳ���������쳣
		return FALSE;   
	}
	else    //�ҵ��˶ϵ�
	{
		ShowBreakPointInfo(pResultPointInfo);
		BOOL bRet = WriteProcessMemory(m_hProcess, 
			m_pFindPoint->lpPointAddr, 
			&(m_pFindPoint->u.chOldByte), 1, NULL);

		if (bRet == FALSE)
		{
			printf("WriteProcessMemory error!\r\n");
			return FALSE;
		}
		//��ȡ����
		UpdateContextFromThread();
		////?????
		m_Context.Eip--;
		m_Eip = (LPVOID)m_Context.Eip;

		if (m_pFindPoint->isOnlyOne == TRUE)    //��һ���Զϵ�
		{
// 			delete m_pFindPoint;
// 			g_ptList.erase(m_itFind);     //erase ????

			g_ptList.remove(m_pFindPoint);
			delete m_pFindPoint;
			m_pFindPoint = nullptr;
		} 

		//???????
// 		else //����һ���Զϵ㣬��Ҫ���õ�������������ȥ����ϵ�
// 		{
// 			//���õ���#define TF 0x100
// 			m_Context.EFlags |= TF;
// 	//		m_isNeedResetPoint = TRUE;   //????
// 		}
	}

	//�ָ�����
	UpdateContextToThread();

	//�Ƿ��ǵ�����¼ģʽ
// 	if (m_isStepRecordMode == TRUE)
// 	{
// 		bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
// 		if (bRet == FALSE)
// 		{
// 			printf("ReadProcessMemory error!");
// 			return FALSE;
// 		}
// 		//��¼ָ��
// 		RecordCode(m_Context.Eip, CodeBuf);
// 		return TRUE;
// 	}

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
	stuPointInfo            tempPointInfo;
	stuPointInfo*           pResultPointInfo = NULL;
	char                    CodeBuf[24] = {0};
	DWORD                   dwOldProtect;
	DWORD                   dwNoUseProtect;

	UpdateContextFromThread();

	//��Ҫ����INT3�ϵ�
// 	if (m_isNeedResetPoint == TRUE)
// 	{
// 		m_isNeedResetPoint = FALSE;
// 		char chCC = (char)0xcc;
// 		VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
// 			1, PAGE_READWRITE, &dwOldProtect);
// 		bRet = WriteProcessMemory(m_hProcess, m_pFindPoint->lpPointAddr, 
// 			&chCC, 1, NULL);
// 		VirtualProtectEx(m_hProcess, m_pFindPoint->lpPointAddr,
// 			1, dwOldProtect, &dwNoUseProtect);
// 		if (bRet == FALSE)
// 		{
// 			printf("WriteProcessMemory error!\r\n");
// 			return FALSE;
// 		}
// 	}
// 
// 	//��Ҫ����Ӳ���ϵ�
// 	if (m_isNeedResetHardPoint == TRUE)
// 	{
// 		m_Context.Dr7 |= (int)pow(4, m_nNeedResetHardPoint);
// 		UpdateContextToThread();
// 		m_isNeedResetHardPoint = FALSE;
// 	}
// 
// 	dwDr6 = m_Context.Dr6;
// 	dwDr6Low = dwDr6 & 0xf; //ȡ��4λ
// 
// 	//�������Ӳ���ϵ㴥���ĵ�������Ҫ�û�������ܼ���
// 	//���⣬�����Ӳ��ִ�жϵ㣬����Ҫ����ʱȡ���ϵ㣬���õ������´��ٻָ��ϵ�
// 	if (dwDr6Low != 0)
// 	{
// 		ShowHardwareBreakpoint(dwDr6Low);
// 		m_nNeedResetHardPoint = log(dwDr6Low)/log(2)+0.5;//��0.5��Ϊ����������
// 		//�ж��� dwDr6Low ָ����DRX�Ĵ������Ƿ���ִ�жϵ�
// 		if((m_Context.Dr7 << (14 - (m_nNeedResetHardPoint * 2))) >> 30 == 0)
// 		{
// 			switch (m_nNeedResetHardPoint)
// 			{
// 			case 0:
// 				m_Context.Dr7 &= 0xfffffffe;
// 				break;
// 			case 1:
// 				m_Context.Dr7 &= 0xfffffffb;
// 				break;
// 			case 2:
// 				m_Context.Dr7 &= 0xffffffef;
// 				break;
// 			case 3:
// 				m_Context.Dr7 &= 0xffffffbf;
// 				break;
// 			default:
// 				printf("Error!\r\n");
// 			}
// 			m_Context.EFlags |= TF;
// 			UpdateContextToThread();
// 			m_isNeedResetHardPoint = TRUE;
// 		}
// 
// 		m_isUserInputStep = TRUE; //�������ֻ��Ϊ���ܹ��ȴ��û�����
// 	}
// 
// 	if (m_isUserInputStep == FALSE)
// 	{
// 		//�����ڴ�ϵ�
// 		ResetMemBp();
// 		return TRUE;
// 	}

	//���´������û�����Ϊ "T" �����Ӳ���ϵ㴥��ʱִ��
	//����˴���INT3�ϵ㣬����Ҫ����ʱɾ��INT3�ϵ�
	//��������Ϊ�����û����롰T�������Ӳ���ϵ㴥��ʱ���Ե�INT3�ϵ�
	//������һ���ط�ͣ������
	memset(&tempPointInfo, 0, sizeof(stuPointInfo));
	tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	tempPointInfo.ptType = ORD_POINT;

	if (FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
	{
		//��һ���Զϵ㣬����Ҫ����ϵ�
		if (pResultPointInfo->isOnlyOne == FALSE)
		{
			m_Context.EFlags |= TF;
			UpdateContextToThread();
	//		m_isNeedResetPoint = TRUE;
		}
		else//һ���Զϵ㣬����������ɾ��
		{
			delete[] m_pFindPoint;
			g_ptList.erase(m_itFind);
		}
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

	//����m_ContextΪ���ڵĻ���ֵ
	UpdateContextFromThread();

	if (m_isStepRecordMode == FALSE)
	{
		ShowAsmCode();
		ShowRegValue(NULL);
	}

	//�����ڴ�ϵ�
//	ResetMemBp();

	//�Ƿ��ǵ�����¼ģʽ
	if (m_isStepRecordMode == TRUE)
	{
		bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
		if (bRet == FALSE)
		{
			printf("ReadProcessMemory error!\r\n");
			return FALSE;
		}
		//��¼ָ��
//		RecordCode(m_Context.Eip, CodeBuf);
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
BOOL CHandleException::ShowRegValue(StuCommand* pCmd)
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
// 	HANDLE hmodule = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_ProcessId);
// 
// 	if (hmodule == INVALID_HANDLE_VALUE)
// 	{
// 		printf("CreateToolhelp32Snapshot error!\r\n");
// 	}
// 
// 	MODULEENTRY32 me;
// 	me.dwSize = sizeof(MODULEENTRY32);
// 
// 	if (::Module32First(hmodule, &me))
// 	{
// 		do 
// 		{
// 			stuDllInfo* pDllInfo = new stuDllInfo;
// 			pDllInfo->dwDllAddr = (DWORD)me.modBaseAddr;
// 			pDllInfo->dwModSize = me.modBaseSize;
// 			strcpy(pDllInfo->szDllName, me.szModule);
// 			g_DllList.push_back(pDllInfo);
// 		} while (::Module32Next(hmodule, &me));
// 	}
// 
// 	CloseHandle(hmodule);

}




//�ڶϵ��б��в����Ƿ���ƥ��PointInfo�ϵ���Ϣ�Ķϵ㡣
//ֻƥ�� �ϵ���ʼ��ַ���ϵ����ͣ��ϵ�������͡�
//����isNeedSave����ʾ�Ƿ���Ҫ����
//����ֵΪTRUE��ʾ�ҵ���FALSE��ʾû���ҵ���
//�ҵ��Ķϵ�ָ�룬���� ppResultPointInfo ������
BOOL CHandleException::FindPointInList(IN stuPointInfo PointInfo, 
	OUT stuPointInfo** ppResultPointInfo,
	BOOL isNeedSave)
{
	list<stuPointInfo*>::iterator it = g_ptList.begin();

	for ( int i = 0; i < g_ptList.size(); i++ )
	{
		stuPointInfo* tempPointInfo = *it;
		if (tempPointInfo->lpPointAddr == PointInfo.lpPointAddr && 
			tempPointInfo->ptType == PointInfo.ptType &&
			tempPointInfo->ptAccess == PointInfo.ptAccess)
		{
			*ppResultPointInfo = tempPointInfo;
			if (isNeedSave == TRUE)
			{
// 				m_itFind = it;
 				m_pFindPoint = tempPointInfo;
			}
			return TRUE;
		}
		it++;
	}

	return FALSE;

}




void CHandleException::ShowBreakPointInfo(stuPointInfo *pPoint)
{
	//����CC�ϵ㲻��ʾ
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




// void CHandleException::RecordCode(int nEip, char *pCodeBuf)
// {
// 	StuCode* pstuCode = AddInAvlTree(nEip, pCodeBuf);
// 	DWORD  dwBytesWritten, dwPos; 
// 	char   buff[400]; 
// 
// 	//�ó����������
// 	//������Ҫ���� pstuCode �ṹ���ж���Ҫ�������뻹�ǵ�������
// 	ContinueRun(pstuCode);
// 
// 	//��ʾ����
// 	if (m_isShowCode == TRUE)
// 	{
// 		printf("%06d %06d %p    ", pstuCode->m_nID, pstuCode->m_nCount, 
// 			pstuCode->m_nEip);
// 		for (int i = 0; i < pstuCode->m_nCodeLen; i++)
// 		{
// 			printf("%s", &hexVale[pstuCode->m_OpCode[i]]);
// 		}
// 		printf("%s%s", "                          "+pstuCode->m_nCodeLen*2,
// 			pstuCode->m_AsmCode);
// 		if (pstuCode->m_chApiName[0] != 0)
// 		{
// 			printf("(%s)", pstuCode->m_chApiName);
// 		}
// 		printf("\r\n");
// 	}
// 
// 	if (pstuCode->m_nCount == 1)
// 	{
// 		wsprintf(buff, "%06d %p    ", pstuCode->m_nID, pstuCode->m_nEip);
// 
// 		for (int i = 0; i < pstuCode->m_nCodeLen; i++)
// 		{
// 			strcat(buff, hexVale[pstuCode->m_OpCode[i]]);
// 		}
// 
// 		strcat(buff, "                          "+pstuCode->m_nCodeLen*2);
// 		strcat(buff, pstuCode->m_AsmCode);
// 
// 		if (pstuCode->m_chApiName[0] != 0)
// 		{
// 			strcat(buff, "(");
// 			strcat(buff, pstuCode->m_chApiName);
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
// }



StuCode* CHandleException::AddInAvlTree(int nEip, char *pCodeBuf)
{
	StuCode stCode;
	stCode.m_nEip = nEip;
	memcpy(stCode.m_OpCode, pCodeBuf, 24);
	node<StuCode>* pNode = g_Avl_Tree.find_data(stCode);
	t_disasm da;

	//û���ҵ�
	if (pNode == NULL)
	{
		int nLen = Disasm(pCodeBuf, 24, 0, &da, DISASM_CODE, nEip);
		stCode.m_nCodeLen = nLen;
		strcpy(stCode.m_AsmCode, da.result);
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
	list<stuPointPage*>::iterator it = g_PointPageList.begin();

	for ( int i = 0; i < g_PointPageList.size(); i++ )
	{
		stuPointPage* pPointPage = *it;
		if (pPointPage->dwPageAddr == dwPageAddr)
		{
			return TRUE;
		}
		it++;
	}
	return FALSE;
}




BOOL  CHandleException::StepInto(StuCommand* pCmd)  //��������T
{
	UpdateContextFromThread();

	//���õ���
	m_Context.EFlags |= TF;   //���쵥���쳣
	m_isUserInputStep = TRUE;
	UpdateContextToThread();    
	return TRUE;
}




//��������
BOOL CHandleException::StepOver(StuCommand* pCmd)
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
		sprintf(buf, "%p", (int)m_Context.Eip + nCodelen);
		strcpy(m_UserCmd.chParam1, buf);
	//	Run(&m_UserCmd);
	}
	else
	{
		StepInto(NULL);
	}

	return TRUE;
}