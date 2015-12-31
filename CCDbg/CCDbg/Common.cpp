#include "stdafx.h"


#include "Common.h"







//16进制字符串转换为数值
LPVOID HexStringToHex(char* pHexString, BOOL isShowError)
{
	DWORD dwAddr = 0;
	int nLen = strlen(pHexString);
	int j = 1;
	for (int i = 0; i < nLen; i++)
	{
		char ch = pHexString[nLen-i-1];

		if (ch >= 'A' && ch <= 'F')
		{
			dwAddr += j*(ch - 'A' + 10);
		} 
		else if (ch >= 'a' && ch <= 'f')
		{
			dwAddr += j*(ch - 'a' + 10);
		}
		else if (ch >= '0' && ch <= '9')
		{
			dwAddr += j*(ch - '0' + 0);
		}
		else
		{
			if (isShowError)
			{
				printf("Invoid hex value!\r\n");
			}
			return 0;
		}
		j *= 16;
	}
	return (LPVOID)dwAddr;
}




//显示帮助
BOOL ShowHelp(StuCommand* pCmd)
{
	printf("   ============================ help menu ===========================---=====\n");
	printf("   **************************************************************************\n");
	printf("\
  * 序号 命令名      命令码 英文说明        参数1    参数2    参数3        *\r\n\
  * 1    单步步入      T    step into                                      *\r\n\
  * 2    单步步过      P    step over                                      *\r\n\
  * 3    运行          G    run             地址或无                       *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 4    反汇编        U    assemble        地址或无                       *\r\n\
  * 5    数据          D    data            地址或无                       *\r\n\
  * 6    寄存器        R    register                                       *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 7    一般断点      bp   breakpoint      地址    [once](一次性)         *\r\n\
  * 8    一般断点列表  bpl  bp list                                        *\r\n\
  * 9    删除一般断点  bpc  clear bp        序号                           *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 10   硬件断点      bh 　hard bp         地址 execute/access/write 长度 *\r\n\
  * 11   硬件断点列表  bhl  hard bp list                                   *\r\n\
  * 12   删除硬件断点  bhc  clear hard bp   序号                           *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 13   内存断点      bm   memory bp       起始地址 access/write 长度     *\r\n\
  * 14   内存断点列表  bml  memory bp list                                 *\r\n\
  * 15   删除内存断点  bmc  clear memory bp 序号                           *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 16   导入脚本      ls   load script                                    *\r\n\
  * 17   导出脚本      es   export script                                  *\r\n\
  * 18   单步记录      sr   step record                                    *\r\n\
  * 19   帮助          h    help                                           *\r\n");
  printf("   **************************************************************************\r\n");
	return FALSE;
}