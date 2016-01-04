#include "stdafx.h"

#include "Common.h"
#include "CHandleException.h"


extern list<CCPointInfo*> g_ptList;                   //断点列表

//设置一般断点
BOOL CHandleException::SetOrdPoint(CCCommand* pCmd)
{
	BOOL    bRet;
	LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);//第一个参数地址
	if (lpAddr == 0)
	{
		printf("Need valid parameter!\r\n");
		return FALSE;
	}
	//在断点列表中查找是否已经存在此处的一般断点
	CCPointInfo tempPointInfo;
	CCPointInfo* pResultPointInfo = NULL;
	memset(&tempPointInfo, 0, sizeof(CCPointInfo));
	tempPointInfo.lpPointAddr = lpAddr;
	tempPointInfo.ptType = ORD_POINT;
	if (stricmp(pCmd->chParam2, "once") == 0)
	{
		tempPointInfo.isOnlyOne = TRUE;
	} 
	else
	{
		tempPointInfo.isOnlyOne = FALSE;
	}

	if (FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
	{
		if (tempPointInfo.isOnlyOne == FALSE)//设置的是非一次性断点
		{
			if (pResultPointInfo->isOnlyOne == FALSE)//查找到的是非一次性断点
			{
				printf("This Ordinary BreakPoint is already exist!\r\n");
			} 
			else
			{
				pResultPointInfo->isOnlyOne = FALSE;
			}
		}
		return FALSE;
	}    

	//设置地址的首地址为0xcc
	char chOld;
	char chCC = 0xcc;
	DWORD dwOldProtect;
	VirtualProtectEx(m_hProcess, lpAddr, 1, PAGE_READWRITE, &dwOldProtect);
	bRet = ReadProcessMemory(m_hProcess, lpAddr, &chOld, 1, NULL);
	if (bRet == FALSE)
	{
		printf("ReadProcessMemory error! may be is not a valid memory address!\r\n");
		return FALSE;
	}

	bRet = WriteProcessMemory(m_hProcess, lpAddr, &chCC, 1, NULL);  //写入'0xCC'
	if (bRet == FALSE)
	{
		printf("WriteProcessMemory error!\r\n");
		return FALSE;
	}

	VirtualProtectEx(m_hProcess, lpAddr, 1, dwOldProtect, &dwOldProtect);  //恢复原来页面属性

	//将新设置的断点加入到断点列表中
	CCPointInfo* NewPointInfo = new CCPointInfo;
	memset(NewPointInfo, 0, sizeof(CCPointInfo));
	NewPointInfo->nPtNum = m_nOrdPtFlag;
	m_nOrdPtFlag++;
	NewPointInfo->ptType = ORD_POINT;
	NewPointInfo->lpPointAddr = lpAddr;
	NewPointInfo->u.chOldByte = chOld;
	NewPointInfo->isOnlyOne = tempPointInfo.isOnlyOne;
	g_ptList.push_back(NewPointInfo);
	// 	if (m_isStepRecordMode == FALSE && m_nOrdPtFlag != 1)
	// 	{
	// 		printf("***Set Ordinary breakpoint(INT3) success!***\r\n");
	// 	}

	return FALSE;
}





//CC断点列表
BOOL CHandleException::ListOrdPoint(CCCommand* pCmd)
{
	printf("------------------------------------\r\n");
	printf("ID    Breakpoint type    Address\r\n");
	list<CCPointInfo*>::iterator it = g_ptList.begin();
	for ( int i = 0; i < g_ptList.size(); i++ )
	{
		CCPointInfo* pPointInfo = *it;
		if (pPointInfo->ptType == ORD_POINT)
		{
			printf("%03d   INT3 breakpoint    0x%p\r\n", 
				pPointInfo->nPtNum, pPointInfo->lpPointAddr);
		}
		it++;
	}
	printf("------------------------------------\r\n");
	return FALSE;
}

//一般断点清除
BOOL CHandleException::ClearOrdPoint(CCCommand* pCmd)
{
	BOOL bRet;
	int nID = (int)HexStringToHex(pCmd->chParam1, TRUE);

	if (nID == 0)
	{
		printf("Need valid ID!\r\n");
		return FALSE;
	}
	list<CCPointInfo*>::iterator it = g_ptList.begin();
	for ( int i = 0; i < g_ptList.size(); i++ )
	{
		CCPointInfo* pPointInfo = *it;
		if (pPointInfo->nPtNum == nID)
		{
			DWORD dwOldProtect;
			DWORD dwNoUseProtect;
			VirtualProtectEx(m_hProcess, pPointInfo->lpPointAddr,
				1, PAGE_READWRITE, &dwOldProtect);
			bRet = WriteProcessMemory(m_hProcess, pPointInfo->lpPointAddr, 
				&(pPointInfo->u.chOldByte), 1, NULL);
			if (bRet == FALSE)
			{
				printf("WriteProcessMemory error!\r\n");
				return FALSE;
			}           
			VirtualProtectEx(m_hProcess, pPointInfo->lpPointAddr,
				1, dwOldProtect, &dwNoUseProtect);

			//如果删除的是需要恢复的INT3断点，则将恢复表示置为 FALSE
			if (m_isNeedResetPoint == TRUE && m_pFindPoint->nPtNum == nID)
			{
				m_isNeedResetPoint = FALSE;
			}

			delete [] pPointInfo;
			g_ptList.erase(it);
			//删除，提示
			printf("Clear the %d Ordinary breakpoint.\r\n", nID);
			return FALSE;
		}
		it++;
	}
	//没有找到，提示
	printf("Can not find the Ordinary breakpoint!\r\n");
	return FALSE;
}