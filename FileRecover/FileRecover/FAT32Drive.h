#pragma once
#include "drive.h"

#include "Common.h"


#include <stdio.h>
#include <vector>
using namespace std;

class CFAT32Drive :
	public CDrive
{
public:
	CFAT32Drive(void);
	~CFAT32Drive(void);


	BOOL CFAT32Drive::Initial(DRIVEPACKET *pDR);
	BOOL CFAT32Drive::TraversalFile(DWORD dwCluster);
	BOOL CFAT32Drive::ExtractFile(LPWSTR lpszBuf, DWORD cchBuf);
	BOOL CFAT32Drive::GetData(DWORD nFileSeq, LPWSTR &lpszData, DWORD &cchData);

	BOOL CFAT32Drive::IsHavingFile(DWORD nFileSeq);
	BOOL CFAT32Drive::GetFileInfoBySeq(DWORD nFileSeq, NTFS_FILEINFO &stFileInfo);


private:

	ULONGLONG		m_n64FatData;
	ULONGLONG		m_n64FatStartSector;
	BYTE						m_bySectorsPerCluster;
	DWORD					m_dwSectorsPerFat;
	WORD					m_wReservedSectors;

	vector<DIRECTORYITEM>	m_delvector;

};

