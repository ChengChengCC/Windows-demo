#include "stdafx.h"
#include "CHandleException.h"
#include "AVLTree.h"
#include <math.h>

list<CCPointInfo*> g_ptList;                   //断点列表
c_tree<CCCode>     g_Avl_Tree;
list<CCPointPage*> g_PointPageList;              //内存断点分页对照表
list<CCCommand*>   g_UserInputList;              //保存用户输入的合法命令的链表
list<CCDllInfo*>   g_DllList;                    //模块信息列表
list<CCResetMemBp*> g_ResetMemBp;              //需要恢复的内存断点
list<CCPageInfo*>  g_PageList;                 //分页信息链表


HANDLE			CHandleException::m_hProcess = 0; 
DWORD			CHandleException::m_ProcessId = 0; 
HANDLE			CHandleException::m_hThread = 0;
LPVOID			CHandleException::m_lpOepAddr = 0;
int				CHandleException::m_nOrdPtFlag = 0;   //断点序号
DEBUG_EVENT		CHandleException::m_CCDbgEvent = {0};
EXCEPTION_DEBUG_INFO CHandleException::m_DbgInfo = {0};
BOOL			CHandleException::m_bIsStart = TRUE;
CONTEXT         CHandleException::m_Context = {0};
CCPointInfo*   CHandleException::m_pFindPoint = NULL;           //找到的断点指针
list<CCPointInfo*>::iterator m_itFind;      //找到的断点在链表中的迭代器位置
LPVOID          CHandleException::m_Eip = 0;                  //调试程序的EIP值
BOOL             CHandleException::m_isStepRecordMode = FALSE;     //是否单步记录模式
int              CHandleException::m_nCount = 0;               //指令执行时的下标值计数器
BOOL             CHandleException::m_IsSystemInt3 = TRUE;
LPVOID           CHandleException::m_lpDisAsmAddr = NULL;         //反汇编的起始地址
BOOL			 CHandleException::m_isUserInputStep = FALSE;
CCCommand       CHandleException::m_UserCmd = {0};
BOOL             CHandleException::m_isNeedResetPoint = FALSE;
int              CHandleException::m_nHardPtNum = 0;     //已经设置硬件断点数量
BOOL             CHandleException::m_isNeedResetHardPoint = FALSE;
int              CHandleException::m_nNeedResetHardPoint = 0;  //在单步中需要重设的硬件断点寄存器


//全局命令-函数对照表
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
	{"", NULL}     //最后一个空项
};




CHandleException::CHandleException(void)
{


}


CHandleException::~CHandleException(void)
{


}





//处理异常函数。返回值为 TRUE, 调试器处理了异常，程序的调试状态为继续。
//返回值为 FALSE，调试状态为调式器没有处理异常。
BOOL CHandleException::HandleException()
{
	m_DbgInfo = m_CCDbgEvent.u.Exception;

	//打开线程
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

	//处理异常的框架
	switch (m_DbgInfo.ExceptionRecord.ExceptionCode) 
	{
		//访问异常
	case EXCEPTION_ACCESS_VIOLATION:   //((DWORD   )0xC0000005L)
		return HandleAccessException();
		break;
		//int3异常
	case EXCEPTION_BREAKPOINT:         //((DWORD   )0x80000003L)
		return HandleInt3Exception();
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: 
		break;
		//单步的处理
	case EXCEPTION_SINGLE_STEP:         //((DWORD   )0x80000004L)
		/*
		当TF位为1时，CPU每执行完一条指令就会产生一个调试异常（#DB）,
		中断到调试异常处理程序，调试异常的向量号为1，从80386开始，当硬件断点发生时
		也会产生调试异常，调用1号服务例程
		*/
		return HandleSingleStepException();
		break;
	case DBG_CONTROL_C: 
		break;
		// Handle other exceptions. 
	} 
	return TRUE;
}




