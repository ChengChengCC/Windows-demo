#include "stdafx.h"
#include "CHandleException.h"
#include "AVLTree.h"


list<stuPointInfo*> g_ptList;                   //断点列表
c_tree<StuCode> g_Avl_Tree;
list<stuPointPage*> g_PointPageList;            //内存断点分页对照表
list<StuCommand*> g_UserInputList;              //保存用户输入的合法命令的链表

HANDLE			CHandleException::m_hProcess = 0; 
DWORD			CHandleException::m_ProcessId = 0; 
HANDLE			CHandleException::m_hThread = 0;
LPVOID			CHandleException::m_lpOepAddr = 0;
int				CHandleException::m_nOrdPtFlag = 0;   //断点序号
DEBUG_EVENT		CHandleException::m_stuDbgEvent = {0};
EXCEPTION_DEBUG_INFO CHandleException::m_DbgInfo = {0};
BOOL			CHandleException::m_bIsStart = TRUE;
CONTEXT         CHandleException::m_Context = {0};
stuPointInfo*   CHandleException::m_pFindPoint = NULL;           //找到的断点指针
list<stuPointInfo*>::iterator m_itFind;  //找到的断点在链表中的迭代器位置
LPVOID          CHandleException::m_Eip = 0;                  //调试程序的EIP值
BOOL             CHandleException::m_isStepRecordMode = FALSE;     //是否单步记录模式
int              CHandleException::m_nCount = 0;               //指令执行时的下标值计数器
BOOL             CHandleException::m_IsSystemInt3 = TRUE;
LPVOID           CHandleException::m_lpDisAsmAddr = NULL;         //反汇编的起始地址
BOOL			 CHandleException::m_isUserInputStep = FALSE;
StuCommand       CHandleException::m_UserCmd = {0};





//全局命令-函数对照表
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
	{"", NULL}     //最后一个空项
};




CHandleException::CHandleException(void)
{


}


CHandleException::~CHandleException(void)
{


}



//设置一般断点
BOOL CHandleException::SetOrdPoint(StuCommand* pCmd)
{
	BOOL    bRet;
	LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);

	if (lpAddr == 0)
	{
		printf("Need valid parameter!\r\n");
		return FALSE;
	}

	//在断点列表中查找是否已经存在此处的一般断点
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
// 		if (tempPointInfo.isOnlyOne == FALSE)//设置的是非一次性断点
// 		{
// 			if (pResultPointInfo->isOnlyOne == FALSE)//查找到的是非一次性断点
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

	//设置地址的首地址为0xcc
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

	//将新设置的断点加入到断点列表中
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


//处理异常函数。返回值为 TRUE, 调试器处理了异常，程序的调试状态为继续。
//返回值为 FALSE，调试状态为调式器没有处理异常。
BOOL CHandleException::HandleException()
{
	m_DbgInfo = m_stuDbgEvent.u.Exception;

	//打开线程
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

	//处理异常的框架
	switch (m_DbgInfo.ExceptionRecord.ExceptionCode) 
	{
		//访问异常
	case EXCEPTION_ACCESS_VIOLATION:   //((DWORD   )0xC0000005L)
	//	return DoAccessException();
		break;
		//int3异常
	case EXCEPTION_BREAKPOINT:         //((DWORD   )0x80000003L)
		return HandleInt3Exception();
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: 
		break;
		//单步的处理
	case EXCEPTION_SINGLE_STEP:         //((DWORD   )0x80000004L)  
		return HandleSingleStepException();
		break;
	case DBG_CONTROL_C: 
		break;
		// Handle other exceptions. 
	} 
	return TRUE;
}





BOOL CHandleException::HandleInt3Exception()//处理INT3异常
{
	BOOL                    bRet;
	stuPointInfo            tempPointInfo;
	stuPointInfo*           pResultPointInfo = NULL;
	char                    CodeBuf[24] = {0};

	//先过掉系统的INT3断点
	if (m_IsSystemInt3 == TRUE)
	{
		m_IsSystemInt3 = FALSE;
		return TRUE;
	}


	//程序刚启动，停在OEP处
	if(m_DbgInfo.ExceptionRecord.ExceptionAddress == m_lpOepAddr
		&& m_bIsStart == TRUE)
	{
		m_bIsStart = FALSE;
		//枚举目标进程的模块，并写入 g_DllList
		EnumDestMod();
	}

	memset(&tempPointInfo, 0, sizeof(stuPointInfo));
	tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	tempPointInfo.ptType = ORD_POINT;

	if(!FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
	{
		//没有找到对应断点，则说明不是用户下的断定
		//返回FALSE，调试器不处理，交给系统继续分派异常
		return FALSE;   
	}
	else    //找到了断点
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
		//获取环境
		UpdateContextFromThread();
		////?????
		m_Context.Eip--;
		m_Eip = (LPVOID)m_Context.Eip;

		if (m_pFindPoint->isOnlyOne == TRUE)    //是一次性断点
		{
// 			delete m_pFindPoint;
// 			g_ptList.erase(m_itFind);     //erase ????

			g_ptList.remove(m_pFindPoint);
			delete m_pFindPoint;
			m_pFindPoint = nullptr;
		} 

		//???????
// 		else //不是一次性断点，需要设置单步，到单步中去重设断点
// 		{
// 			//设置单步#define TF 0x100
// 			m_Context.EFlags |= TF;
// 	//		m_isNeedResetPoint = TRUE;   //????
// 		}
	}

	//恢复环境
	UpdateContextToThread();

	//是否是单步记录模式
// 	if (m_isStepRecordMode == TRUE)
// 	{
// 		bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
// 		if (bRet == FALSE)
// 		{
// 			printf("ReadProcessMemory error!");
// 			return FALSE;
// 		}
// 		//记录指令
// 		RecordCode(m_Context.Eip, CodeBuf);
// 		return TRUE;
// 	}

	//显示反汇编代码
	m_lpDisAsmAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	ShowAsmCode();
	ShowRegValue(NULL);

	//等待用户输入
	bRet = FALSE;
	while (bRet == FALSE)
	{
		bRet = WaitForUserInput();
	}
	return TRUE;
}


