#include "StdAfx.h"
#include "NTFSDrive.h"
#include "MFTRecord.h"


CNTFSDrive::CNTFSDrive(void)
{
	m_pullArrayMftRec = NULL;

	m_dwBytesPerCluster = 0;
	m_dwBytesPerMFTRec = 0;
	m_n64NtfsStart = 0;
}


CNTFSDrive::~CNTFSDrive(void)
{
	if(m_pullArrayMftRec)
		delete []m_pullArrayMftRec;

	m_pullArrayMftRec = NULL;
}



BOOL CNTFSDrive::Initial(DRIVEPACKET *pDR)
{
	BOOT_BLOCK			stBB = {0};
	ULONGLONG			n64MftSize = 0;

	if(!pDR)
		return 0;

	if(!m_Sct.Initial())
		return 0;

	//��������ƫ��		�������			֮ǰ��������	ÿ�����ֽ���
	m_n64NtfsStart = (pDR->ullRS + pDR->ullPS) * m_Sct.m_dwBPS;				//

	//��������Ϣ
	if(!m_Sct._Read(m_n64NtfsStart,(LPWSTR)&stBB, sizeof(stBB)))
		return 0;

	if(memcmp(stBB.Format, L"NTFS", 4)==0)					//???ΪʲôҪreturn
		return 0;
	
	//���� ÿ���ж��� �ֽ�
	m_dwBytesPerCluster = stBB.SectorsPerCluster * m_Sct.m_dwBPS;//ÿ���ֽ���		ÿ���ж��ٸ�����*ÿ�������ֽ���
	m_dwBytesPerMFTRec = 0x01<<((-1)*((char)stBB.ClustersPerFileRecord));//ÿ��¼�ֽ���


	//���ļ��� MFT �����������ÿһ���ļ�������			
	if(!(n64MftSize = GetMftLength(stBB.VolumeSerialNumber & 0xffffffff)))//��ȡMFT����
		return 0;

	if(!InitMftRecArray(n64MftSize, stBB.MftStartLcn))
		return 0;


	return TRUE;
}


BOOL CNTFSDrive::InitMftRecArray(ULONGLONG n64MftSize, ULONGLONG n64MftStartLcn)
{
	PRUNITEM_NODE	pNode = 0;
	ULONGLONG		nMftStart = 0, n64LCN = 0;
	int				nLen = 0, i = 0;
	WCHAR			szBuf[1024] = {0};


	CMFTRecord		mftRec;

	ULONGLONG n64MftRecOffset= m_n64NtfsStart + (n64MftStartLcn * m_dwBytesPerCluster);

	if(!m_Sct._Read(n64MftRecOffset, (LPWSTR)szBuf, 1024))
		return 0;

	if(!mftRec.ExtractFile(szBuf, 1024, FALSE))
		return 0;

	//�ܼ�¼��
	m_u64MFTRecNum = n64MftSize / m_dwBytesPerMFTRec;
	m_pullArrayMftRec = new ULONGLONG[(DWORD)m_u64MFTRecNum];//�����ڴ�

	pNode = mftRec.m_stDataAttr.pRunList;
	while(pNode->pNext)
	{
		n64LCN += pNode->n64Offset;
		nLen = (DWORD)((pNode->n64Len * m_dwBytesPerCluster) / m_dwBytesPerMFTRec);
		nMftStart = m_n64NtfsStart + n64LCN * m_dwBytesPerCluster;
		for(int j=0; j<nLen; j++)
		{
			m_pullArrayMftRec[i++] = nMftStart;

			nMftStart += m_dwBytesPerMFTRec;
		}

		pNode = pNode->pNext;
	}


	return TRUE;
}

BOOL CNTFSDrive::IsHavingFile(DWORD nFileSeq)
{
		return (nFileSeq<m_u64MFTRecNum) ? TRUE : FALSE;
	return TRUE;
}

BOOL CNTFSDrive::GetFileInfoBySeq(DWORD nFileSeq, NTFS_FILEINFO &stFileInfo)
{
	static FILETIME		stFileTime = {0}, stLocalTime = {0};
	static SYSTEMTIME	stSysTime = {0};
	static CHAR		szBuf[1024] = {0};
	static ULONGLONG	n64MftRecStart = 0;

	CMFTRecord			mftRec;

	memset(&stFileInfo, 0, sizeof(stFileInfo));

	n64MftRecStart = m_pullArrayMftRec[nFileSeq];
	if(!n64MftRecStart)
		return 0;

	if(!m_Sct._Read(n64MftRecStart, (LPWSTR)szBuf, 1024))
		return 0;

	if(!mftRec.ExtractFile((LPWSTR)szBuf, 1024, 1))
		return 0;

	//�ļ����ǿ�
	if(!mftRec.m_stFileNameAttr.wFilename[0])
		return 0;

	//�ļ����ַ���
	//wcstombs(stFileInfo.szFileName, mftRec.m_stFileNameAttr.wFilename, MAX_PATH);
	wcscpy_s(stFileInfo.szFileName,mftRec.m_stFileNameAttr.wFilename);

	//ʱ�������ַ���
	memcpy(&stFileTime, &mftRec.m_stFileNameAttr.n64Create, sizeof(ULONGLONG));
	FileTimeToLocalFileTime(&stFileTime, &stLocalTime);
	FileTimeToSystemTime(&stLocalTime, &stSysTime);
	wsprintf(stFileInfo.szCreate, L"%d/%d/%d %02d:%02d:%02d",
		stSysTime.wYear, stSysTime.wMonth, stSysTime.wDay, stSysTime.wHour, stSysTime.wMinute, stSysTime.wSecond);

	//��С�ַ���
	ULONGLONG n64FileSize = (mftRec.m_cchData == -1)?mftRec.m_stDataAttr.n64RealSize:mftRec.m_cchData;
	if(n64FileSize > 1024 * 1024 * 1024)
	{
		wsprintf(stFileInfo.szFileSize, L"%0.1f GB", (float)(signed __int64)(n64FileSize / 1024 / 1024 ) / 1024);
	}
	else if(n64FileSize > 1024 * 1024)
	{
		wsprintf(stFileInfo.szFileSize, L"%0.1f MB", (float)(signed __int64)(n64FileSize / 1024) / 1024);
	}
	else if(n64FileSize > 1024)
	{
		wsprintf(stFileInfo.szFileSize, L"%0.1f KB", (float)(signed __int64)n64FileSize / 1024);
	}
	else
	{
		wsprintf(stFileInfo.szFileSize, L"%d B", (int)n64FileSize);
	}


	return TRUE;
}

