#pragma once

#include "Common.h"
#include "Sector.h"

class CDrive
{
public:
	CDrive(void);
	~CDrive(void);

public:

virtual BOOL Initial(DRIVEPACKET *pDR) = 0;
virtual BOOL IsHavingFile(DWORD nFileSeq) = 0;
virtual BOOL GetFileInfoBySeq(DWORD nFileSeq, NTFS_FILEINFO &stFileInfo) = 0;
virtual BOOL GetData(DWORD nFileSeq, LPWSTR &lpszData, DWORD &cchData) = 0;

protected:
CSector			m_Sct;

};

CDrive *FactoryDrive(int nType);
