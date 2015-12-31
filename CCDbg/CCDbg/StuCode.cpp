#include "stdafx.h"

#include "StuCode.h"


int StuCode::operator==(const StuCode & c)
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




int StuCode::operator<(const StuCode & c)
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




int StuCode::operator>(const StuCode & c)
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