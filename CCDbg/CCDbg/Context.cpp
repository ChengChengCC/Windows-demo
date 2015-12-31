#include "stdafx.h"
#include "CHandleException.h"




//���߳���Ϣ���µ� M_Context ������
BOOL CHandleException::UpdateContextFromThread()
{
	BOOL bRet = TRUE;
	m_Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
	bRet = GetThreadContext(m_hThread, &m_Context);
	if (bRet == FALSE)
	{
		printf("GetThreadContext error!\r\n");
	}
	return bRet;   
}




//�� M_Context �еĻ������µ��߳���
BOOL CHandleException::UpdateContextToThread()
{
	BOOL bRet = TRUE;
	bRet = SetThreadContext(m_hThread, &m_Context);
	if (bRet == FALSE)
	{
		printf("SetThreadContext error!\r\n");
	}
	return bRet;   
}