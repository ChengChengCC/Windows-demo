#include "stdafx.h"

#include "Common.h"
#include "CHandleException.h"

extern list<CCPageInfo*>  g_PageList;                 //分页信息链表
extern list<CCPointPage*>   g_PointPageList;  
extern list<CCPointInfo*>   g_ptList;                   //断点列表
extern list<CCResetMemBp*>  g_ResetMemBp;              //需要恢复的内存断点
//设置内存断点
BOOL CHandleException::SetMemPoint(CCCommand* pCmd)
{
	LPVOID  lpAddr = HexStringToHex(pCmd->chParam1, TRUE);

	if (lpAddr == 0)
	{
		printf("Need valid parameter!\r\n");
		return FALSE;
	}

	CCPointInfo tempPointInfo;
	CCPointInfo* pResultPointInfo = NULL;
	memset(&tempPointInfo, 0, sizeof(CCPointInfo));
	tempPointInfo.lpPointAddr = lpAddr;
	tempPointInfo.ptType = MEM_POINT;
	tempPointInfo.isOnlyOne = FALSE;

	if (stricmp("access", pCmd->chParam2) == 0)  //内存访问断点
	{
		tempPointInfo.ptAccess = ACCESS;
	} 
	else if (stricmp("write", pCmd->chParam2) == 0)  //内存读写断点
	{
		tempPointInfo.ptAccess = WRITE;
	}
	else
	{
		printf("Void access!\r\n");
		return FALSE;
	}

	int nLen = (int)HexStringToHex(pCmd->chParam3, TRUE);  //长度

	if (nLen == 0 )
	{
		printf("Point length can not set Zero!\r\n");
		return FALSE;
	}

	tempPointInfo.dwPointLen = nLen;
	tempPointInfo.nPtNum = m_nOrdPtFlag;
	m_nOrdPtFlag++;

	if (FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
	{
		if (pResultPointInfo->dwPointLen >= nLen)//存在同样类型且长度大于要设置断点的断点
		{
			printf("The Memory breakpoint is already exist!\r\n");
			return FALSE;
		} 
		else//查找到的断点长度小于要设置的断点长度，则删除掉找到的断点，重新设置
			//只删除断点-分页表项 和 断点表项
		{
			DeletePointInList(pResultPointInfo->nPtNum, FALSE);
		}
	}

	// 根据 tempPointInfo 设置内存断点
	// 添加断点链表项，添加内存断点-分页表中记录，添加分页信息表记录

	// 首先根据 tempPointInfo 中的地址和长度获得所跨越的全部分页

	LPVOID lpAddress = (LPVOID)((int)tempPointInfo.lpPointAddr & 0xfffff000);
	DWORD OutAddr = (DWORD)tempPointInfo.lpPointAddr + tempPointInfo.dwPointLen;
	MEMORY_BASIC_INFORMATION mbi = {0};
	while ( TRUE )
	{
		//查询内存断点所在虚拟内存页信息
		if ( sizeof(mbi) != VirtualQueryEx(m_hProcess, lpAddress, &mbi, sizeof(mbi)) )
		{
			break;
		}
		//所在虚拟内存也基地址大于内存断点结束地址  出错
		if ((DWORD)mbi.BaseAddress >= OutAddr)
		{
			break;            
		}
		//已分配物理页或系统页文件，可以访问
		if ( mbi.State == MEM_COMMIT )
		{
			//将内存分页信息添加到分页表中
			AddRecordInPageList(mbi.BaseAddress, 
				mbi.RegionSize, 
				mbi.AllocationProtect);
			//将断点-分页信息添加到断点-分页表中
			DWORD dwPageAddr = (DWORD)mbi.BaseAddress;
			while (dwPageAddr < OutAddr)
			{
				CCPointPage *pPointPage = new CCPointPage;
				pPointPage->dwPageAddr = dwPageAddr;
				pPointPage->nPtNum = tempPointInfo.nPtNum;
				g_PointPageList.push_back(pPointPage);
				//设置该内存页为不可访问
				DWORD dwTempProtect;
				VirtualProtectEx(m_hProcess, (LPVOID)dwPageAddr,
					1, PAGE_NOACCESS, &dwTempProtect);   //设置内存断点
				dwPageAddr += 0x1000;
			}

			//             TRACE2("0x%p  0x%p \r\n",mbi.BaseAddress,mbi.RegionSize);       
		}
		lpAddress = (LPVOID)((DWORD)mbi.BaseAddress + mbi.RegionSize);
		if ((DWORD)lpAddress >= OutAddr)
		{
			break;
		}
	}

	//断点添加到断点信息表中
	CCPointInfo *pPoint = new CCPointInfo;
	memcpy(pPoint, &tempPointInfo, sizeof(CCPointInfo));
	g_ptList.push_back(pPoint);
	printf("***Set Memory breakpoint success!***\r\n");
	return FALSE;
}




//将内存分页信息添加到分页表中，如果和已有的记录重复，则不添加
void CHandleException::AddRecordInPageList(LPVOID BaseAddr, 
	DWORD dwRegionSize, 
	DWORD dwProtect)
{
	DWORD dwBaseAddr = (DWORD)BaseAddr;
	int nPageNum = dwRegionSize / 0x1000;
	for (int i = 0; i < nPageNum; i++)
	{
		CCPageInfo* pFind = NULL;
		if (!FindRecordInPageList(dwBaseAddr, &pFind))
		{
			CCPageInfo* pPageInfo = new CCPageInfo;
			pPageInfo->dwPageAddr = dwBaseAddr;
			pPageInfo->dwOldProtect = dwProtect;
			g_PageList.push_back(pPageInfo);
		}

		dwBaseAddr += 0x1000;
	}
}



//查找分页信息表中的记录
BOOL CHandleException::FindRecordInPageList(DWORD dwBaseAddr, 
	CCPageInfo** ppFind)
{
	list<CCPageInfo*>::iterator it = g_PageList.begin();
	for ( int i = 0; i < g_PageList.size(); i++ )
	{
		CCPageInfo* pPageInfo = *it;
		if (pPageInfo->dwPageAddr == dwBaseAddr)
		{
			*ppFind = pPageInfo;
			return TRUE;
		}
		it++;
	}
	return FALSE;
}


void CHandleException::ResetMemBp()//根据g_ResetMemBp重设内存断点，在单步中调用
{
	list<CCResetMemBp*>::iterator itDw = g_ResetMemBp.begin();
	while (g_ResetMemBp.size())
	{
		CCResetMemBp* p = *itDw;
		itDw++;
		DWORD dwTempProtect;
		VirtualProtectEx(m_hProcess, (LPVOID)p->dwAddr,
			1, PAGE_NOACCESS, &dwTempProtect);
		delete p;
		g_ResetMemBp.remove(p);
	}
}



//内存断点列表
BOOL CHandleException::ListMemPoint(CCCommand* pCmd)
{
	printf("------------------------------------------------------------\r\n");
	printf("ID    Breakpoint type       Address     Access type   Length\r\n");
	list<CCPointInfo*>::iterator it = g_ptList.begin();
	for ( int i = 0; i < g_ptList.size(); i++ )
	{
		CCPointInfo* pPointInfo = *it;
		if (pPointInfo->ptType == MEM_POINT)
		{
			printf("%03d   Memory breakpoint     0x%p", 
				pPointInfo->nPtNum, pPointInfo->lpPointAddr);
			if (pPointInfo->ptAccess == WRITE)
			{
				printf("  WRITE ");
			} 
			else
			{
				printf("  ACCESS");
			}
			printf("        0x%x\r\n", pPointInfo->dwPointLen);
		}
		it++;
	}
	printf("------------------------------------------------------------\r\n");
	return FALSE;
}

//内存断点清除
BOOL CHandleException::ClearMemPoint(CCCommand* pCmd)
{
	int nID = (int)HexStringToHex(pCmd->chParam1, TRUE);

	if (nID == 0)
	{
		printf("Need valid ID!\r\n");
		return FALSE;
	}

	if (DeletePointInList(nID, TRUE))
	{
		//如果要删除的内存断点正好是要恢复的内存断点，则不需要再恢复内存断点
		list<CCResetMemBp*>::iterator itDw = g_ResetMemBp.begin();
		while (itDw != g_ResetMemBp.end())
		{
			CCResetMemBp* p = *itDw;
			itDw++;
			if (p->nID == nID)
			{
				delete p;
				g_ResetMemBp.remove(p);
				break;
			}
		}

		//         if (nID == m_nNeedResetMemPointID1)
		//         {
		//             m_isNeedResetPageProp1 = FALSE;
		//         }
		//         if (nID == m_nNeedResetMemPointID2)
		//         {
		//             m_isNeedResetPageProp2 = FALSE;
		//         }
		printf("Clear the %d Memory breakpoint.\r\n", nID);
	}
	else
	{
		printf("Can not find the Memory breakpoint!\r\n");
	}
	return FALSE;
}


