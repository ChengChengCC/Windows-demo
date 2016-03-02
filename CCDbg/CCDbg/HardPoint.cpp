#include "stdafx.h"
#include "Common.h"

#include "CHandleException.h"
/*
DR0~DR3   -----调试地址寄存器
DR4~DR5   -----保留
DR6       -----调试状态寄存器   指示哪个调试寄存器被命中
DR7       -----调试控制寄存器   

长度： 00 1字节长
       01 2字节长
	   11 4字节长
	   10 8字节长（奔腾4或至强CPU），其他处理器未定义
	   这里将长度限制为只能是1，2，4

均使用的是L0~L3,局部断点启动，针对当前任务
*/




//设置硬件断点
BOOL CHandleException::SetHardPoint(CCCommand* pCmd)
{
	if(!UpdateContextFromThread())
	{
		return FALSE;
	}

	if (m_nHardPtNum == 4)  //硬件断点最多4个Dr0~Dr3
	{
		printf("Warning:Current Hardware breakpoint is full!\r\n");
		printf("        You can delete some Hardware breakpoint!\r\n");
		return FALSE; 
	}
	LPVOID lpAddr = HexStringToHex(pCmd->chParam1, TRUE); //地址

	if (lpAddr == 0)
	{
		printf("Need valid parameter!\r\n");
		return FALSE;
	}

	CCPointInfo PointInfo;
	memset(&PointInfo, 0, sizeof(CCPointInfo));
	PointInfo.lpPointAddr = lpAddr;
	PointInfo.ptType = HARD_POINT;

	if (stricmp("access", pCmd->chParam2) == 0)  //硬件访问断点
	{
		PointInfo.ptAccess = ACCESS;
	} 
	else if (stricmp("write", pCmd->chParam2) == 0)   //硬件读写断点
	{
		PointInfo.ptAccess = WRITE;
	}
	else if (stricmp("execute", pCmd->chParam2) == 0)  //硬件执行断点
	{
		PointInfo.ptAccess = EXECUTE;
	}
	else
	{
		printf("Void access!\r\n");
		return FALSE;
	}
	/*
	Dr7的控制位决定
	*/
	int nLen = (int)HexStringToHex(pCmd->chParam3, TRUE);  //长度
	if (nLen != 0 && PointInfo.ptAccess == EXECUTE) //硬件执行断点不需要长度
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
	int nDrNum = -1;                 // 硬件断点是在下标为nDrNum的 DRX 寄存器上
	int nPointLen = -1;

	//如果硬件断点数量>0
	if (m_nHardPtNum > 0)
	{
		//确定当前context中是佛已经存在目标地址的硬件断点
		if(FindPointInConext(PointInfo, &nDrNum, &nPointLen))
		{
			//找到了，需要比较一下找到的断点的字节长度是否比要设置的断点字节长度长
			if (nPointLen >= nLen)
			{
				//如果找到的断点字节长度大于需要设置的新断点，
				//则不需要重新设置，直接返回
				printf("The Hardware breakpoint is exist!\r\n");
				return FALSE;
			}
			else //否则硬件断点需要重新设置，但硬件断点数量维持不变，所以这里先将数量减1
			{
				m_nHardPtNum--;
			}
		}
	}

	//没有找到 或 需要重新设置
	if (nDrNum == -1) //需要先找一个空闲的调试寄存器
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

	//根据 nDrNum 设置硬件断点
	switch (nDrNum)
	{
	case 0:
		m_Context.Dr0 = (DWORD)PointInfo.lpPointAddr;
		m_Context.Dr7 |= 1;
		//R/W0:16 17    LEN0:18 19
		m_Context.Dr7 &= 0xfff0ffff;//清掉第16，17，18，19位（置0）
		//LEN 长度
		//00 1字节
		//01 2字节
		//10 8字节(奔腾4或者至强)或者未定义(其他的cpu)
		//11 4字节
		switch (PointInfo.dwPointLen)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			m_Context.Dr7 |= 0x00040000;//18位 置1
			break;
		case 4:
			m_Context.Dr7 |= 0x000c0000;//18位 置1, 19位 置1
			break;
		}
		switch (PointInfo.ptAccess)
		{
		case EXECUTE:
			break;
		case WRITE:
			m_Context.Dr7 |= 0x00010000;//16位 置1
			break;
		case ACCESS:
			m_Context.Dr7 |= 0x00030000;//16,17位 置1
			break;
		}
		break;
	case 1:
		m_Context.Dr1 = (DWORD)PointInfo.lpPointAddr;
		m_Context.Dr7 |= 4;
		m_Context.Dr7 &= 0xff0fffff;//清掉第20，21，22，23位（置0）
		//LEN 长度
		switch (PointInfo.dwPointLen)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			m_Context.Dr7 |= 0x00400000;//22位 置1
			break;
		case 4:
			m_Context.Dr7 |= 0x00c00000;//22位 置1, 23位 置1
			break;
		}
		switch (PointInfo.ptAccess)
		{
		case EXECUTE:
			break;
		case WRITE:
			m_Context.Dr7 |= 0x00100000;//20位 置1
			break;
		case ACCESS:
			m_Context.Dr7 |= 0x00300000;//20,21位 置1
			break;
		}
		break;
	case 2:
		m_Context.Dr2 = (DWORD)PointInfo.lpPointAddr;
		m_Context.Dr7 |= 0x10;
		m_Context.Dr7 &= 0xf0ffffff;//清掉第24，25，26，27位（置0）
		//LEN 长度
		switch (PointInfo.dwPointLen)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			m_Context.Dr7 |= 0x04000000;//26位 置1
			break;
		case 4:
			m_Context.Dr7 |= 0x0c000000;//26位 置1, 27位 置1
			break;
		}
		switch (PointInfo.ptAccess)
		{
		case EXECUTE:
			break;
		case WRITE:
			m_Context.Dr7 |= 0x01000000;//24位 置1
			break;
		case ACCESS:
			m_Context.Dr7 |= 0x03000000;//24,25位 置1
			break;
		}
		break;
	case 3:
		m_Context.Dr3 = (DWORD)PointInfo.lpPointAddr;
		m_Context.Dr7 |= 0x40;
		m_Context.Dr7 &= 0x0fffffff;//清掉第28，29，30，31位（置0）
		//LEN 长度
		switch (PointInfo.dwPointLen)
		{
		case 0:
			break;
		case 1:
			break;
		case 2:
			m_Context.Dr7 |= 0x40000000;//30位 置1
			break;
		case 4:
			m_Context.Dr7 |= 0xc0000000;//30位 置1, 31位 置1
			break;
		}
		switch (PointInfo.ptAccess)
		{
		case EXECUTE:
			break;
		case WRITE:
			m_Context.Dr7 |= 0x10000000;//28位 置1
			break;
		case ACCESS:
			m_Context.Dr7 |= 0x30000000;//28,29位 置1
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



//硬件断点列表
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

//硬件断点清除
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

	//如果要重设的硬件断点序号 m_nNeedResetHardPoint 等于要删除的硬件断点序号，
	//则硬件断点不需重设
	if (nID == m_nNeedResetHardPoint)
	{
		m_isNeedResetHardPoint = FALSE;
	}

	UpdateContextToThread();
	m_nHardPtNum--;
	printf("Clear the %d Hardware breakpoint.\r\n", nID+1);
	return FALSE;
}


//在当前线程context中查找是否已经存在 PointInfo 指定的硬件断点
//返回TRUE表示找到，FALSE表示未找到
//参数nDrNum返回找到的DRX寄存器的下标，参数nPointLen返回找到的断点长度
BOOL CHandleException::FindPointInConext(CCPointInfo PointInfo, 
	int *nDrNum, int *nPointLen)
{
	//L0为1 0位 ，使用Dr0寄存器设置断点
	if((m_Context.Dr7 & 1) && m_Context.Dr0 == (DWORD)PointInfo.lpPointAddr)
	{							//左移14，右移30，取17,16两位R/W0 
		int nAccess = (m_Context.Dr7 << 14) >> 30;  //断点触发条件
		if (((PointInfo.ptAccess == EXECUTE) && (nAccess == 0)) ||
			((PointInfo.ptAccess == WRITE) && (nAccess == 1)) ||
			((PointInfo.ptAccess == ACCESS) && (nAccess == 3)) //读写中断，执行除外
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
	//L1为1 2位        0x0100
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