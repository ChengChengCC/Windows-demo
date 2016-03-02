#include "stdafx.h"
#include "Common.h"

#include "CHandleException.h"
/*
DR0~DR3   -----���Ե�ַ�Ĵ���
DR4~DR5   -----����
DR6       -----����״̬�Ĵ���   ָʾ�ĸ����ԼĴ���������
DR7       -----���Կ��ƼĴ���   

���ȣ� 00 1�ֽڳ�
       01 2�ֽڳ�
	   11 4�ֽڳ�
	   10 8�ֽڳ�������4����ǿCPU��������������δ����
	   ���ｫ��������Ϊֻ����1��2��4

��ʹ�õ���L0~L3,�ֲ��ϵ���������Ե�ǰ����
*/




//����Ӳ���ϵ�
BOOL CHandleException::SetHardPoint(CCCommand* pCmd)
{
	if(!UpdateContextFromThread())
	{
		return FALSE;
	}

	if (m_nHardPtNum == 4)  //Ӳ���ϵ����4��Dr0~Dr3
	{
		printf("Warning:Current Hardware breakpoint is full!\r\n");
		printf("        You can delete some Hardware breakpoint!\r\n");
		return FALSE; 
	}
	LPVOID lpAddr = HexStringToHex(pCmd->chParam1, TRUE); //��ַ

	if (lpAddr == 0)
	{
		printf("Need valid parameter!\r\n");
		return FALSE;
	}

	CCPointInfo PointInfo;
	memset(&PointInfo, 0, sizeof(CCPointInfo));
	PointInfo.lpPointAddr = lpAddr;
	PointInfo.ptType = HARD_POINT;

	if (stricmp("access", pCmd->chParam2) == 0)  //Ӳ�����ʶϵ�
	{
		PointInfo.ptAccess = ACCESS;
	} 
	else if (stricmp("write", pCmd->chParam2) == 0)   //Ӳ����д�ϵ�
	{
		PointInfo.ptAccess = WRITE;
	}
	else if (stricmp("execute", pCmd->chParam2) == 0)  //Ӳ��ִ�жϵ�
	{
		PointInfo.ptAccess = EXECUTE;
	}
	else
	{
		printf("Void access!\r\n");
		return FALSE;
	}
	/*
	Dr7�Ŀ���λ����
	*/
	int nLen = (int)HexStringToHex(pCmd->chParam3, TRUE);  //����
	if (nLen != 0 && PointInfo.ptAccess == EXECUTE) //Ӳ��ִ�жϵ㲻��Ҫ����
	{
		printf("Point length error!\r\n");
		return FALSE;
	}
	if (nLen != 0 && nLen != 1 && nLen != 2 && nLen != 4)
	{
		printf("Point length error!\r\n");
		return FALSE;
	}

	PointInfo.dwPointLen = nLen;
	int nDrNum = -1;                 // Ӳ���ϵ������±�ΪnDrNum�� DRX �Ĵ�����
	int nPointLen = -1;

	//���Ӳ���ϵ�����>0
	if (m_nHardPtNum > 0)
	{
		//ȷ����ǰcontext���Ƿ��Ѿ�����Ŀ���ַ��Ӳ���ϵ�
		if(FindPointInConext(PointInfo, &nDrNum, &nPointLen))
		{
			//�ҵ��ˣ���Ҫ�Ƚ�һ���ҵ��Ķϵ���ֽڳ����Ƿ��Ҫ���õĶϵ��ֽڳ��ȳ�
			if (nPointLen >= nLen)
			{
				//����ҵ��Ķϵ��ֽڳ��ȴ�����Ҫ���õ��¶ϵ㣬
				//����Ҫ�������ã�ֱ�ӷ���
				printf("The Hardware breakpoint is exist!\r\n");
				return FALSE;
			}
			else //����Ӳ���ϵ���Ҫ�������ã���Ӳ���ϵ�����ά�ֲ��䣬���������Ƚ�������1
			{
				m_nHardPtNum--;
			}
		}
	}

	//û���ҵ� �� ��Ҫ��������
	if (nDrNum == -1) //��Ҫ����һ�����еĵ��ԼĴ���
	{
		if((m_Context.Dr7 & 1) == 0)
		{
			nDrNum = 0;
		}
		else if ((m_Context.Dr7 & 0x4) == 0)
		{
			nDrNum = 1;
		}
		else if ((m_Context.Dr7 & 0x10) == 0)
		{
			nDrNum = 2;
		}
		else
		{
			nDrNum = 3;
		}
	}

	//���� nDrNum ����Ӳ���ϵ�
	switch (nDrNum)
	{
	case 0:
		m_Context.Dr0 = (DWORD)PointInfo.lpPointAddr;
		m_Context.Dr7 |= 1;
		//R/W0:16 17    LEN0:18 19
		m_Context.Dr7 &= 0xfff0ffff;//�����16��17��18��19λ����0��
		//LEN ����
		//00 1�ֽ�
		//01 2�ֽ�
		//10 8�ֽ�(����4������ǿ)����δ����(������cpu)
		//11 4�ֽ�
		switch (PointInfo.dwPointLen)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			m_Context.Dr7 |= 0x00040000;//18λ ��1
			break;
		case 4:
			m_Context.Dr7 |= 0x000c0000;//18λ ��1, 19λ ��1
			break;
		}
		switch (PointInfo.ptAccess)
		{
		case EXECUTE:
			break;
		case WRITE:
			m_Context.Dr7 |= 0x00010000;//16λ ��1
			break;
		case ACCESS:
			m_Context.Dr7 |= 0x00030000;//16,17λ ��1
			break;
		}
		break;
	case 1:
		m_Context.Dr1 = (DWORD)PointInfo.lpPointAddr;
		m_Context.Dr7 |= 4;
		m_Context.Dr7 &= 0xff0fffff;//�����20��21��22��23λ����0��
		//LEN ����
		switch (PointInfo.dwPointLen)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			m_Context.Dr7 |= 0x00400000;//22λ ��1
			break;
		case 4:
			m_Context.Dr7 |= 0x00c00000;//22λ ��1, 23λ ��1
			break;
		}
		switch (PointInfo.ptAccess)
		{
		case EXECUTE:
			break;
		case WRITE:
			m_Context.Dr7 |= 0x00100000;//20λ ��1
			break;
		case ACCESS:
			m_Context.Dr7 |= 0x00300000;//20,21λ ��1
			break;
		}
		break;
	case 2:
		m_Context.Dr2 = (DWORD)PointInfo.lpPointAddr;
		m_Context.Dr7 |= 0x10;
		m_Context.Dr7 &= 0xf0ffffff;//�����24��25��26��27λ����0��
		//LEN ����
		switch (PointInfo.dwPointLen)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			m_Context.Dr7 |= 0x04000000;//26λ ��1
			break;
		case 4:
			m_Context.Dr7 |= 0x0c000000;//26λ ��1, 27λ ��1
			break;
		}
		switch (PointInfo.ptAccess)
		{
		case EXECUTE:
			break;
		case WRITE:
			m_Context.Dr7 |= 0x01000000;//24λ ��1
			break;
		case ACCESS:
			m_Context.Dr7 |= 0x03000000;//24,25λ ��1
			break;
		}
		break;
	case 3:
		m_Context.Dr3 = (DWORD)PointInfo.lpPointAddr;
		m_Context.Dr7 |= 0x40;
		m_Context.Dr7 &= 0x0fffffff;//�����28��29��30��31λ����0��
		//LEN ����
		switch (PointInfo.dwPointLen)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			m_Context.Dr7 |= 0x40000000;//30λ ��1
			break;
		case 4:
			m_Context.Dr7 |= 0xc0000000;//30λ ��1, 31λ ��1
			break;
		}
		switch (PointInfo.ptAccess)
		{
		case EXECUTE:
			break;
		case WRITE:
			m_Context.Dr7 |= 0x10000000;//28λ ��1
			break;
		case ACCESS:
			m_Context.Dr7 |= 0x30000000;//28,29λ ��1
			break;
		}
		break;
	}

	if(!UpdateContextToThread())
	{
		return FALSE;
	}

	m_nHardPtNum++;
	printf("***Set Hard breakpoint success!***\r\n");
	return FALSE;
}



