#pragma  once

#include "Common.h"
#include "StuCode.h"


class CHandleException
{
public:

	static HANDLE           m_hProcess; 
	static DWORD            m_ProcessId; 
	static HANDLE           m_hThread;
	static LPVOID           m_lpOepAddr;
	static int				m_nOrdPtFlag;   //断点序号
	static DEBUG_EVENT      m_stuDbgEvent;
	static BOOL				m_bIsStart;
	static CONTEXT          m_Context;              //调试程序的环境
	static stuPointInfo*    m_pFindPoint;           //找到的断点指针
//	static list<stuPointInfo*>::iterator m_itFind;  //找到的断点在链表中的迭代器位置
	static EXCEPTION_DEBUG_INFO m_DbgInfo;
	static LPVOID           m_Eip;                  //调试程序的EIP值
	static BOOL             m_isStepRecordMode;     //是否单步记录模式
	static int              m_nCount;               //指令执行时的下标值计数器
	static BOOL				m_IsSystemInt3;
	static LPVOID           m_lpDisAsmAddr;         //反汇编的起始地址
	static BOOL				m_isUserInputStep;
	static StuCommand       m_UserCmd;              //用户输入的字符串转换成的命令结构体
public:

	CHandleException(void);
	virtual ~CHandleException(void);

    static BOOL SetOrdPoint(StuCommand* pCmd); //设置普通CC断点
	static BOOL CHandleException::HandleException();
	static BOOL HandleInt3Exception();//处理INT3异常
	static BOOL HandleSingleStepException();//处理单步异常
	static VOID EnumDestMod();
	static BOOL FindPointInList(IN stuPointInfo PointInfo, OUT stuPointInfo** ppResultPointInfo,BOOL isNeedSave);
	static void ShowBreakPointInfo(stuPointInfo *pPoint);
	static BOOL UpdateContextFromThread();
	static BOOL UpdateContextToThread();
	static void RecordCode(int nEip, char *pCodeBuf);
	static StuCode* AddInAvlTree(IN int nEip, IN char* pCodeBuf);
	static void ShowAsmCode();
	static BOOL ShowMulAsmCode(StuCommand* pCmd);
	static BOOL FindRecordInPointPageList(DWORD dwPageAddr);
	static BOOL StepInto(StuCommand* pCmd);
	static BOOL ShowRegValue(StuCommand* pCmd);
	static BOOL StepOver(StuCommand* pCmd);
	static BOOL WaitForUserInput();
	

};





typedef HANDLE (__stdcall *PFNOpenThreadFun)(
	DWORD dwDesiredAccess,  // access right
	BOOL bInheritHandle,    // handle inheritance option
	DWORD dwThreadId        // thread identifier
	);