BOOL CHandleException::HandleAccessException()//处理访问异常部分  内存断点
{
	BOOL                    bRet;
	DWORD                   dwAccessAddr;       //读写地址
	DWORD                   dwAccessFlag;       //读写标志
	BOOL                    isExceptionFromMemPoint = FALSE;    //异常是否由内存断点设置引起，默认为否
	CCPointInfo*            pPointInfo = NULL;                  //命中的断点
	BOOL                    isHitMemPoint = FALSE;

	//EXCEPTION_ACCESS_VIOLATION
	//If this value is zero, the thread attempted to read the inaccessible data. 
	//If this value is 1, the thread attempted to write to an inaccessible address. 
	//If this value is 8, the thread causes a user-mode data execution prevention (DEP) violation.

	dwAccessFlag = m_DbgInfo.ExceptionRecord.ExceptionInformation[0];
	dwAccessAddr = m_DbgInfo.ExceptionRecord.ExceptionInformation[1];
	//根据 访问地址 到“断点-分页表”中去查找
	//同一个内存分页可能有多个断点
	//如果没有在“断点-分页表”中查找到，则说明这个异常不是断点引起的
	list<CCPointPage*>::iterator it = g_PointPageList.begin();
	int nSize = g_PointPageList.size();

	//遍历链表中每个节点，将每个匹配的“断点-分页记录”都添加到g_ResetMemBp链表中
	for ( int i = 0; i < nSize; i++ )
	{
		CCPointPage* pPointPage = *it;
		//如果在“断点-分页表”中查找到
		//再根据断点表中信息判断是否符合用户所下断点信息
		if (pPointPage->dwPageAddr == (dwAccessAddr & 0xfffff000))//判断触发异常的地址在不在断点表中
		{
			CCResetMemBp *p = new CCResetMemBp;
			p->dwAddr = pPointPage->dwPageAddr;
			p->nID = pPointPage->nPtNum;                
			g_ResetMemBp.push_back(p);

			//暂时恢复内存页原来的属性
			BOOL bDoOnce = FALSE;
			if (!bDoOnce)
			{
				//这些操作只需要执行一次
				bDoOnce = TRUE;
				isExceptionFromMemPoint = TRUE;
				TempResumePageProp(pPointPage->dwPageAddr); //暂时恢复页面属性
				//设置单步，在单步中将断点设回
				UpdateContextFromThread();
				m_Context.EFlags |= TF;
				UpdateContextToThread();
			}

			//先找到断点序号对应的断点
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

			//再判断是否符合用户所下断点信息，断点类型和断点范围均相符
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

	//如果异常不是由内存断点设置引起，则调试器不处理
	if (isExceptionFromMemPoint == FALSE)
	{
		return FALSE;
	}

	//如果命中内存断点，则暂停，显示相关信息并等待用户输入
	if (isHitMemPoint)
	{
		ShowBreakPointInfo(pPointInfo);
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
	}
	return TRUE;
}




BOOL CHandleException::HandleInt3Exception()//处理INT3异常
{
	BOOL                    bRet;
	CCPointInfo             tempPointInfo;
	CCPointInfo*            pResultPointInfo = NULL;
	char                    CodeBuf[24] = {0};
	/*
	过滤初始断点，当新进程的初始线程在自己的上下文中初始化时，作为进程初始化的一个步骤，
	ntdll.dll中的LdrpInitializeProcess函数会检查正在初始化的进程是否处于被调试状态（检查
	PEB的BeingDebugged字段），如果是，他会调用DbgBreakPoint()触发一个断点异常，
	目的是中断到调试器，称为初始断点。
	*/
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

	memset(&tempPointInfo, 0, sizeof(CCPointInfo));
	tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	tempPointInfo.ptType = ORD_POINT;

	if(!FindPointInList(tempPointInfo, &pResultPointInfo, TRUE))
	{
		//没有找到对应断点，则说明不是用户或者调试器的断点
		//返回FALSE，调试器不处理，交给系统继续分派异常
		return FALSE;   
	}
	else    //找到了断点
	{
		ShowBreakPointInfo(pResultPointInfo);
		BOOL bRet = WriteProcessMemory(m_hProcess, //写回原来的字节
			m_pFindPoint->lpPointAddr, 
			&(m_pFindPoint->u.chOldByte), 1, NULL);

		if (bRet == FALSE)
		{
			printf("WriteProcessMemory error!\r\n");
			return FALSE;
		}
		//获取当前线程的Context
		UpdateContextFromThread();
		//0xcc 向前减去
		m_Context.Eip--;
		m_Eip = (LPVOID)m_Context.Eip;

		if (m_pFindPoint->isOnlyOne == TRUE)    //是一次性断点
		{
			g_ptList.remove(m_pFindPoint);
			delete m_pFindPoint;
			m_pFindPoint = nullptr;
		} 

		//
		else //不是一次性断点，就会是用户设置的断点，
			//需要设置单步，被调试进程再运行就会触发单步异常，到单步中异常中去处理，
		{
			//设置单步#define TF 0x100 标志寄存器TF位
			m_Context.EFlags |= TF;
			m_isNeedResetPoint = TRUE;   //
		}
	}

	//更新线程的Context
	UpdateContextToThread();

	//是否是单步记录模式，Trace功能
	if (m_isStepRecordMode == TRUE)  
	{
		bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
		if (bRet == FALSE)
		{
			printf("ReadProcessMemory error!");
			return FALSE;
		}
		//记录指令
		RecordCode(m_Context.Eip, CodeBuf);
		return TRUE;
	}

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
	CCPointInfo             tempPointInfo;
	CCPointInfo*            pResultPointInfo = NULL;
	char                    CodeBuf[24] = {0};
	DWORD                   dwOldProtect;
	DWORD                   dwNoUseProtect;

	UpdateContextFromThread();

	//需要重设CC断点，就是用户设置的CC断点被触发以后,恢复了原来的内存的内容，设置了单步异常
	//这里如果是CC断点设置的单步，那么就需要将0xcc重新写回目标地址
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
	//需要重设硬件断点  
	if (m_isNeedResetHardPoint == TRUE)
	{
		m_Context.Dr7 |= (int)pow((double)4, m_nNeedResetHardPoint);
		UpdateContextToThread();
		m_isNeedResetHardPoint = FALSE;
	}

	dwDr6 = m_Context.Dr6;
	dwDr6Low = dwDr6 & 0xf; //取低4位，分别对应Dr0~Dr3

	//如果是由硬件断点触发的单步，需要用户输入才能继续
	//另外，如果是硬件执行断点，则需要先暂时取消断点，设置单步，下次再恢复断点
	if (dwDr6Low != 0)  
	{
		ShowHardwareBreakpoint(dwDr6Low);  
		m_nNeedResetHardPoint = log((double)dwDr6Low)/log((double)2)+0.5;//加0.5是为了四舍五入
		//判断由 dwDr6Low 指定的DRX寄存器，是否是执行断点
		if((m_Context.Dr7 << (14 - (m_nNeedResetHardPoint * 2))) >> 30 == 0)
		{
			switch (m_nNeedResetHardPoint) //取消硬件断点
			{
			case 0:
				m_Context.Dr7 &= 0xfffffffe;  //1110 取消Dr0
				break;
			case 1:
				m_Context.Dr7 &= 0xfffffffb;  //1011 取消Dr1
				break;
			case 2:
				m_Context.Dr7 &= 0xffffffef;  //1110 1111 取消Dr2
				break;
			case 3:
				m_Context.Dr7 &= 0xffffffbf;  //1011 1111 取消Dr3
				break;
			default:
				printf("Error!\r\n");
			}
			m_Context.EFlags |= TF;  //设置单步
			UpdateContextToThread();
			m_isNeedResetHardPoint = TRUE;
		}

		m_isUserInputStep = TRUE; //这个设置只是为了能够等待用户输入
	}

	if (m_isUserInputStep == FALSE)
	{
		//重设内存断点
		ResetMemBp();
		return TRUE;
	}

	//以下代码在用户输入为 "T" 命令、或硬件断点触发时执行
	//如果此处有INT3断点，则需要先暂时删除INT3断点
	//这样做是为了在用户输入“T”命令、或硬件断点触发时忽略掉INT3断点
	//以免在一个地方停下两次
	memset(&tempPointInfo, 0, sizeof(CCPointInfo));
	tempPointInfo.lpPointAddr = m_DbgInfo.ExceptionRecord.ExceptionAddress;
	tempPointInfo.ptType = ORD_POINT;
	if (FindPointInList(tempPointInfo, &pResultPointInfo, TRUE)) //当前断下的地址是否存在CC断点
	{
		//非一次性断点，才需要重设断点
		if (pResultPointInfo->isOnlyOne == FALSE)
		{
			m_Context.EFlags |= TF;  //用户设置的断点，继续设置单步异常
			UpdateContextToThread();
			m_isNeedResetPoint = TRUE;
		}
		else//一次性断点，从链表里面删除
		{
			delete[] m_pFindPoint;
			g_ptList.erase(m_itFind);
		}
		//暂时去掉0xCC，恢复原来的指令
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

	//获取线程Context
	UpdateContextFromThread();

	if (m_isStepRecordMode == FALSE)
	{
		ShowAsmCode();
		ShowRegValue(NULL);
	}

	//重设内存断点
	ResetMemBp();

	//是否是单步记录模式  Trace
	if (m_isStepRecordMode == TRUE)
	{
		bRet = ReadProcessMemory(m_hProcess, (LPVOID)m_Context.Eip, CodeBuf, 24, NULL);
		if (bRet == FALSE)
		{
			printf("ReadProcessMemory error!\r\n");
			return FALSE;
		}
		//记录指令
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

//显示寄存器函数
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

//枚举模块
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
在断点列表中查找是否有匹配PointInfo断点信息的断点。
只匹配 断点起始地址，断点类型，断点访问类型。
参数isNeedSave，表示是否需要保存
返回值为TRUE表示找到；FALSE表示没有找到。
找到的断点指针，放入 ppResultPointInfo 参数中
*/
{
	list<CCPointInfo*>::iterator it = g_ptList.begin();
	for ( int i = 0; i < g_ptList.size(); i++ )
	{
		CCPointInfo* tempPointInfo = *it;
		if (tempPointInfo->lpPointAddr == PointInfo.lpPointAddr &&   //地址匹配
			tempPointInfo->ptType == PointInfo.ptType &&             //类型匹配 
			tempPointInfo->ptAccess == PointInfo.ptAccess)
		{
			*ppResultPointInfo = tempPointInfo;
			if (isNeedSave == TRUE)
			{
// 				m_itFind = it;   ？？？
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
	//先根据序号找到对应断点
	list<CCPointInfo*>::iterator it = g_ptList.begin();
	for ( int i = 0; i < g_ptList.size(); i++ )
	{
		CCPointInfo* pPointInfo = *it;
		if (pPointInfo->nPtNum == nPtNum)//找到了
		{
			//先判断是不是内存断点，如果不是就是错误的
			if (pPointInfo->ptType != MEM_POINT)
			{
				printf("Only allow memony breakpoint reach here!\r\n");
				return FALSE;
			}

			//是内存断点，要删除断点表项 和 断点-分页表项
			delete [] pPointInfo;
			g_ptList.erase(it);

			//删除断点-分页表项
			//DeleteRecordInPointPageList(nPtNum, isNeedResetProtect);   ？？？

			return TRUE; 
		}
		it++;
	}
	return FALSE;
}




void CHandleException::ShowBreakPointInfo(CCPointInfo *pPoint)
{
	//单次CC断点不显示，是调试器自己下的
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




void CHandleException::RecordCode(int nEip, char *pCodeBuf)  //OD 的Trace功能
{
// 	CCCode* pCCCode = AddInAvlTree(nEip, pCodeBuf);
// 	DWORD  dwBytesWritten, dwPos; 
// 	char   buff[400]; 
// 
// 	//让程序继续运行
// 	//但是需要根据 pCCCode 结构体判断是要单步步入还是单步步过
// 	ContinueRun(pCCCode);
// 
// 	//显示出来
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

	//没有找到
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

	//找到
	pNode->data.m_nCount++;
	return &pNode->data;
}




//在断点-分页表中查找记录
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




BOOL  CHandleException::StepInto(CCCommand* pCmd)  //单步步入T
{
	UpdateContextFromThread();
	//设置单步
	m_Context.EFlags |= TF;   //设置标志寄存器为单步模式，CPU每执行一条指令，触发单步异常
	m_isUserInputStep = TRUE;
	UpdateContextToThread();    
	return TRUE;
}




//单步步过
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



BOOL CHandleException::Run(CCCommand* pCmd)  //返回TRUE，直接运行
{
	LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);
	if (lpAddr != (LPVOID)0)
	{
		wsprintf(pCmd->chParam2, "%s", "once");
		SetOrdPoint(pCmd);
	}
	return TRUE;
}



//临时恢复内存属性页的属性
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