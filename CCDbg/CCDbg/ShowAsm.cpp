#include "stdafx.h"
#include "CHandleException.h"



extern list<CCDllInfo*>   g_DllList;                    //ģ����Ϣ�б�


// ��m_lpDisAsmAddr ��ָ����λ�ÿ�ʼ���з����
void CHandleException::ShowAsmCode()
{
	char            CodeBuf[20];
	t_disasm        da;
	int             nCodelen;
	BOOL            bRet;
	BOOL            isNeedResetFirstPage = FALSE;
	BOOL            isNeedResetSecondPage = FALSE;
	DWORD           dwTempProtect1;
	DWORD           dwTempProtect2;
	DWORD           dwOldProtect;

	//�鿴Ҫ��������ĵ�ַ���ڵ��ڴ��ҳ�Ƿ��Ѿ����ڴ�ϵ�
	//����У����޸��ڴ�����ҳΪ�ɶ�������֮���ٸ�Ϊ���ɷ���
	//ע�⣬�����ڴ���ܿ��ҳ
	if (FindRecordInPointPageList((DWORD)m_lpDisAsmAddr & 0xfffff000))
	{
		VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
			1, PAGE_READONLY, &dwTempProtect1);
		isNeedResetFirstPage = TRUE;
	}
	//+20�ĵ�ַ�ǲ����ڴ�ϵ�ҳ�Ļ���ַ
	if (FindRecordInPointPageList(((DWORD)m_lpDisAsmAddr + 20) & 0xfffff000))
	{
		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
			1, PAGE_READONLY, &dwTempProtect2);
		isNeedResetSecondPage = TRUE;
	}

	bRet = ReadProcessMemory(m_hProcess, m_lpDisAsmAddr, CodeBuf, 20, NULL);

	//����֮��ָ����ԣ�����Ҫע�⣬���� m_lpDisAsmAddr �� m_lpDisAsmAddr+20
	//������ͬһ�������ڴ�ҳ�ϣ��� SecondPage �� FirstPage��ͬһ����ҳ
	//�����Ȼָ� SecondPage����ָ� FirstPage
	if (isNeedResetSecondPage)
	{
		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
			1, dwTempProtect2, &dwOldProtect);
		isNeedResetSecondPage = FALSE;
	}

	if (isNeedResetFirstPage)
	{
		VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
			1, dwTempProtect1, &dwOldProtect);
		isNeedResetFirstPage = FALSE;
	}

	if (bRet == FALSE)
	{
		printf("ReadProcessMemory error!\r\n");
		return;
	}

	//����������ڴ��ֽ�����0xCC��ͷ�ģ���鿴���0xCC�Ƿ����û��µ�����ϵ�
	if (CodeBuf[0] == 0xCC)
	{
		CCPointInfo tempPointInfo;
		CCPointInfo* pResultPointInfo = NULL;
		memset(&tempPointInfo, 0, sizeof(CCPointInfo));
		tempPointInfo.lpPointAddr = m_lpDisAsmAddr;
		tempPointInfo.ptType = ORD_POINT;

		if(FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
		{
			CodeBuf[0] = pResultPointInfo->u.chOldByte;
		}
	}

	nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//���÷��������

	//����JMP �� CALL ָ����Ҫ������ַ�� CALL ����Ҫ���� ģ���� + ������
	printf("%p    %s %s %s", m_lpDisAsmAddr, da.dump, 
		"                        " + strlen(da.dump), da.result);

	char chCall[5] = {0};
	char chJmp[2] = {0};
	memcpy(chCall, da.result, 4);
	memcpy(chJmp, da.result, 1);

	if (stricmp(chCall, "CALL") == 0 || 
		stricmp(chJmp, "J") == 0 || 
		da.result[strlen(da.result)-1] == ']')
	{
		ShowFunctionName(da.result);
	}
	printf("\r\n");

	// lpDisAsmAddr ��ַҪ����ƶ�
	m_lpDisAsmAddr = (LPVOID)(nCodelen + (int)m_lpDisAsmAddr);

	bRet = FALSE;
}

//��ʾ���з������뺯��
BOOL CHandleException::ShowMulAsmCode(CCCommand* pCmd)
{
	LPVOID lpAddr = HexStringToHex(pCmd->chParam1, TRUE);
	if (lpAddr != 0)
	{
		m_lpDisAsmAddr = lpAddr;
	}
	for (int i = 0; i < 8; i++)
	{
		ShowAsmCode();
	}
	return FALSE;
}





