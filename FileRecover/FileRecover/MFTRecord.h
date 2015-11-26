#pragma once

#include "Common.h"

/*
MFT ��¼��
MFT ��һ��ӳ������д�������ж���������ļ���
�� MFT �У�NTFS �����ϵ�ÿ���ļ�(���� MFT ����)������һӳ���
MFT �еĸ�������������ݣ� ��С��ʱ�估ʱ�������ȫ���Ժ�����λ�á�
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

