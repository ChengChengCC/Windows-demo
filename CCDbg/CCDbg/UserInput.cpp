#include "stdafx.h"

#include "Common.h"
#include "CHandleException.h"
extern StuCmdNode g_aryCmd[];
extern list<StuCommand*> g_UserInputList; 

BOOL CHandleException::WaitForUserInput()
{
	printf("COMMAND:");
	char chUserInputString[41] = {0};
	GetSafeStr(chUserInputString, 40);

	memset(&m_UserCmd, 0, sizeof(StuCommand));

	//�û�������ַ���ת��Ϊ����ṹ��
	BOOL bRet = ChangeStrToCmd(chUserInputString, &m_UserCmd);

	if ( bRet == FALSE)
	{
		return FALSE;
	}

	//����������������������������Ӧ������
	pfnCmdProcessFun pFun = GetFunFromAryCmd(m_UserCmd);
	if (pFun)
	{
		StuCommand *pCmd = new StuCommand;
		memcpy(pCmd, &m_UserCmd, sizeof(StuCommand));
		g_UserInputList.push_back(pCmd);
		return (*pFun)(pCmd); //�����������
	}
	else
	{
		printf("Error input!\r\n");
		return FALSE;
	}
}

//�����������Ҷ�Ӧ�Ĵ�����ָ��
pfnCmdProcessFun GetFunFromAryCmd(StuCommand UserCmd )
{
	int i = 0;
	while (g_aryCmd[i].pFun != NULL)
	{
		if (stricmp(g_aryCmd[i].chCmd, UserCmd.chCmd) == 0)
		{
			return g_aryCmd[i].pFun;
		}
		i++;
	}
	return NULL;
}


//��ȫ����
void __stdcall GetSafeStr(char *p, int n)
{
	for(int i = 0; i < n; i++)
	{
		*p = getchar();
		if(*p == 0x0a)
		{
			*p = 0;
			break;
		}
		p++;
	}
	_flushall();
}



BOOL ChangeStrToCmd(IN char* chUserInputString, OUT StuCommand* pUserCmd)
{
	char * chStr = DelFrontSpace(chUserInputString);
	int nLen = strlen(chStr);

	int i = 0;
	int j = 0;  // ������� stuCommand �ṹ���еڼ����ֶ�
	//     int k = FIELD_LEN;
	while (chStr[0] != '\0' && chStr[0] != '\r')
	{
		for (i = 0; i < nLen; i++)
		{
			if (chStr[i] == ' ' || chStr[i] == '\0' || chStr[i] == '\r')
			{
				break;
			}

			if (i >= FIELD_LEN)
			{
				printf("Error command! One of the param's length greater than 20!\r\n");
				return FALSE;
			}
		}

		memcpy((char*)((int)pUserCmd + FIELD_LEN*j), chStr, i);
		j++;

		chStr = DelFrontSpace((char*)((int)chStr + i));
		nLen = strlen(chStr);
	}
	return TRUE;
}



char* DelFrontSpace(char * pStr)
{
	int nLen = strlen(pStr);
	for (int i = 0; i < nLen; i++)
	{
		if (pStr[i] != ' ')
		{
			return &pStr[i];
		}
	}
	return &pStr[nLen];
}