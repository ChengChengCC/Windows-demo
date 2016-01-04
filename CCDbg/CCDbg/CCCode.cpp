#include "stdafx.h"

#include "CCCode.h"


int CCCode::operator==(const CCCode & c)
{
	//得到指令长度的较小值
	int nMinCodeLen = min(m_nCodeLen, c.m_nCodeLen);
	if(c.m_nEip == m_nEip && 
		0 == memcmp(c.m_OpCode, m_OpCode, nMinCodeLen))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


int CCCode::operator<(const CCCode & c)
{
	if(c.m_nEip < m_nEip)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int CCCode::operator>(const CCCode & c)
{
	if(c.m_nEip > m_nEip)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}