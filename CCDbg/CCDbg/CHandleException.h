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
	static int				m_nOrdPtFlag;   //�ϵ����
	static DEBUG_EVENT      m_CCDbgEvent;
	static BOOL				m_bIsStart;
	static CONTEXT          m_Context;              //���Գ���Ļ���
	static CCPointInfo*    m_pFindPoint;           //�ҵ��Ķϵ�ָ��
//	static list<CCPointInfo*>::iterator m_itFind;  //�ҵ��Ķϵ��������еĵ�����λ��
	static EXCEPTION_DEBUG_INFO m_DbgInfo;
	static LPVOID           m_Eip;                  //���Գ����EIPֵ
	static BOOL             m_isStepRecordMode;     //�Ƿ񵥲���¼ģʽ
	static int              m_nCount;               //ָ��ִ��ʱ���±�ֵ������
	static BOOL				m_IsSystemInt3;
	static LPVOID           m_lpDisAsmAddr;         //��������ʼ��ַ
	static BOOL				m_isUserInputStep;
	static CCCommand        m_UserCmd;              //�û�������ַ���ת���ɵ�����ṹ��
	static BOOL             m_isNeedResetPoint;     //�ڵ������Ƿ���Ҫ������ʱ��ȡ���Ķϵ�
	static int              m_nHardPtNum;           //Ӳ���ϵ�����������
	static BOOL             m_isNeedResetHardPoint;
	static int              m_nNeedResetHardPoint;  //�ڵ�������Ҫ�����Ӳ���ϵ�Ĵ���
public:

	CHandleException(void);
	virtual ~CHandleException(void);

    static BOOL SetOrdPoint(CCCommand* pCmd); //������ͨCC�ϵ�
	static BOOL ListOrdPoint(CCCommand* pCmd);  //��ͨCC�ϵ�
	static BOOL ClearOrdPoint(CCCommand* pCmd);  //���CC�ϵ�

	static BOOL SetMemPoint(CCCommand* pCmd);   //�����ڴ�ϵ�
	static BOOL ListMemPoint(CCCommand* pCmd);
	static BOOL ClearMemPoint(CCCommand* pCmd);


	static BOOL SetHardPoint(CCCommand* pCmd);  //����Ӳ���ϵ�
	static BOOL ListHardPoint(CCCommand* pCmd);
	static BOOL ClearHardPoint(CCCommand* pCmd);

	static BOOL HandleException();
	static BOOL DoAccessException();//��������쳣����
	static BOOL HandleInt3Exception();//����INT3�쳣
	static BOOL HandleSingleStepException();//�������쳣

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