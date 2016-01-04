#include "stdafx.h"
#include "CHandleException.h"




//将线程的context信息更新到 m_Context 变量中
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




//将 m_context中的context信息更新到线程中
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