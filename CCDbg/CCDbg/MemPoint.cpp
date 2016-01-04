#include "stdafx.h"

#include "Common.h"
#include "CHandleException.h"

extern list<CCPageInfo*>  g_PageList;                 //��ҳ��Ϣ����
extern list<CCPointPage*>   g_PointPageList;  
extern list<CCPointInfo*>   g_ptList;                   //�ϵ��б�
extern list<CCResetMemBp*>  g_ResetMemBp;              //��Ҫ�ָ����ڴ�ϵ�
//�����ڴ�ϵ�
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

	if (stricmp("access", pCmd->chParam2) == 0)  //�ڴ���ʶϵ�
	{
		tempPointInfo.ptAccess = ACCESS;
	} 
	else if (stricmp("write", pCmd->chParam2) == 0)  //�ڴ��д�ϵ�
	{
		tempPointInfo.ptAccess = WRITE;
	}
	else
	{
		printf("Void access!\r\n");
		return FALSE;
	}

	int nLen = (int)HexStringToHex(pCmd->chParam3, TRUE);  //����

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
		if (pResultPointInfo->dwPointLen >= nLen)//����ͬ�������ҳ��ȴ���Ҫ���öϵ�Ķϵ�
		{
			printf("The Memory breakpoint is already exist!\r\n");
			return FALSE;
		} 
		else//���ҵ��Ķϵ㳤��С��Ҫ���õĶϵ㳤�ȣ���ɾ�����ҵ��Ķϵ㣬��������
			//ֻɾ���ϵ�-��ҳ���� �� �ϵ����
		{
			DeletePointInList(pResultPointInfo->nPtNum, FALSE);
		}
	}

	// ���� tempPointInfo �����ڴ�ϵ�
	// ��Ӷϵ����������ڴ�ϵ�-��ҳ���м�¼����ӷ�ҳ��Ϣ���¼

	// ���ȸ��� tempPointInfo �еĵ�ַ�ͳ��Ȼ������Խ��ȫ����ҳ

	LPVOID lpAddress = (LPVOID)((int)tempPointInfo.lpPointAddr & 0xfffff000);
	DWORD OutAddr = (DWORD)tempPointInfo.lpPointAddr + tempPointInfo.dwPointLen;
	MEMORY_BASIC_INFORMATION mbi = {0};
	while ( TRUE )
	{
		//��ѯ�ڴ�ϵ����������ڴ�ҳ��Ϣ
		if ( sizeof(mbi) != VirtualQueryEx(m_hProcess, lpAddress, &mbi, sizeof(mbi)) )
		{
			break;
		}
		//���������ڴ�Ҳ����ַ�����ڴ�ϵ������ַ  ����
		if ((DWORD)mbi.BaseAddress >= OutAddr)
		{
			break;            
		}
		//�ѷ�������ҳ��ϵͳҳ�ļ������Է���
		if ( mbi.State == MEM_COMMIT )
		{
			//���ڴ��ҳ��Ϣ��ӵ���ҳ����
			AddRecordInPageList(mbi.BaseAddress, 
				mbi.RegionSize, 
				mbi.AllocationProtect);
			//���ϵ�-��ҳ��Ϣ��ӵ��ϵ�-��ҳ����
			DWORD dwPageAddr = (DWORD)mbi.BaseAddress;
			while (dwPageAddr < OutAddr)
			{
				CCPointPage *pPointPage = new CCPointPage;
				pPointPage->dwPageAddr = dwPageAddr;
				pPointPage->nPtNum = tempPointInfo.nPtNum;
				g_PointPageList.push_back(pPointPage);
				//���ø��ڴ�ҳΪ���ɷ���
				DWORD dwTempProtect;
				VirtualProtectEx(m_hProcess, (LPVOID)dwPageAddr,
					1, PAGE_NOACCESS, &dwTempProtect);   //�����ڴ�ϵ�
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

	//�ϵ���ӵ��ϵ���Ϣ����
	CCPointInfo *pPoint = new CCPointInfo;
	memcpy(pPoint, &tempPointInfo, sizeof(CCPointInfo));
	g_ptList.push_back(pPoint);
	printf("***Set Memory breakpoint success!***\r\n");
	return FALSE;
}




//���ڴ��ҳ��Ϣ��ӵ���ҳ���У���������еļ�¼�ظ��������
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



//���ҷ�ҳ��Ϣ���еļ�¼
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


void CHandleException::ResetMemBp()//����g_ResetMemBp�����ڴ�ϵ㣬�ڵ����е���
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



//�ڴ�ϵ��б�
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

//�ڴ�ϵ����
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
		//���Ҫɾ�����ڴ�ϵ�������Ҫ�ָ����ڴ�ϵ㣬����Ҫ�ٻָ��ڴ�ϵ�
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