//Ӳ���ϵ��б�
BOOL CHandleException::ListHardPoint(CCCommand* pCmd)
{
	printf("------------------------------------------------------------\r\n");
	printf("ID    Breakpoint type       Address     Access type   Length\r\n");
	if (m_nHardPtNum == 0)
	{
		return FALSE;
	}
	for (int i = 0; i < 4; i++)
	{
		if((m_Context.Dr7 & (int)pow((double)4, i)) != 0 || 
			(m_isNeedResetHardPoint == TRUE && m_nNeedResetHardPoint == i) )
		{
			printf("%d     Hardware breakpoint   ", i+1);
			switch (i)
			{
			case 0:
				printf("0x%p", m_Context.Dr0);
				break;
			case 1:
				printf("0x%p", m_Context.Dr1);
				break;
			case 2:
				printf("0x%p", m_Context.Dr2);
				break;
			case 3:
				printf("0x%p", m_Context.Dr3);
				break;
			}
			switch ((m_Context.Dr7 << (14 - 4*i)) >> 30)
			{
			case 0:
				printf("  EXECUTE");
				break;
			case 1:
				printf("  WRITE  ");
				break;
			case 3:
				printf("  ACCESS ");
				break;
			}
			printf("       ");
			switch ((m_Context.Dr7 << (12 - 4*i)) >> 30)
			{
			case 0:
				printf("1");
				break;
			case 1:
				printf("2");
				break;
			case 3:
				printf("4");
				break;
			}
			printf("\r\n");

		}
	}
	printf("------------------------------------------------------------\r\n");
	return FALSE;
}

