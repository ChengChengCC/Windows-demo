#include "StdAfx.h"
#include "FAT32Drive.h"


CFAT32Drive::CFAT32Drive(void)
{
	m_n64FatData = 0;
	m_n64FatStartSector = 0;
	m_bySectorsPerCluster = 0;
	m_dwSectorsPerFat = 0;
	m_wReservedSectors = 0;
}


CFAT32Drive::~CFAT32Drive(void)
{
}


BOOL CFAT32Drive::Initial(DRIVEPACKET *pDR)
{
	DOS_BOOT_RECORD		stDBR = {0};

	if(!pDR)
		return 0;

	if(!m_Sct.Initial())			//��ʼ�� ������Ϣ
		return 0;

	m_n64FatStartSector = pDR->ullRS + pDR->ullPS;			//��������ƫ��  ��������

	if(!m_Sct.Read(m_n64FatStartSector, (LPWSTR)&stDBR))		//�� �������� ��ȡ��Ϣ
		return 0;

	if(wcscmp(stDBR.szFileSystem, L"FAT32") != 0)
		return 0;

	//�������� ��Ϣ ��ֵ
	m_bySectorsPerCluster = stDBR.bySectorsPerCluster;			//ÿ�ص�������
	m_dwSectorsPerFat = stDBR.dwSectorsPerFat;					//ÿFat����������
	m_wReservedSectors = stDBR.wReservedSectors;				//��������
	m_n64FatData = m_n64FatStartSector + m_wReservedSectors + m_dwSectorsPerFat * 2;		//��������ƫ�� + �������� + ÿFat������

	MessageBox(0, L"��������׼����ʼ����������Ҫ�ܳ�ʱ�������Ӧ", L"", 0);

	if(!TraversalFile(2))
		return 0;

	MessageBox(0, L"��ʼ���Ѿ��ɹ����", L"", 0);

	return 1;
}


//�� ����
BOOL CFAT32Drive::TraversalFile(DWORD dwCluster)
{
	ULONGLONG		n64Offset = 0;
	CHAR			*lpszBuf = 0;

	//�����ڴ�					ÿ�ص������� * ÿ�����ֽ���
	lpszBuf = new CHAR[m_bySectorsPerCluster * 512];

	//��һ��ƫ��
	n64Offset = m_n64FatData + (dwCluster - 2) * m_bySectorsPerCluster;

	//��ȡĿ¼���ڴ�
	if(!m_Sct.Read(n64Offset, (LPWSTR)lpszBuf, m_bySectorsPerCluster))
		return 0;

	//                ������Ϣ        ÿ�ص�������*512
	ExtractFile((LPWSTR)lpszBuf, m_bySectorsPerCluster * 512);

	delete []lpszBuf;

	return 1;
}


//����ļ�
BOOL CFAT32Drive::ExtractFile(LPWSTR lpszBuf, DWORD cchBuf)
{
	DWORD								dwPos = 0;
	ULONGLONG					n64Offset = 0;
	PSHORTFILENAME			pShortName = 0;				//С�ļ� Name
	PLONGFILENAME			pLongName = 0;				//���ļ� Name

	while(dwPos<cchBuf)
	{

		if(lpszBuf[dwPos] == 0)//û��ʹ��
		{
			break;
		}
		else if(lpszBuf[dwPos] == (-27))//��ɾ��
		{
			BOOL									bOnlyShortName = TRUE;
			DIRECTORYITEM				stFTI = {0};

			if(((PLONGFILENAME)&lpszBuf[dwPos])->byCharacteristics == 0xf)//���ļ���
			{
				int				i=0;
				WCHAR			wszBuf[10][20] = {0}, wszFileName[200] = {0};

				while(((PLONGFILENAME)&lpszBuf[dwPos])->byCharacteristics == 0xf)//����ѭ��
				{
					pLongName = (PLONGFILENAME)&lpszBuf[dwPos];
					dwPos += sizeof(LONGFILENAME);

					memcpy(&wszBuf[i][0], pLongName->wszFileName_5, 5*2);
					memcpy(&wszBuf[i][5], pLongName->wszFileName_6, 6*2);
					memcpy(&wszBuf[i][11], pLongName->wszFileName_2, 2*2);

					i++;
				}

				while(i--)//��ת
				{
					lstrcatW(wszFileName, wszBuf[i]);
				}

				//wcstombs(stFTI.szFileName, wszFileName, 200);//ת��
				wcscpy(stFTI.szFileName,wszFileName);

				bOnlyShortName = FALSE;//�г��ļ���
			}

			*&lpszBuf[dwPos] = '#';
			pShortName = (PSHORTFILENAME)&lpszBuf[dwPos];
			dwPos += sizeof(SHORTFILENAME);

			if(pShortName->byAttributes & ATTR_DIR)//Ŀ¼
			{
			}
			else//�ļ�
			{
				if(bOnlyShortName)//ֻ�ж��ļ���
				{		
					int			i = 0;

					for(i=7; i>=0; i--)//ȥ���ұ߿ո�
					{
						if(pShortName->szFileName[i] != 32)
							break;
					}

					memcpy(stFTI.szFileName, pShortName->szFileName, i+1);

					if(pShortName->szExtension[0] != 0 || pShortName->szExtension[1] != 0 || pShortName->szExtension[2] != 0)
					{

						lstrcat(stFTI.szFileName, L".");
						wcsncat(stFTI.szFileName, pShortName->szExtension, 3);
					}
				}

				stFTI.wStartClusterHi = pShortName->wStartClusterHi;
				stFTI.wStartClusterLo = pShortName->wStartClusterLo;
				stFTI.wCreateDate = pShortName->wCreateDate;
				stFTI.wCreateTime = pShortName->wCreateTime;
				stFTI.dwFileSize = pShortName->dwFileSize;

				m_delvector.push_back(stFTI);
			}
		}
		else//��ʹ����
		{
			if(lpszBuf[dwPos] != 46)//������'.'
			{
				if((((PLONGFILENAME)&lpszBuf[dwPos])->byCharacteristics != 0xf) && 
					(((PSHORTFILENAME)&lpszBuf[dwPos])->byAttributes & ATTR_DIR))//���ļ���������Ŀ¼
				{
					pShortName = (PSHORTFILENAME)&lpszBuf[dwPos];
					if(memcmp(pShortName->szFileName, ".",  1) != 0 && memcmp(pShortName->szFileName, "..",  2) != 0)
					{	
						DWORD dwStartCluster = ((pShortName->wStartClusterHi << 16) & 0xffff0000) |
							(pShortName->wStartClusterLo & 0xffff);

						TraversalFile(dwStartCluster);//�ݹ�
					}
				}
			}

			dwPos += 32;
		}
	}

	return 1;
}



