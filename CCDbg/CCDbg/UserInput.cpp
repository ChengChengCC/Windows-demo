#include "stdafx.h"

#include "Common.h"
#include "CHandleException.h"
extern CCCmdNode g_aryCmd[];
extern list<CCCommand*> g_UserInputList; 

BOOL CHandleException::WaitForUserInput()
{
	printf("COMMAND:");
	char chUserInputString[41] = {0};
	GetSafeStr(chUserInputString, 40);
	memset(&m_UserCmd, 0, sizeof(CCCommand));
	//用户输入的字符串转换为命令结构体
	BOOL bRet = ChangeStrToCmd(chUserInputString, &m_UserCmd);
	if ( bRet == FALSE)
	{
		return FALSE;
	}

	//根据输入的命令，查命令链表，调用相应处理函数
	pfnCmdProcessFun pFun = GetFunFromAryCmd(m_UserCmd);
	if (pFun)
	{
		CCCommand *pCmd = new CCCommand;
		memcpy(pCmd, &m_UserCmd, sizeof(CCCommand));
		g_UserInputList.push_back(pCmd);
		return (*pFun)(pCmd); //调用命令处理函数
	}
	else
	{
		printf("Error input!\r\n");
		return FALSE;
	}
}

//查命令链表，找对应的处理函数指针
pfnCmdProcessFun GetFunFromAryCmd(CCCommand UserCmd )
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


//安全输入
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



BOOL ChangeStrToCmd(IN char* chUserInputString, OUT CCCommand* pUserCmd)
{
	char * chStr = DelFrontSpace(chUserInputString);
	int nLen = strlen(chStr);

	int i = 0;
	int j = 0;  // 代表的是 CCCommand 结构体中第几个字段
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