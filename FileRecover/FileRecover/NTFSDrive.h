#pragma once
#include "drive.h"
class CNTFSDrive :
	public CDrive
{
public:
	CNTFSDrive(void);
	~CNTFSDrive(void);

	BOOL Initial(DRIVEPACKET *pDR);
	BOOL InitMftRecArray(ULONGLONG n64MftSize, ULONGLONG n64MftStartLcn);
	BOOL IsHavingFile(DWORD nFileSeq);
	BOOL GetFileInfoBySeq(DWORD nFileSeq, NTFS_FILEINFO &stFileInfo);
	BOOL GetData(DWORD nFileSeq, LPWSTR &lpszData, DWORD &cchData);

	ULONGLONG CNTFSDrive::GetMftLength(DWORD dwVolumeSerialNumber);


public:
	ULONGLONG		m_n64NtfsStart;
	ULONGLONG		m_u64MFTRecNum;
	ULONGLONG		*m_pullArrayMftRec;
	DWORD					m_dwBytesPerCluster, m_dwBytesPerMFTRec;//每簇字节数, 每记录字节数
};