BOOL CNTFSDrive::GetData(DWORD nFileSeq, LPWSTR &lpszData, DWORD &cchData)
{
	ULONGLONG			n64MftRecStart = {0};
	TCHAR				szBuf[1024] = {0};

	CMFTRecord			mftRec;

	n64MftRecStart = m_pullArrayMftRec[nFileSeq];

	if(!m_Sct._Read(n64MftRecStart, szBuf, 1024))
		return 0;

	if(!mftRec.ExtractFile(szBuf, 1024, 0))
		return 0;

	if(mftRec.m_cchData == -1)
	{
		PRUNITEM_NODE		pNode = 0;
		ULONGLONG			n64LCN = 0, n64Offset = 0;
		DWORD				n64Len = 0, nRead = 0;

		cchData = (DWORD)mftRec.m_stDataAttr.n64RealSize;
		lpszData = new TCHAR[(DWORD)mftRec.m_stDataAttr.n64AllocSize];
		pNode = mftRec.m_stDataAttr.pRunList;

		while(pNode->pNext)
		{
			n64LCN += pNode->n64Offset;
			n64Len = (DWORD)(pNode->n64Len * m_dwBytesPerCluster);
			n64Offset = m_n64NtfsStart + n64LCN * m_dwBytesPerCluster;

			m_Sct._Read(n64Offset, lpszData + nRead, n64Len);
			nRead += n64Len;

			pNode = pNode->pNext;
		}
	}
	else
	{
		cchData = mftRec.m_cchData;
		lpszData = new TCHAR[cchData];

		memcpy(lpszData, mftRec.m_szData, cchData);
	}

	return TRUE;
}



//��� ���ļ� ������ĳ���											�����к� 
ULONGLONG CNTFSDrive::GetMftLength(DWORD dwVolumeSerialNumber)
{

	TCHAR		szDrives[MAX_PATH] = {0}, szNtfsDrive[4] = {0};
	HANDLE		hVol = 0;
	DWORD		dwWritten = 0, dwSerialNumber = 0;
	TCHAR		szDevName[20] = {0};

	NTFS_VOLUME_DATA_BUFFER  ntfsVolData = {0};

	//��õ�ǰ �߼����� ��·��   ��ǰ��������е���
	GetLogicalDriveStrings(MAX_PATH, szDrives);

	for(int i=0; szDrives[i]; i+=4)
	{
		memcpy(szNtfsDrive, &szDrives[i], 8);

		if(lstrcmpi(szNtfsDrive, L"A:\\") == 0)
			continue;

		//�̶�������				�������������
		if(DRIVE_FIXED != GetDriveType(szNtfsDrive))
			continue;

		//��ȡһ������̾���ص���Ϣ
		GetVolumeInformation(szNtfsDrive, NULL, NULL, &dwSerialNumber, NULL, NULL, NULL, NULL);

		//��� �õ������к��봫���������к���ͬ
		if(dwSerialNumber == dwVolumeSerialNumber)
		{
			//�豸��
			wsprintf(szDevName, L"\\\\.\\%c:", szNtfsDrive[0]);

			//���豸
			hVol = CreateFile(szDevName, GENERIC_READ | GENERIC_WRITE, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,OPEN_EXISTING, 0, NULL);
			if(hVol ==  INVALID_HANDLE_VALUE)
				return FALSE;


			//��Ring0����Ϣ ���NTFS_VOLUME_DATA
			DeviceIoControl(hVol, FSCTL_GET_NTFS_VOLUME_DATA, 
				NULL,
				0,
				&ntfsVolData,
				sizeof(ntfsVolData),
				&dwWritten, 
				NULL);

			if(dwWritten != sizeof(ntfsVolData))
				return FALSE;

			CloseHandle(hVol);

			//���ݵĳ���
			return ntfsVolData.MftValidDataLength.QuadPart;
		}

	}

	return FALSE;
}