BOOL CHandleException::ShowFunctionName(char *pResult)
{
	int nLen = strlen(pResult);
	char* pAddr;
	BOOL bRet;
	DWORD dwFunAddr = 0;

	//�Ӻ���ǰ������ �ո� �� ���� �ĵط�
	while (pResult[nLen-1] != ' ' && pResult[nLen-1] != ',' && nLen > 0)
	{
		nLen--;
	}
	pAddr = &pResult[nLen];

	//�Ƿ���������[]
	if (pAddr[0] == '[' && pAddr[strlen(pAddr)-1] == ']')
	{
		//ȥ��������֮���Ƿ��ǺϷ��ĵ�ַ
		pAddr[strlen(pAddr)-1] = '\0';
		pAddr = &pAddr[1];
		LPVOID dwAddr = HexStringToHex(pAddr, FALSE);

		if (dwAddr == 0)
		{
			return FALSE;
		}

		//����ַ��ȥȡ����
		DWORD dwOldProtect;
		DWORD dwNoUseProtect;
		VirtualProtectEx(m_hProcess, dwAddr, 1, PAGE_READWRITE, &dwOldProtect);
		bRet = ReadProcessMemory(m_hProcess, dwAddr, &dwFunAddr, 4, NULL);
		VirtualProtectEx(m_hProcess, dwAddr, 1, dwOldProtect, &dwNoUseProtect);
		if (bRet == FALSE)
		{
			printf("ReadProcessMemory error");
			return FALSE;
		}
	} 
	else
	{
		//ֱ���жϵ�ַ���Ƿ���API
		LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
		if (dwAddr == 0)
		{
			return FALSE;
		}
		dwFunAddr = (DWORD)dwAddr;
	}

	//�ж� dwFunAddr �Ƿ���API��ַ
	//�����ж� dwFunAddr ���ĸ�ģ��ĵ�ַ
	BOOL isHit = FALSE;
	CCDllInfo* pDllInfo = NULL;
	list<CCDllInfo*>::iterator it = g_DllList.begin();
	for (int i = 0; i < g_DllList.size(); i++)
	{
		pDllInfo = *it;
		if (dwFunAddr > pDllInfo->dwDllAddr && 
			dwFunAddr < pDllInfo->dwDllAddr + pDllInfo->dwModSize)
		{
			isHit = TRUE;
			break;
		}
		it++;
	}

	//���û���ҵ���Ӧ��ģ�飬��ɾ����ģ�������е�ģ���¼
	//����ö��ģ�飬���²��Ҷ�Ӧģ��
	if (isHit == FALSE)
	{
		list<CCDllInfo*>::iterator itDll = g_DllList.begin();
		while (itDll != g_DllList.end())
		{
			CCDllInfo* pDll = *itDll;
			itDll++;
			delete pDll;
			g_DllList.remove(pDll);
		}

		EnumDestMod();

		it = g_DllList.begin();
		for (int i = 0; i < g_DllList.size(); i++)
		{
			pDllInfo = *it;
			if (dwFunAddr > pDllInfo->dwDllAddr && 
				dwFunAddr < pDllInfo->dwDllAddr + pDllInfo->dwModSize)
			{
				isHit = TRUE;
				break;
			}
			it++;
		}

	}

	if (isHit == FALSE)
	{
		return FALSE;
	}

	//���������Ƿ�����ĳ������
	char chFuncName[MAXBYTE] = {0};
	isHit = FindFunction(dwFunAddr, pDllInfo->dwDllAddr, chFuncName);

	if (isHit == TRUE)
	{
		printf("(%s)", chFuncName);
		return TRUE;
	}

	//���CALL���ĵ�ַ��һ����ת��JMP��CALL���ٽ������JMP��CALL�ĵ�ַ
	char            CodeBuf[20];
	t_disasm        da;
	int             nCodelen;
	BOOL            isNeedResetFirstPage = FALSE;
	BOOL            isNeedResetSecondPage = FALSE;
	DWORD           dwTempProtect1;
	DWORD           dwTempProtect2;
	DWORD           dwOldProtect;

	//�鿴Ҫ��������ĵ�ַ���ڵ��ڴ��ҳ�Ƿ��Ѿ����ڴ�ϵ�
	//����У����޸��ڴ�����ҳΪ�ɶ�������֮���ٸ�Ϊ���ɷ���
	//ע�⣬�����ڴ���ܿ��ҳ
	if (FindRecordInPointPageList((DWORD)dwFunAddr & 0xfffff000))
	{
		VirtualProtectEx(m_hProcess, (LPVOID)dwFunAddr,
			1, PAGE_READONLY, &dwTempProtect1);
		isNeedResetFirstPage = TRUE;
	}

	if (FindRecordInPointPageList(((DWORD)dwFunAddr + 20) & 0xfffff000))
	{
		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)dwFunAddr+20),
			1, PAGE_READONLY, &dwTempProtect2);
		isNeedResetSecondPage = TRUE;
	}

	bRet = ReadProcessMemory(m_hProcess, (LPVOID)dwFunAddr, CodeBuf, 20, NULL);

	//����֮������ϵ㣬����Ҫע�⣬���� m_lpDisAsmAddr �� m_lpDisAsmAddr+20
	//������ͬһ����ҳ�ϣ��� SecondPage �� FirstPage��ͬһ����ҳ
	//�����Ȼָ� SecondPage����ָ� FirstPage
	if (isNeedResetSecondPage)
	{
		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)dwFunAddr+20),
			1, dwTempProtect2, &dwOldProtect);
		isNeedResetSecondPage = FALSE;
	}

	if (isNeedResetFirstPage)
	{
		VirtualProtectEx(m_hProcess, (LPVOID)dwFunAddr,
			1, dwTempProtect1, &dwOldProtect);
		isNeedResetFirstPage = FALSE;
	}

	if (bRet == FALSE)
	{
		printf("ReadProcessMemory error!\r\n");
		return FALSE;
	}

	//����������ڴ��ֽ�����0xCC��ͷ�ģ���鿴���0xCC�Ƿ����û��µ�����ϵ�
	if (CodeBuf[0] == 0xCC)
	{
		CCPointInfo tempPointInfo;
		CCPointInfo* pResultPointInfo = NULL;
		memset(&tempPointInfo, 0, sizeof(CCPointInfo));
		tempPointInfo.lpPointAddr = m_lpDisAsmAddr;
		tempPointInfo.ptType = ORD_POINT;

		if(FindPointInList(tempPointInfo, &pResultPointInfo, FALSE))
		{
			CodeBuf[0] = pResultPointInfo->u.chOldByte;
		}
	}

	nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, dwFunAddr);//���÷��������

	//����JMP �� CALL ָ����Ҫ������ַ�� CALL ����Ҫ���� ģ���� + ������
	char chCall[5] = {0};
	char chJmp[2] = {0};
	memcpy(chCall, da.result, 4);
	memcpy(chJmp, da.result, 1);
	if (stricmp(chCall, "CALL") == 0 || stricmp(chJmp, "J") == 0)
	{
		printf("(%s)", da.result);
		return ShowFunctionName(da.result);
	}
	return FALSE;
}




