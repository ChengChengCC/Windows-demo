#pragma  once
#include "Common.h"


class StuCode
{
public:
	int     m_nID;                  //指令执行时的下标值
	int		m_nEip;				    //指令对应的EIP
	int		m_nCount;				//指令执行的次数
	char    m_OpCode[24];           //指令机器码
	char    m_nCodeLen;             //指令长度
	char	m_AsmCode[100];		    //指令内容
	char    m_chApiName[100];       //如果是CALL指令，且CALL到API，这里记录API名称
public:
	StuCode()
	{
		m_nID = -1;
		m_nEip = -1;
		m_nCount = 1;
		m_nCodeLen = 24;
		memset(m_AsmCode, 0, 100);
		memset(m_chApiName, 0, 100);
	}

	int operator==(const StuCode & c);
	int operator>(const StuCode & c);
	int operator<(const StuCode & c);
};