BOOL CFAT32Drive::GetData(DWORD nFileSeq, LPWSTR &lpszData, DWORD &cchData)
{
	DIRECTORYITEM	stFTI = m_delvector[nFileSeq];		//ͨ������ ȡ��Ŀ¼ֵ
	ULONGLONG		n64Offset = 0;
	DWORD			dwLen = 0, dwClusterSize = 0, dwRead = 0;

	dwClusterSize = m_bySectorsPerCluster * m_Sct.m_dwBPS;
	dwLen = (stFTI.dwFileSize + dwClusterSize - 1) / dwClusterSize;

	cchData = stFTI.dwFileSize;
	lpszData = new TCHAR[dwLen * dwClusterSize];

	n64Offset =  m_n64FatData + (stFTI.dwStartCluster - 2) * m_bySectorsPerCluster;
	for(DWORD i=0; i<dwLen; i++)
	{
		if(!m_Sct.Read(n64Offset, lpszData + dwRead, m_bySectorsPerCluster))
			return 0;

		dwRead += dwClusterSize;
		n64Offset += m_bySectorsPerCluster;
	}

	return 1;
}



BOOL CFAT32Drive::GetFileInfoBySeq(DWORD nFileSeq, NTFS_FILEINFO &stFileInfo)
{
	DIRECTORYITEM stFTI = m_delvector[nFileSeq];

	//�ļ����ַ���
	memcpy(stFileInfo.szFileName, stFTI.szFileName, MAX_PATH);

	//�ļ���С�ַ���
	DWORD	dwFileSize = stFTI.dwFileSize;
	if(dwFileSize > 1024 * 1024 * 1024)
	{
		wsprintf(stFileInfo.szFileSize, L"%0.1f GB", (float)(dwFileSize / 1024 / 1024 ) / 1024);
	}
	else if(dwFileSize > 1024 * 1024)
	{
		wsprintf(stFileInfo.szFileSize, L"%0.1f MB", (float)(dwFileSize / 1024) / 1024);
	}
	else if(dwFileSize > 1024)
	{
		wsprintf(stFileInfo.szFileSize, L"%0.1f KB", (float)dwFileSize / 1024);
	}
	else
	{
		wsprintf(stFileInfo.szFileSize, L"%d B", (int)dwFileSize);
	}
	
	////���������ַ���
	wsprintf(stFileInfo.szCreate, L"%d/%d/%d %02d:%02d:%02d",
		(WORD)((stFTI.wCreateDate>>9) & 0x7F) + 1980, 
		(TCHAR)((stFTI.wCreateDate>>5) & 0xF), 
		(TCHAR)(stFTI.wCreateDate & 0x1F), 
		(TCHAR)((stFTI.wCreateTime>>11) & 0x1F), 
		(TCHAR)((stFTI.wCreateTime>>5) & 0x3F),
		(TCHAR)(stFTI.wCreateTime & 0x1F) * 2);

	return TRUE;
}

BOOL CFAT32Drive::IsHavingFile(DWORD nFileSeq)
{
	return (nFileSeq < m_delvector.size())?TRUE:FALSE;
}