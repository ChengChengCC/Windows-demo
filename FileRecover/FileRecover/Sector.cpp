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
打开设备			我们要从这个设备句柄中读取设备的信息  ReadFile
Num: 第几个硬盘
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
 初始化各成员
 */
BOOL CSector::Initial()
{
	DISK_GEOMETRY	stGeometry = {0};					//磁盘基本信息
	DWORD						dwBytesReturned = 0;
	DWORD						dwLen = 0;

	if(!m_hDevice)
	{
		if(!Open())			//打开磁盘设备
			return FALSE;
	}

	//传出值
	dwLen = sizeof(DISK_GEOMETRY);

	//发内存到 Ring0 获取数据		读取扇区 、磁道、柱面 等最基本的信息
	DeviceIoControl(m_hDevice,IOCTL_DISK_GET_DRIVE_GEOMETRY,
		NULL,
		0,
		&stGeometry,
		dwLen,
		&dwBytesReturned,
		NULL);


	if(dwBytesReturned != dwLen)//错误
	{
		CloseHandle(m_hDevice);
		m_hDevice = NULL;
		return FALSE;
	}


	m_dwBPS = stGeometry.BytesPerSector;					//每扇区字节
	m_dwSPT = stGeometry.SectorsPerTrack;					//每磁道扇区数
	m_dwTPC = stGeometry.TracksPerCylinder ;				//每柱面磁道数

	m_dwSPC = m_dwSPT * m_dwTPC;							//每柱面扇区 = 每柱面磁道数*每磁道扇区数
	
	return TRUE;
}


/*
读取扇区
nSectorOfStart: 要开始读取的扇区号
lpszSector: 读取扇区数据缓冲区
nNumberOfRead: 要读取的扇区数量
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

	dwTotalRead = m_dwBPS * nNumberOfRead;//总共要读取的字节			每扇区字节*扇区数

	nOffset.QuadPart= m_dwBPS * nSectorOfStart;//计算偏移字节				每扇区字节*开始扇区号

	//设置文件偏移指针
	SetFilePointer(m_hDevice, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);

	//向Ring 0 发消息  锁定硬盘
	DeviceIoControl(m_hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytesReturned,
		NULL); 

	//分配内存
	wzBuff = (WCHAR*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, dwTotalRead);
	if(wzBuff==NULL)
	{
		MessageBox(0, L"HeapAlloc() Error", L"", 0);
		return FALSE;
	}

	//ReadFile 读取设备
	dwRet = ReadFile(m_hDevice,wzBuff,dwTotalRead,&dwBytesOfRead,NULL);

	if(dwRet == FALSE || dwBytesOfRead != dwTotalRead)
	{
		MessageBox(0, L"ReadFile() Error", L"", 0);

		HeapFree(GetProcessHeap(),HEAP_ZERO_MEMORY, wzBuff);//释放
		
		//解锁硬盘
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
		//复制到缓冲区  
		memcpy(lpszSector, wzBuff, dwBytesOfRead);

		HeapFree(GetProcessHeap(),HEAP_ZERO_MEMORY, wzBuff);//释放
		
		//解锁硬盘
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
写入磁盘
nSectorOfStart: 要开始写入的扇区号
lpszBuffer: 要写入到扇区的数据缓冲区
nNumberOfWrite: 要写入的扇区数
*/
BOOL CSector::Write(LONGLONG nSectorOfStart, LPWSTR lpszBuffer, int nNumberOfWrite)
{
	DWORD						dwBytesReturned = 0;
	DWORD						dwTotalWrite = 0;
	DWORD						dwBytesOfWrite  = 0;
	INTEGER_64				nOffset = {0};

	if(!m_hDevice || !lpszBuffer || !nNumberOfWrite)
		return FALSE;

	dwTotalWrite = m_dwBPS * nNumberOfWrite;			//总共要写入的字节			每个扇区字节数*扇区数

	nOffset.QuadPart= m_dwBPS * nSectorOfStart;			//计算偏移字节						每个扇区字节数*开始写入的扇区号

	SetFilePointer(m_hDevice, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);						//设置文件偏移指针

	//锁定硬盘
	DeviceIoControl(m_hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytesReturned
		,NULL); 

	//计算完偏移之后 并设置好文件偏移指针 即可向文件写了
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
		//解锁硬盘
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


//这里是读每个分区的扇区
BOOL CSector::_Read(LONGLONG nByteOfStart, LPWSTR lpszBuf, int cchBuf)
{
	DWORD				dwBytesReturned = 0, dwRet = 0, dwBytesOfRead = 0;
	INTEGER_64		nOffset = {0};
	//char szBuffer[0x1000] = {0};
	char *szBuffer = NULL;
	if(!m_hDevice || !lpszBuf || !cchBuf)
		return FALSE;

	
	nOffset.QuadPart = nByteOfStart;
	SetFilePointer(m_hDevice, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);//设置文件偏移指针

	 //锁定硬盘
	DWORD ret = DeviceIoControl(m_hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytesReturned,
		NULL);

	//读取

	
	dwRet = ReadFile(m_hDevice, lpszBuf,cchBuf , &dwBytesOfRead, NULL);

	int i = GetLastError();

	if(dwRet == FALSE || dwBytesOfRead != cchBuf)
	{
		MessageBox(0, L"ReadFile() Error", L"", 0);

		//解锁硬盘
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
			NULL);//解锁硬盘

		return TRUE;
	}
}

BOOL CSector::_Write(LONGLONG nByteOfStart, LPWSTR lpszBuf, int cchBuf)
{
	DWORD			dwBytesReturned = 0, dwTotalWrite = 0, dwBytesOfWrite = 0;
	INTEGER_64		nOffset = {0};

	if(!m_hDevice || !lpszBuf || !cchBuf)
		return FALSE;

	//设置文件偏移指针
	nOffset.QuadPart = nByteOfStart;
	SetFilePointer(m_hDevice, nOffset.LowPart, &nOffset.HighPart, FILE_BEGIN);

	//锁定硬盘
	DeviceIoControl(m_hDevice,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytesReturned,
		NULL); 

	//写到 磁盘中
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
			NULL);//解锁硬盘

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
			NULL);//解锁硬盘


		return TRUE;
	}
}



CSector::~CSector(void)
{
	//关闭设备
	if(m_hDevice)
		CloseHandle(m_hDevice);

	m_hDevice = NULL;
}