//���Һ�������
BOOL CHandleException::FindFunction(DWORD dwFunAddr, DWORD dwDllAddr, char* pFunName)
{
	DWORD dwNtHeaderRva = 0;
	DWORD dwOldProtect;
	DWORD dwNoUseProtect;
	BOOL bRet;
	VirtualProtectEx(m_hProcess, (LPVOID)dwDllAddr, 1, PAGE_READWRITE, &dwOldProtect);
	bRet = ReadProcessMemory(m_hProcess, 
		(LPVOID)((DWORD)dwDllAddr + 0x3c), &dwNtHeaderRva, 4, NULL);  //NtHeader

	if (bRet == FALSE)
	{
		printf("FindFunction ReadProcessMemory error!\r\n");
		return FALSE;
	}

	DWORD pNtHeader = (DWORD)dwDllAddr + dwNtHeaderRva;

	DWORD dwRva = 0;
	bRet = ReadProcessMemory(m_hProcess, 
		(LPVOID)(pNtHeader + 0x78), &dwRva, 4, NULL);  //�������ַ
	if (bRet == FALSE)
	{
		printf("FindFunction ReadProcessMemory error!\r\n");
		return FALSE;
	}

	VirtualProtectEx(m_hProcess, (LPVOID)dwDllAddr, 1, dwOldProtect, &dwNoUseProtect);

	if (dwRva == 0)
	{
		return FALSE;
	}

	IMAGE_EXPORT_DIRECTORY ExportDir = {0};
	VirtualProtectEx(m_hProcess, (LPVOID)(dwRva + (DWORD)dwDllAddr),
		1, PAGE_READWRITE, &dwOldProtect);
	bRet = ReadProcessMemory(m_hProcess, 
		(LPVOID)(dwRva + (DWORD)dwDllAddr), 
		&ExportDir, sizeof(IMAGE_EXPORT_DIRECTORY), NULL);
	if (bRet == FALSE)
	{
		printf("FindFunction ReadProcessMemory error!\r\n");
		return FALSE;
	}
	VirtualProtectEx(m_hProcess, (LPVOID)(dwRva + (DWORD)dwDllAddr),
		1, dwOldProtect, &dwNoUseProtect);

	DWORD dwRvaFunAddr = ExportDir.AddressOfFunctions;

	int i;
	BOOL isHit = FALSE;
	for (i = 0; i < ExportDir.NumberOfFunctions; i++)
	{
		DWORD dwRvaReadFunAddr = 0;
		VirtualProtectEx(m_hProcess, (LPVOID)(dwRvaFunAddr + (DWORD)dwDllAddr + 4*i),
			1, PAGE_READWRITE, &dwOldProtect);
		bRet = ReadProcessMemory(m_hProcess, 
			(LPVOID)(dwRvaFunAddr + (DWORD)dwDllAddr + 4*i), 
			&dwRvaReadFunAddr, 4, NULL);
		if (bRet == FALSE)
		{
			printf("FindFunction ReadProcessMemory error!\r\n");
			return FALSE;
		}
		VirtualProtectEx(m_hProcess, (LPVOID)(dwRvaFunAddr + (DWORD)dwDllAddr + 4*i),
			1, dwOldProtect, &dwNoUseProtect);

		if (dwFunAddr == dwRvaReadFunAddr + (DWORD)dwDllAddr)
		{
			isHit = TRUE;
			break;
		}
	}
	if (isHit == FALSE)
	{
		return FALSE;
	}
	//������� i �ҵ�������Ŷ�Ӧ���±�j   �ҵ�������
	int j = 0;
	WORD dwNameOrd = 0;
	BOOL isHaveName = FALSE;    //�Ƿ��к�����
	for(; j < ExportDir.NumberOfNames; j++)
	{
		VirtualProtectEx(m_hProcess, 
			(LPVOID)(ExportDir.AddressOfNameOrdinals + (DWORD)dwDllAddr + 2*j), 
			1, PAGE_READWRITE, &dwOldProtect);
		bRet = ReadProcessMemory(m_hProcess, 
			(LPVOID)(ExportDir.AddressOfNameOrdinals + (DWORD)dwDllAddr + 2*j), 
			&dwNameOrd, 4, NULL);
		if (bRet == FALSE)
		{
			printf("FindFunction ReadProcessMemory error!\r\n");
			return FALSE;
		}
		VirtualProtectEx(m_hProcess, 
			(LPVOID)(ExportDir.AddressOfNameOrdinals + (DWORD)dwDllAddr + 2*j), 
			1, dwOldProtect, &dwNoUseProtect);

		if ( (dwNameOrd == i))
		{
			isHaveName = TRUE;
			break;
		}
	}

	if (isHaveName)
	{
		DWORD dwRvaFunNameAddr = 0;
		VirtualProtectEx(m_hProcess, 
			(LPVOID)(ExportDir.AddressOfNames + (DWORD)dwDllAddr + (j)*4), 
			1, PAGE_READWRITE, &dwOldProtect);
		bRet = ReadProcessMemory(m_hProcess, 
			(LPVOID)(ExportDir.AddressOfNames + (DWORD)dwDllAddr + (j)*4), 
			&dwRvaFunNameAddr, 4, NULL);
		if (bRet == FALSE)
		{
			printf("FindFunction ReadProcessMemory error!\r\n");
			return FALSE;
		}
		VirtualProtectEx(m_hProcess, 
			(LPVOID)(ExportDir.AddressOfNames + (DWORD)dwDllAddr + (j)*4), 
			1, dwOldProtect, &dwNoUseProtect);

		VirtualProtectEx(m_hProcess, 
			(LPVOID)(dwRvaFunNameAddr + (DWORD)dwDllAddr), 
			1, PAGE_READWRITE, &dwOldProtect);
		bRet = ReadProcessMemory(m_hProcess, 
			(LPVOID)(dwRvaFunNameAddr + (DWORD)dwDllAddr), 
			pFunName, MAXBYTE, NULL);
		if (bRet == FALSE)
		{
			printf("FindFunction ReadProcessMemory error!\r\n");
			return FALSE;
		}
		VirtualProtectEx(m_hProcess, 
			(LPVOID)(dwRvaFunNameAddr + (DWORD)dwDllAddr), 
			1, dwOldProtect, &dwNoUseProtect);
	}
	else //��ŷ�ʽ�ĺ���
	{
		wsprintf(pFunName, "#%d", i + ExportDir.Base);
	}

	char DllName[MAXBYTE] = {0};
	VirtualProtectEx(m_hProcess, 
		(LPVOID)(ExportDir.Name + (DWORD)dwDllAddr), 
		1, PAGE_READWRITE, &dwOldProtect);
	bRet = ReadProcessMemory(m_hProcess, 
		(LPVOID)(ExportDir.Name + (DWORD)dwDllAddr), 
		DllName, MAXBYTE, NULL);
	if (bRet == FALSE)
	{
		printf("FindFunction ReadProcessMemory error!\r\n");
		return FALSE;
	}
	VirtualProtectEx(m_hProcess, 
		(LPVOID)(ExportDir.Name + (DWORD)dwDllAddr), 
		1, dwOldProtect, &dwNoUseProtect);

	i = 0;
	while (DllName[i] != '.')
	{
		i++;
	}
	DllName[++i] = '\0';
	strcat(DllName, pFunName);
	memcpy(pFunName, DllName, MAXBYTE);
	return TRUE;
}
