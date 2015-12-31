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
	static int				m_nOrdPtFlag;   //�ϵ����
	static DEBUG_EVENT      m_stuDbgEvent;
	static BOOL				m_bIsStart;
	static CONTEXT          m_Context;              //���Գ���Ļ���
	static stuPointInfo*    m_pFindPoint;           //�ҵ��Ķϵ�ָ��
//	static list<stuPointInfo*>::iterator m_itFind;  //�ҵ��Ķϵ��������еĵ�����λ��
	static EXCEPTION_DEBUG_INFO m_DbgInfo;
	static LPVOID           m_Eip;                  //���Գ����EIPֵ
	static BOOL             m_isStepRecordMode;     //�Ƿ񵥲���¼ģʽ
	static int              m_nCount;               //ָ��ִ��ʱ���±�ֵ������
	static BOOL				m_IsSystemInt3;
	static LPVOID           m_lpDisAsmAddr;         //��������ʼ��ַ
	static BOOL				m_isUserInputStep;
	static StuCommand       m_UserCmd;              //�û�������ַ���ת���ɵ�����ṹ��
public:

	CHandleException(void);
	virtual ~CHandleException(void);

    static BOOL SetOrdPoint(StuCommand* pCmd); //������ͨCC�ϵ�
	static BOOL CHandleException::HandleException();
	static BOOL HandleInt3Exception();//����INT3�쳣
	static BOOL HandleSingleStepException();//�������쳣
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