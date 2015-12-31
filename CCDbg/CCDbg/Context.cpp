#include "stdafx.h"
#include "CHandleException.h"




//将线程信息更新到 M_Context 变量中
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




//将 M_Context 中的环境更新到线程中
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