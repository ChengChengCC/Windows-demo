#include "stdafx.h"
#include "CHandleException.h"


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

	//����֮������ϵ㣬����Ҫע�⣬���� m_lpDisAsmAddr �� m_lpDisAsmAddr+20
	//������ͬһ����ҳ�ϣ��� SecondPage �� FirstPage��ͬһ����ҳ
	//�����Ȼָ� SecondPage����ָ� FirstPage
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

	//����������ڴ��ֽ�����0xCC��ͷ�ģ���鿴���0xCC�Ƿ����û��µ�����ϵ�
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

	nCodelen= Disasm(CodeBuf, 20, 0, &da, DISASM_CODE, (ulong)m_lpDisAsmAddr);//���÷��������

	//����JMP �� CALL ָ����Ҫ������ַ�� CALL ����Ҫ���� ģ���� + ������
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

	// lpDisAsmAddr ��ַҪ����ƶ�
	m_lpDisAsmAddr = (LPVOID)(nCodelen + (int)m_lpDisAsmAddr);

	bRet = FALSE;
}

//��ʾ���з������뺯��
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
