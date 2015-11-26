#include "StdAfx.h"
#include "Sector.h"


CSector::CSector(void)
{
	m_hDevice = NULL;

	m_dwBPS = 0;
	m_dwSPT = 0;
	m_dwTPC = 0;
	m_dwSPC = 0;
}




/*
���豸			����Ҫ������豸����ж�ȡ�豸����Ϣ  ReadFile
Num: �ڼ���Ӳ��
*/
BOOL CSector::Open(int Num)
{
	TCHAR			szDevicename[64] = {0};

	wsprintf(szDevicename, L"\\\\.\\PHYSICALDRIVE%d", Num);

	m_hDevice = CreateFile(szDevicename,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

	if(m_hDevice == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile() Error\n");
		return FALSE;
	}

	return TRUE;
}




/*
 ��ʼ������Ա
 */
BOOL CSector::Initial()
{
	DISK_GEOMETRY	stGeometry = {0};					//���̻�����Ϣ
	DWORD						dwBytesReturned = 0;
	DWORD						dwLen = 0;

	if(!m_hDevice)
	{
		if(!Open())			//�򿪴����豸
			return FALSE;
	}

	//����ֵ
	dwLen = sizeof(DISK_GEOMETRY);

	//���ڴ浽 Ring0 ��ȡ����		��ȡ���� ���ŵ������� �����������Ϣ
	DeviceIoControl(m_hDevice,IOCTL_DISK_GET_DRIVE_GEOMETRY,
		NULL,
		0,
		&stGeometry,
		dwLen,
		&dwBytesReturned,
		NULL);


	if(dwBytesReturned != dwLen)//����
	{
		CloseHandle(m_hDevice);
		m_hDevice = NULL;
		return FALSE;
	}


	m_dwBPS = stGeometry.BytesPerSector;					//ÿ�����ֽ�
	m_dwSPT = stGeometry.SectorsPerTrack;					//ÿ�ŵ�������
	m_dwTPC = stGeometry.TracksPerCylinder ;				//ÿ����ŵ���

	m_dwSPC = m_dwSPT * m_dwTPC;							//ÿ�������� = ÿ����ŵ���*ÿ�ŵ�������
	
	return TRUE;
}


/*
��ȡ����
nSectorOfStart: Ҫ��ʼ��ȡ��������
lpszSector: ��ȡ�������ݻ�����
nNumberOfRead: Ҫ��ȡ����������
*/																												
BOOL CSector::Read(LONGLONG nSectorOfStart, LPWSTR lpszSector, int nNumberOfRead)
{
	WCHAR						*wzBuff = 0;
	DWORD						dwBytesReturned = 0;
	DWORD						dwRet = 0;
	DWORD						dwBytesOfRead = 0;
	DWORD						dwTotalRead = 0;
	INTEGER_64				nOffset = {0};

	if(!m_hDevice || !lpszSector || !nNumberOfRead)
		return FALSE;

	dwTotalRead = m_dwBPS * nNumberOfRead;//�ܹ�Ҫ��ȡ���ֽ�			ÿ�����ֽ�*������

	nOffset.QuadPart= m_dwBPS * nSectorOfStart;//����ƫ���ֽ�				ÿ�����ֽ�*��ʼ������

	//�����ļ�ƫ��ָ��
	SetFilePointer(m_hDevice, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);

	//��Ring 0 ����Ϣ  ����Ӳ��
	DeviceIoControl(m_hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytesReturned,
		NULL); 

	//�����ڴ�
	wzBuff = (WCHAR*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, dwTotalRead);
	if(wzBuff==NULL)
	{
		MessageBox(0, L"HeapAlloc() Error", L"", 0);
		return FALSE;
	}

	//ReadFile ��ȡ�豸
	dwRet = ReadFile(m_hDevice,wzBuff,dwTotalRead,&dwBytesOfRead,NULL);

	if(dwRet == FALSE || dwBytesOfRead != dwTotalRead)
	{
		MessageBox(0, L"ReadFile() Error", L"", 0);

		HeapFree(GetProcessHeap(),HEAP_ZERO_MEMORY, wzBuff);//�ͷ�
		
		//����Ӳ��
		DeviceIoControl(m_hDevice,
			FSCTL_UNLOCK_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&dwBytesReturned,
			NULL);

		return FALSE;
	}
	else
	{
		//���Ƶ�������  
		memcpy(lpszSector, wzBuff, dwBytesOfRead);

		HeapFree(GetProcessHeap(),HEAP_ZERO_MEMORY, wzBuff);//�ͷ�
		
		//����Ӳ��
		DeviceIoControl(m_hDevice,
			FSCTL_UNLOCK_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&dwBytesReturned,
			NULL);

		return TRUE;
	}
}


/*
д�����
nSectorOfStart: Ҫ��ʼд���������
lpszBuffer: Ҫд�뵽���������ݻ�����
nNumberOfWrite: Ҫд���������
*/
BOOL CSector::Write(LONGLONG nSectorOfStart, LPWSTR lpszBuffer, int nNumberOfWrite)
{
	DWORD						dwBytesReturned = 0;
	DWORD						dwTotalWrite = 0;
	DWORD						dwBytesOfWrite  = 0;
	INTEGER_64				nOffset = {0};

	if(!m_hDevice || !lpszBuffer || !nNumberOfWrite)
		return FALSE;

	dwTotalWrite = m_dwBPS * nNumberOfWrite;			//�ܹ�Ҫд����ֽ�			ÿ�������ֽ���*������

	nOffset.QuadPart= m_dwBPS * nSectorOfStart;			//����ƫ���ֽ�						ÿ�������ֽ���*��ʼд���������

	SetFilePointer(m_hDevice, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);						//�����ļ�ƫ��ָ��

	//����Ӳ��
	DeviceIoControl(m_hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytesReturned
		,NULL); 

	//������ƫ��֮�� �����ú��ļ�ƫ��ָ�� �������ļ�д��
	if(WriteFile(m_hDevice,lpszBuffer,dwTotalWrite,&dwBytesOfWrite,NULL) == FALSE)
	{
		MessageBox(0, L"WriteFile() Error", L"", 0);

		DeviceIoControl(m_hDevice,
			FSCTL_UNLOCK_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&dwBytesOfWrite,
			NULL);

		return FALSE;
	}
	else
	{
		//����Ӳ��
		DeviceIoControl(m_hDevice,
			FSCTL_UNLOCK_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&dwBytesReturned,
			NULL);


		return TRUE;
	}
}


//�����Ƕ�ÿ������������
BOOL CSector::_Read(LONGLONG nByteOfStart, LPWSTR lpszBuf, int cchBuf)
{
	DWORD				dwBytesReturned = 0, dwRet = 0, dwBytesOfRead = 0;
	INTEGER_64		nOffset = {0};
	//char szBuffer[0x1000] = {0};
	char *szBuffer = NULL;
	if(!m_hDevice || !lpszBuf || !cchBuf)
		return FALSE;

	
	nOffset.QuadPart = nByteOfStart;
	SetFilePointer(m_hDevice, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);//�����ļ�ƫ��ָ��

	 //����Ӳ��
	DWORD ret = DeviceIoControl(m_hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytesReturned,
		NULL);

	//��ȡ

	
	dwRet = ReadFile(m_hDevice, lpszBuf,cchBuf , &dwBytesOfRead, NULL);

	int i = GetLastError();

	if(dwRet == FALSE || dwBytesOfRead != cchBuf)
	{
		MessageBox(0, L"ReadFile() Error", L"", 0);

		//����Ӳ��
		DeviceIoControl(m_hDevice,
			FSCTL_UNLOCK_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&dwBytesReturned,
			NULL);

		return FALSE;
	}
	else
	{
		DeviceIoControl(m_hDevice,
			FSCTL_UNLOCK_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&dwBytesReturned,
			NULL);//����Ӳ��

		return TRUE;
	}
}

BOOL CSector::_Write(LONGLONG nByteOfStart, LPWSTR lpszBuf, int cchBuf)
{
	DWORD			dwBytesReturned = 0, dwTotalWrite = 0, dwBytesOfWrite = 0;
	INTEGER_64		nOffset = {0};

	if(!m_hDevice || !lpszBuf || !cchBuf)
		return FALSE;

	//�����ļ�ƫ��ָ��
	nOffset.QuadPart = nByteOfStart;
	SetFilePointer(m_hDevice, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);

	//����Ӳ��
	DeviceIoControl(m_hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytesReturned,
		NULL); 

	//д�� ������
	if(WriteFile(m_hDevice,lpszBuf,cchBuf,&dwBytesOfWrite,NULL)==FALSE)
	{
		MessageBox(0, L"WriteFile() Error", L"", 0);

		DeviceIoControl(m_hDevice,
			FSCTL_UNLOCK_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&dwBytesOfWrite,
			NULL);//����Ӳ��

		return FALSE;
	}
	else
	{
		DeviceIoControl(m_hDevice,
			FSCTL_UNLOCK_VOLUME,
			NULL,
			0,
			NULL,
			0,
			&dwBytesReturned,
			NULL);//����Ӳ��


		return TRUE;
	}
}



CSector::~CSector(void)
{
	//�ر��豸
	if(m_hDevice)
		CloseHandle(m_hDevice);

	m_hDevice = NULL;
}
