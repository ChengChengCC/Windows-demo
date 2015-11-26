#pragma once

#include "Common.h"

/*
MFT 记录类
MFT 是一个映射磁盘中储存的所有对象的索引文件。
在 MFT 中，NTFS 磁盘上的每个文件(包括 MFT 自身)至少有一映射项。
MFT 中的各项包含如下数据： 大小、时间及时间戳、安全属性和数据位置。
*/
class CMFTRecord
{
public:
	CMFTRecord(void);
	~CMFTRecord(void);

	PFILE_RECORD_HEADER CMFTRecord::GetFullMftRec(LPWSTR lpszMftRec, DWORD cchMftRec);
	BOOL CMFTRecord::ExtractFile(LPWSTR lpszMftRec, DWORD cchMftRec, BOOL bOnlyDelete);
	BOOL CMFTRecord::ExtractData(PNTFS_ATTRIBUTE pNtfsAttr, LPSTR &lpszData, DWORD &cchData);

private:
	CHAR*								m_lpszMftRec;
	DWORD								m_dwCurPos;
public:
	ATTR_STANDARD			m_stStdAttr;
	ATTR_FILENAME				m_stFileNameAttr;
	ATTR_DATA						m_stDataAttr;
	TCHAR								m_szData[1024];
	DWORD								m_cchData;

};

