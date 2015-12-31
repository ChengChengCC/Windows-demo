#include "stdafx.h"


#include "Common.h"







//16�����ַ���ת��Ϊ��ֵ
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




//��ʾ����
BOOL ShowHelp(StuCommand* pCmd)
{
	printf("   ============================ help menu ===========================---=====\n");
	printf("   **************************************************************************\n");
	printf("\
  * ��� ������      ������ Ӣ��˵��        ����1    ����2    ����3        *\r\n\
  * 1    ��������      T    step into                                      *\r\n\
  * 2    ��������      P    step over                                      *\r\n\
  * 3    ����          G    run             ��ַ����                       *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 4    �����        U    assemble        ��ַ����                       *\r\n\
  * 5    ����          D    data            ��ַ����                       *\r\n\
  * 6    �Ĵ���        R    register                                       *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 7    һ��ϵ�      bp   breakpoint      ��ַ    [once](һ����)         *\r\n\
  * 8    һ��ϵ��б�  bpl  bp list                                        *\r\n\
  * 9    ɾ��һ��ϵ�  bpc  clear bp        ���                           *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 10   Ӳ���ϵ�      bh ��hard bp         ��ַ execute/access/write ���� *\r\n\
  * 11   Ӳ���ϵ��б�  bhl  hard bp list                                   *\r\n\
  * 12   ɾ��Ӳ���ϵ�  bhc  clear hard bp   ���                           *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 13   �ڴ�ϵ�      bm   memory bp       ��ʼ��ַ access/write ����     *\r\n\
  * 14   �ڴ�ϵ��б�  bml  memory bp list                                 *\r\n\
  * 15   ɾ���ڴ�ϵ�  bmc  clear memory bp ���                           *\r\n\
  *------------------------------------------------------------------------*\r\n\
  * 16   ����ű�      ls   load script                                    *\r\n\
  * 17   �����ű�      es   export script                                  *\r\n\
  * 18   ������¼      sr   step record                                    *\r\n\
  * 19   ����          h    help                                           *\r\n");
  printf("   **************************************************************************\r\n");
	return FALSE;
}