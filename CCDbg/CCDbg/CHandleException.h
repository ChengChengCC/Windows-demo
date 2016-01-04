#pragma  once

#include "Common.h"
#include "CCCode.h"


class CHandleException
{
public:

	static HANDLE           m_hProcess; 
	static DWORD            m_ProcessId; 
	static HANDLE           m_hThread;
	static LPVOID           m_lpOepAddr;
	static int				m_nOrdPtFlag;   //断点序号
	static DEBUG_EVENT      m_CCDbgEvent;
	static BOOL				m_bIsStart;
	static CONTEXT          m_Context;              //调试程序的环境
	static CCPointInfo*    m_pFindPoint;           //找到的断点指针
//	static list<CCPointInfo*>::iterator m_itFind;  //找到的断点在链表中的迭代器位置
	static EXCEPTION_DEBUG_INFO m_DbgInfo;
	static LPVOID           m_Eip;                  //调试程序的EIP值
	static BOOL             m_isStepRecordMode;     //是否单步记录模式
	static int              m_nCount;               //指令执行时的下标值计数器
	static BOOL				m_IsSystemInt3;
	static LPVOID           m_lpDisAsmAddr;         //反汇编的起始地址
	static BOOL				m_isUserInputStep;
	static CCCommand        m_UserCmd;              //用户输入的字符串转换成的命令结构体
	static BOOL             m_isNeedResetPoint;     //在单步中是否需要重设临时被取消的断点
	static int              m_nHardPtNum;           //硬件断点已设置数量
	static BOOL             m_isNeedResetHardPoint;
	static int              m_nNeedResetHardPoint;  //在单步中需要重设的硬件断点寄存器
public:

	CHandleException(void);
	virtual ~CHandleException(void);

    static BOOL SetOrdPoint(CCCommand* pCmd); //设置普通CC断点
	static BOOL ListOrdPoint(CCCommand* pCmd);  //普通CC断点
	static BOOL ClearOrdPoint(CCCommand* pCmd);  //清除CC断点

	static BOOL SetMemPoint(CCCommand* pCmd);   //设置内存断点
	static BOOL ListMemPoint(CCCommand* pCmd);
	static BOOL ClearMemPoint(CCCommand* pCmd);


	static BOOL SetHardPoint(CCCommand* pCmd);  //设置硬件断点
	static BOOL ListHardPoint(CCCommand* pCmd);
	static BOOL ClearHardPoint(CCCommand* pCmd);

	static BOOL HandleException();
	static BOOL DoAccessException();//处理访问异常部分
	static BOOL HandleInt3Exception();//处理INT3异常
	static BOOL HandleSingleStepException();//处理单步异常

	static VOID EnumDestMod();
	static BOOL FindPointInList(IN CCPointInfo PointInfo, OUT CCPointInfo** ppResultPointInfo,BOOL isNeedSave);
	static BOOL DeletePointInList(int nPtNum, BOOL isNeedResetProtect);
	static void ShowBreakPointInfo(CCPointInfo *pPoint);
	static void ShowHardwareBreakpoint(DWORD dwDr6Low);
	static BOOL UpdateContextFromThread();
	static BOOL UpdateContextToThread();
	static void RecordCode(int nEip, char *pCodeBuf);
	static CCCode* AddInAvlTree(IN int nEip, IN char* pCodeBuf);
	static void ShowAsmCode();
	static BOOL ShowFunctionName(char *pResult);
	static BOOL ShowMulAsmCode(CCCommand* pCmd);
	static BOOL FindRecordInPointPageList(DWORD dwPageAddr);
	static BOOL StepInto(CCCommand* pCmd);
	static BOOL ShowRegValue(CCCommand* pCmd);
	static BOOL StepOver(CCCommand* pCmd);
	static BOOL Run(CCCommand* pCmd);
	static BOOL WaitForUserInput();
	
	static void AddRecordInPageList(LPVOID BaseAddr, DWORD dwRegionSize, DWORD dwProtect);
	static BOOL FindRecordInPageList(DWORD dwBaseAddr, CCPageInfo** ppFind);
	static BOOL FindPointInConext(CCPointInfo PointInfo, 
		int *nDrNum, int *nPointLen);
	static BOOL FindFunction(DWORD dwFunAddr, DWORD dwDllAddr, char* pFunName);
	static void ResetMemBp();

	static void TempResumePageProp(DWORD dwPageAddr);

};



struct CCDllInfo
{
	DWORD dwDllAddr;
	DWORD dwModSize;
	char  szDllName[MAX_PATH];
};



typedef HANDLE (__stdcall *PFNOpenThreadFun)(
	DWORD dwDesiredAccess,  // access right
	BOOL bInheritHandle,    // handle inheritance option
	DWORD dwThreadId        // thread identifier
	);