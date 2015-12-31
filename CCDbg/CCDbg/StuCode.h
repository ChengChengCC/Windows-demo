#pragma  once
#include "Common.h"


class StuCode
{
public:
	int     m_nID;                  //ָ��ִ��ʱ���±�ֵ
	int		m_nEip;				    //ָ���Ӧ��EIP
	int		m_nCount;				//ָ��ִ�еĴ���
	char    m_OpCode[24];           //ָ�������
	char    m_nCodeLen;             //ָ���
	char	m_AsmCode[100];		    //ָ������
	char    m_chApiName[100];       //�����CALLָ���CALL��API�������¼API����
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