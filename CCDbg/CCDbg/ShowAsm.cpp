#include "stdafx.h"
#include "CHandleException.h"


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
// 	if (FindRecordInPointPageList((DWORD)m_lpDisAsmAddr & 0xfffff000))
// 	{
// 		VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
// 			1, PAGE_READONLY, &dwTempProtect1);
// 		isNeedResetFirstPage = TRUE;
// 	}
// 
// 	if (FindRecordInPointPageList(((DWORD)m_lpDisAsmAddr + 20) & 0xfffff000))
// 	{
// 		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
// 			1, PAGE_READONLY, &dwTempProtect2);
// 		isNeedResetSecondPage = TRUE;
// 	}

	bRet = ReadProcessMemory(m_hProcess, m_lpDisAsmAddr, CodeBuf, 20, NULL);

	//读完之后重设断点，这里要注意，可能 m_lpDisAsmAddr 和 m_lpDisAsmAddr+20
	//还是在同一个分页上，即 SecondPage 和 FirstPage是同一个分页
	//所以先恢复 SecondPage，后恢复 FirstPage
// 	if (isNeedResetSecondPage)
// 	{
// 		VirtualProtectEx(m_hProcess, (LPVOID)((DWORD)m_lpDisAsmAddr+20),
// 			1, dwTempProtect2, &dwOldProtect);
// 		isNeedResetSecondPage = FALSE;
// 	}
// 
// 	if (isNeedResetFirstPage)
// 	{
// 		VirtualProtectEx(m_hProcess, m_lpDisAsmAddr,
// 			1, dwTempProtect1, &dwOldProtect);
// 		isNeedResetFirstPage = FALSE;
// 	}
// 
// 	if (bRet == FALSE)
// 	{
// 		printf("ReadProcessMemory error!\r\n");
// 		return;
// 	}

	//如果读到的内存字节是以0xCC开头的，则查看这个0xCC是否是用户下的软件断点
	if (CodeBuf[0] == 0xCC)
	{
		stuPointInfo tempPointInfo;
		stuPointInfo* pResultPointInfo = NULL;
		memset(&tempPointInfo, 0, sizeof(stuPointInfo));
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

// 	if (stricmp(chCall, "CALL") == 0 || 
// 		stricmp(chJmp, "J") == 0 || 
// 		da.result[strlen(da.result)-1] == ']')
// 	{
// 		ShowFunctionName(da.result);
// 	}
	printf("\r\n");

	// lpDisAsmAddr 地址要向后移动
	m_lpDisAsmAddr = (LPVOID)(nCodelen + (int)m_lpDisAsmAddr);

	bRet = FALSE;
}

//显示多行反汇编代码函数
BOOL CHandleException::ShowMulAsmCode(StuCommand* pCmd)
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