BOOL CHandleException::HandleSingleStepException()//处理单步异常
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

	//需要重设INT3断点
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
// 	//需要重设硬件断点
// 	if (m_isNeedResetHardPoint == TRUE)
// 	{
// 		m_Context.Dr7 |= (int)pow(4, m_nNeedResetHardPoint);
// 		UpdateContextToThread();
// 		m_isNeedResetHardPoint = FALSE;
// 	}
// 
// 	dwDr6 = m_Context.Dr6;
// 	dwDr6Low = dwDr6 & 0xf; //取低4位
// 
// 	//如果是由硬件断点触发的单步，需要用户输入才能继续
// 	//另外，如果是硬件执行断点，则需要先暂时取消断点，设置单步，下次再恢复断点
// 	if (dwDr6Low != 0)
// 	{
// 		ShowHardwareBreakpoint(dwDr6Low);
// 		m_nNeedResetHardPoint = log(dwDr6Low)/log(2)+0.5;//加0.5是为了四舍五入
// 		//判断由 dwDr6Low 指定的DRX寄存器，是否是执行断点
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
// 		m_isUserInputStep = TRUE; //这个设置只是为了能够等待用户输入
// 	}
// 
// 	if (m_isUserInputStep == FALSE)
// 	{
// 		//重设内存断点
// 		ResetMemBp();
// 		return TRUE;
// 	}

	//以下代码在用户输入为 "T" 命令、或硬件断点触发时执行
	//如果此处有INT3断点，则需要先暂时删除INT3断点
	//这样做是为了在用户输入“T”命令、或硬件断点触发时忽略掉INT3断点
	//以免在一个地方停下两次
	memset(&tempPointInfo, 0, sizeof(stuPointInfo));
	tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	tempPointInfo.ptType = ORD_POINT;

	if (FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
	{
		//非一次性断点，才需要重设断点
		if (pResultPointInfo->isOnlyOne == FALSE)
		{
			m_Context.EFlags |= TF;
			UpdateContextToThread();
	//		m_isNeedResetPoint = TRUE;
		}
		else//一次性断点，从链表里面删除
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

	//更新m_Context为现在的环境值
	UpdateContextFromThread();

	if (m_isStepRecordMode == FALSE)
	{
		ShowAsmCode();
		ShowRegValue(NULL);
	}

	//重设内存断点
//	ResetMemBp();

	//是否是单步记录模式
	if (m_isStepRecordMode == TRUE)
	{
		bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
		if (bRet == FALSE)
		{
			printf("ReadProcessMemory error!\r\n");
			return FALSE;
		}
		//记录指令
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

//显示寄存器函数
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

//枚举模块
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




//在断点列表中查找是否有匹配PointInfo断点信息的断点。
//只匹配 断点起始地址，断点类型，断点访问类型。
//参数isNeedSave，表示是否需要保存
//返回值为TRUE表示找到；FALSE表示没有找到。
//找到的断点指针，放入 ppResultPointInfo 参数中
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
	//单次CC断点不显示
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
			printf("error!");//不会有这种情况
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
// 	//让程序继续运行
// 	//但是需要根据 pstuCode 结构体判断是要单步步入还是单步步过
// 	ContinueRun(pstuCode);
// 
// 	//显示出来
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

	//没有找到
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

	//找到
	pNode->data.m_nCount++;
	return &pNode->data;
}




//在断点-分页表中查找记录
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




BOOL  CHandleException::StepInto(StuCommand* pCmd)  //单步步入T
{
	UpdateContextFromThread();

	//设置单步
	m_Context.EFlags |= TF;   //制造单步异常
	m_isUserInputStep = TRUE;
	UpdateContextToThread();    
	return TRUE;
}




//单步步过
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

	//查看要反汇编代码的地址所在的内存分页是否已经有内存断点
	//如果有，先修改内存属性页为可读，读完之后再改为原先属性
	//注意，所读内存可能跨分页
	//高10位代表页目录的索引      0000 0000 00
	//中间10位代表页表的索引	  01 0111 1111
	//低12位代表页偏移		    1111 0100 0100
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

	//读完之后重设断点，这里要注意，可能 m_lpDisAsmAddr 和 m_lpDisAsmAddr+20
	//还是在同一个分页上，即 SecondPage 和 FirstPage是同一个分页
	//所以先恢复 SecondPage，后恢复 FirstPage
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

	nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//调用反汇编引擎
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