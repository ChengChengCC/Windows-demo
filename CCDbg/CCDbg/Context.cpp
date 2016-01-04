#include "stdafx.h"
#include "CHandleException.h"




//���̵߳�context��Ϣ���µ� m_Context ������
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




//�� m_context�е�context��Ϣ���µ��߳���
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