//Ӳ���ϵ����
BOOL CHandleException::ClearHardPoint(CCCommand* pCmd)
{
	int nID = (int)HexStringToHex(pCmd->chParam1, TRUE);
	if (nID < 1 || nID > 4)
	{
		printf("Need valid ID!\r\n");
		return FALSE;
	}

	nID--;
	if ((m_Context.Dr7 & (int)pow((double)4, nID)) == 0 && 
		(nID != m_nNeedResetHardPoint || m_isNeedResetHardPoint == FALSE))
	{
		printf("Can not find the Hardware breakpoint!\r\n");
		return FALSE;
	}

	m_Context.Dr7 &= ~(int)pow((double)4, nID);

	//���Ҫ�����Ӳ���ϵ���� m_nNeedResetHardPoint ����Ҫɾ����Ӳ���ϵ���ţ�
	//��Ӳ���ϵ㲻������
	if (nID == m_nNeedResetHardPoint)
	{
		m_isNeedResetHardPoint = FALSE;
	}

	UpdateContextToThread();
	m_nHardPtNum--;
	printf("Clear the %d Hardware breakpoint.\r\n", nID+1);
	return FALSE;
}


//�ڵ�ǰ�߳�context�в����Ƿ��Ѿ����� PointInfo ָ����Ӳ���ϵ�
//����TRUE��ʾ�ҵ���FALSE��ʾδ�ҵ�
//����nDrNum�����ҵ���DRX�Ĵ������±꣬����nPointLen�����ҵ��Ķϵ㳤��
BOOL CHandleException::FindPointInConext(CCPointInfo PointInfo, 
	int *nDrNum, int *nPointLen)
{
	//L0Ϊ1 0λ ��ʹ��Dr0�Ĵ������öϵ�
	if((m_Context.Dr7 & 1) && m_Context.Dr0 == (DWORD)PointInfo.lpPointAddr)
	{							//����14������30��ȡ17,16��λR/W0 
		int nAccess = (m_Context.Dr7 << 14) >> 30;  //�ϵ㴥������
		if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
			((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
			((PointInfo.ptAccess == ACCESS) && (nAccess == 3)) //��д�жϣ�ִ�г���
			)
		{
			*nDrNum = 0;
			*nPointLen = ((m_Context.Dr7 << 12) >> 30);
			switch (*nPointLen)
			{
			case 0:
				*nPointLen = 1;
				break;
			case 1:
				*nPointLen = 2;
				break;
			case 3:
				*nPointLen = 4;
				break;
			default:
				printf("error!\r\n");
			}
			return TRUE;
		}
	} 
	//L1Ϊ1 2λ        0x0100
	if((m_Context.Dr7 & 4) && m_Context.Dr1 == (DWORD)PointInfo.lpPointAddr)
	{
		int nAccess = (m_Context.Dr7 << 10) >> 30;
		if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
			((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
			((PointInfo.ptAccess == ACCESS) && (nAccess == 3))
			)
		{
			*nDrNum = 1;
			*nPointLen = ((m_Context.Dr7 << 8) >> 30);
			switch (*nPointLen)
			{
			case 0:
				*nPointLen = 1;
				break;
			case 1:
				*nPointLen = 2;
				break;
			case 3:
				*nPointLen = 4;
				break;
			default:
				printf("error!\r\n");
			}
			return TRUE;
		}
	}
	//                  0x00010000
	if((m_Context.Dr7 & 0x10) && m_Context.Dr2 == (DWORD)PointInfo.lpPointAddr)
	{
		int nAccess = (m_Context.Dr7 << 6) >> 30;
		if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
			((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
			((PointInfo.ptAccess == ACCESS) && (nAccess == 3))
			)
		{
			*nDrNum = 2;
			*nPointLen = ((m_Context.Dr7 << 4) >> 30);
			switch (*nPointLen)
			{
			case 0:
				*nPointLen = 1;
				break;
			case 1:
				*nPointLen = 2;
				break;
			case 3:
				*nPointLen = 4;
				break;
			default:
				printf("error!\r\n");
			}
			return TRUE;
		}
	}
	 //                 
	if((m_Context.Dr7 & 0x40) && m_Context.Dr3 == (DWORD)PointInfo.lpPointAddr)
	{
		int nAccess = (m_Context.Dr7 << 2) >> 30;
		if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
			((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
			((PointInfo.ptAccess == ACCESS) && (nAccess == 3))
			)
		{
			*nDrNum = 3;
			*nPointLen = (m_Context.Dr7 >> 30);
			switch (*nPointLen)
			{
			case 0:
				*nPointLen = 1;
				break;
			case 1:
				*nPointLen = 2;
				break;
			case 3:
				*nPointLen = 4;
				break;
			default:
				printf("error!\r\n");
			}
			return TRUE;
		}
	}

	return FALSE;
}




void CHandleException::ShowHardwareBreakpoint(DWORD dwDr6Low)
{
	printf("***********************************************************\r\n");
	printf("Hit the breakpoint: \r\n");
	int nIdx;
	switch (dwDr6Low)
	{
	case 1:
		nIdx = 0;
		printf("Breakpoint address: %p", m_Context.Dr0);
		break;
	case 2:
		nIdx = 1;
		printf("Breakpoint address: %p", m_Context.Dr1);
		break;
	case 4:
		nIdx = 2;
		printf("Breakpoint address: %p", m_Context.Dr2);
		break;
	case 8:
		nIdx = 3;
		printf("Breakpoint address: %p", m_Context.Dr3);
		break;
	}
	printf(".   Hardware breakpoint at Dr%d.\r\n", nIdx);
	int nType = (m_Context.Dr7 << (14 - (nIdx * 2))) >> 30;
	int nLen = (m_Context.Dr7 << (12 - (nIdx * 2))) >> 30;

	printf("Breakpoint type is");
	switch (nType)
	{
	case 0:
		printf(" EXECUTE.");
		break;
	case 1:
		printf(" WRITE.  ");
		break;
	case 3:
		printf(" ACCESS. ");
		break;
	}
	printf("     Breakpoint length is ");
	switch (nLen)
	{
	case 0:
		printf("1");
		break;
	case 1:
		printf("2");
		break;
	case 3:
		printf("4");
		break;
	}
	printf(".\r\n");
	printf("***********************************************************\r\n");
}