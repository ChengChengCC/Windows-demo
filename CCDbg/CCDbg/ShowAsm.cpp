#include "stdafx.h"
#include "CHandleException.h"



extern list<CCDllInfo*>   g_DllList;                    //模块信息列表


// 从m_lpDisAsmAddr 所指定的位置开始进行反汇编
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

	//查看要反汇编代码的地址所在的内存分页是否已经有内存断点
	//如果有，先修改内存属性页为可读，读完之后再改为不可访问
	//注意，所读内存可能跨分页
	if (FindRecordInPointPageList((DWORD)m_lpDisAsmAddr & 0xfffff000))
	{
		VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
			1, PAGE_READONLY, &dwTempProtect1);
		isNeedResetFirstPage = TRUE;
	}
	//+20的地址是不是内存断点页的基地址
	if (FindRecordInPointPageList(((DWORD)m_lpDisAsmAddr + 20) & 0xfffff000))
	{
		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
			1, PAGE_READONLY, &dwTempProtect2);
		isNeedResetSecondPage = TRUE;
	}

	bRet = ReadProcessMemory(m_hProcess, m_lpDisAsmAddr, CodeBuf, 20, NULL);

	//读完之后恢复属性，这里要注意，可能 m_lpDisAsmAddr 和 m_lpDisAsmAddr+20
	//还是在同一个虚拟内存页上，即 SecondPage 和 FirstPage是同一个分页
	//所以先恢复 SecondPage，后恢复 FirstPage
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

	//如果读到的内存字节是以0xCC开头的，则查看这个0xCC是否是用户下的软件断点
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

	nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//调用反汇编引擎

	//对于JMP 和 CALL 指令需要修正地址， CALL 后面要换成 模块名 + 函数名
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

	// lpDisAsmAddr 地址要向后移动
	m_lpDisAsmAddr = (LPVOID)(nCodelen + (int)m_lpDisAsmAddr);

	bRet = FALSE;
}

//显示多行反汇编代码函数
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

	//从后往前读到有 空格 或 逗号 的地方
	while (pResult[nLen-1] != ' ' && pResult[nLen-1] != ',' && nLen > 0)
	{
		nLen--;
	}
	pAddr = &pResult[nLen];

	//是否有中括号[]
	if (pAddr[0] == '[' && pAddr[strlen(pAddr)-1] == ']')
	{
		//去除中括号之后是否是合法的地址
		pAddr[strlen(pAddr)-1] = '\0';
		pAddr = &pAddr[1];
		LPVOID dwAddr = HexStringToHex(pAddr, FALSE);

		if (dwAddr == 0)
		{
			return FALSE;
		}

		//到地址中去取内容
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
		//直接判断地址，是否是API
		LPVOID dwAddr = HexStringToHex(pAddr, FALSE);
		if (dwAddr == 0)
		{
			return FALSE;
		}
		dwFunAddr = (DWORD)dwAddr;
	}

	//判断 dwFunAddr 是否是API地址
	//首先判断 dwFunAddr 是哪个模块的地址
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

	//如果没有找到对应的模块，则删除掉模块链表中的模块记录
	//重新枚举模块，重新查找对应模块
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

	//读导出表看是否命中某个函数
	char chFuncName[MAXBYTE] = {0};
	isHit = FindFunction(dwFunAddr, pDllInfo->dwDllAddr, chFuncName);

	if (isHit == TRUE)
	{
		printf("(%s)", chFuncName);
		return TRUE;
	}

	//如果CALL到的地址是一个跳转表JMP或CALL，再解析这个JMP、CALL的地址
	char            CodeBuf[20];
	t_disasm        da;
	int             nCodelen;
	BOOL            isNeedResetFirstPage = FALSE;
	BOOL            isNeedResetSecondPage = FALSE;
	DWORD           dwTempProtect1;
	DWORD           dwTempProtect2;
	DWORD           dwOldProtect;

	//查看要反汇编代码的地址所在的内存分页是否已经有内存断点
	//如果有，先修改内存属性页为可读，读完之后再改为不可访问
	//注意，所读内存可能跨分页
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

	//读完之后重设断点，这里要注意，可能 m_lpDisAsmAddr 和 m_lpDisAsmAddr+20
	//还是在同一个分页上，即 SecondPage 和 FirstPage是同一个分页
	//所以先恢复 SecondPage，后恢复 FirstPage
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

	//如果读到的内存字节是以0xCC开头的，则查看这个0xCC是否是用户下的软件断点
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

	nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, dwFunAddr);//调用反汇编引擎

	//对于JMP 和 CALL 指令需要修正地址， CALL 后面要换成 模块名 + 函数名
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




//查找函数名称
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
		(LPVOID)(pNtHeader + 0x78), &dwRva, 4, NULL);  //导出表地址
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
	//根据序号 i 找到函数序号对应的下标j   找到函数名
	int j = 0;
	WORD dwNameOrd = 0;
	BOOL isHaveName = FALSE;    //是否有函数名
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
	else //序号方式的函数
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
