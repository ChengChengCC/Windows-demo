#include "StdAfx.h"
#include "MFTRecord.h"


CMFTRecord::CMFTRecord(void)
{

	m_lpszMftRec = NULL;
	m_dwCurPos = NULL;
	m_cchData = NULL;

	RtlZeroMemory(&m_stStdAttr, sizeof(m_stStdAttr));
	RtlZeroMemory(&m_stFileNameAttr, sizeof(m_stFileNameAttr));
	RtlZeroMemory(&m_stDataAttr, sizeof(m_stDataAttr));
	RtlZeroMemory(m_szData, sizeof(m_szData));

}


CMFTRecord::~CMFTRecord(void)
{
	if(m_lpszMftRec)
		delete []m_lpszMftRec;
}



PFILE_RECORD_HEADER CMFTRecord::GetFullMftRec(LPWSTR lpszMftRec, DWORD cchMftRec)
{
	PFILE_RECORD_HEADER		pFileRecHead = 0;
	DWORD										dwCurPos = 0;
	WORD										wUpdateSeguence = 0;

	//复制
	m_lpszMftRec = new CHAR[cchMftRec];
	RtlCopyMemory(m_lpszMftRec, lpszMftRec, cchMftRec);

	pFileRecHead = (PFILE_RECORD_HEADER)&m_lpszMftRec[m_dwCurPos];

	
	//是否是文件记录
	if(memcmp(pFileRecHead->szSignature, L"FILE",8)!=1)
		return 0;

	dwCurPos += pFileRecHead->wFixupOffset;

	//更新序列号	
	memcpy(&wUpdateSeguence, &m_lpszMftRec[dwCurPos], sizeof(WORD));
	dwCurPos += 2;

	if(memcmp(&wUpdateSeguence, &m_lpszMftRec[510], sizeof(WORD)))
		return 0;
	memcpy(&m_lpszMftRec[510], &m_lpszMftRec[dwCurPos], sizeof(WORD));
	dwCurPos += 2;

	if(memcmp(&wUpdateSeguence, &m_lpszMftRec[1022], sizeof(WORD)))
		return 0;
	memcpy(&m_lpszMftRec[1022], &m_lpszMftRec[dwCurPos], sizeof(WORD));

	return pFileRecHead;
}

BOOL CMFTRecord::ExtractFile(LPWSTR lpszMftRec, DWORD cchMftRec, BOOL bOnlyDelete)
{
	PFILE_RECORD_HEADER			pFileRecHead = 0;
	PNTFS_ATTRIBUTE						pNtfsAttr = 0;
	BOOL												bRetVal = 0;

	CHAR				*lpszTmpData = 0;
	DWORD				cchTmpData = 0;

	if(!(pFileRecHead = GetFullMftRec(lpszMftRec, cchMftRec)))
		return 0;

	if(pFileRecHead->wFlags && bOnlyDelete)
		return 0;

	m_dwCurPos = pFileRecHead->wAttribOffset;
	do
	{
		pNtfsAttr = (PNTFS_ATTRIBUTE)&m_lpszMftRec[m_dwCurPos];

		switch(pNtfsAttr->dwType)
		{
		case 0x10://STANDARD_INFORMATION
			{
				bRetVal = ExtractData(pNtfsAttr, lpszTmpData, cchTmpData);
				if(!bRetVal || !lpszTmpData)
					return 0;

				memcpy(&m_stStdAttr, lpszTmpData, sizeof(m_stStdAttr));

				delete []lpszTmpData;
				lpszTmpData = 0;
				cchTmpData = 0;
			}
			break;
		case 0x30://FILE_NAME
			{
				bRetVal = ExtractData(pNtfsAttr, lpszTmpData, cchTmpData);
				if(!bRetVal || !lpszTmpData)
					return 0;

				if(((PATTR_FILENAME)lpszTmpData)->chFileNameType != 2)
				{
					memcpy(&m_stFileNameAttr, lpszTmpData, cchTmpData);
				}

				delete []lpszTmpData;
				lpszTmpData = 0;
				cchTmpData = 0;
			}
			break;		
		case 0x40: //OBJECT_ID
			break;
		case 0x50: //SECURITY_DESCRIPTOR
			break;
		case 0x60: //VOLUME_NAME
			break;
		case 0x70: //VOLUME_INFORMATION
			break;
		case 0x80://DATA
			{
				bRetVal = ExtractData(pNtfsAttr, lpszTmpData, cchTmpData);
				if(!bRetVal || !lpszTmpData)
					return 0;

				if(cchTmpData == -1)
				{
					memcpy(&m_stDataAttr, lpszTmpData, sizeof(m_stDataAttr));
					m_cchData = -1;
				}
				else
				{
					memcpy(m_szData, lpszTmpData, cchTmpData);
					m_cchData = cchTmpData;
				}

				delete []lpszTmpData;
				lpszTmpData = 0;
				cchTmpData = 0;
			}
			break;
		case 0x90: //INDEX_ROOT
			break;
		case 0xa0: //INDEX_ALLOCATION
			break;
		case 0xb0: //BITMAP
			break;
		case 0xc0: //REPARSE_POINT
			break;
		case 0xd0: //EA_INFORMATION
			break;
		case 0xe0: //EA
			break;
		case 0xf0: //PROPERTY_SET
			break;
		case 0x100: //LOGGED_UTILITY_STREAM
			break;
		case 0x1000: //FIRST_USER_DEFINED_ATTRIBUTE
			break;
		default:
			break;
		}

		m_dwCurPos += pNtfsAttr->dwFullLength;
	}
	while(pNtfsAttr->dwType != 0xFFFFFFFF && pNtfsAttr->dwType != 0);

	if(m_lpszMftRec)
		delete []m_lpszMftRec;
	m_lpszMftRec = NULL;

	if(lpszTmpData)
		delete []lpszTmpData;
	lpszTmpData = 0;
	cchTmpData = 0;

	return 1;
}


BOOL CMFTRecord::ExtractData(PNTFS_ATTRIBUTE pNtfsAttr, LPSTR &lpszData, DWORD &cchData)
{
	if(!pNtfsAttr)
		return 0;

	DWORD			dwCurPos = m_dwCurPos;
	NTFS_ATTRIBUTE	ntfsAttr = *pNtfsAttr;

	if(!ntfsAttr.uchNonResFlag)
	{
		cchData = ntfsAttr.Attr.Resident.dwLength;

		lpszData = new CHAR[cchData];
		memcpy(lpszData, &m_lpszMftRec[dwCurPos + ntfsAttr.Attr.Resident.wAttrOffset], cchData);

		return 1;
	}
	else
	{
		DATARUN			stDR = {0};
		PRUNITEM_NODE	pNode = 0;

		lpszData = new CHAR[sizeof(stDR)];

		stDR.n64AllocSize = ntfsAttr.Attr.NonResident.n64AllocSize;
		stDR.n64RealSize = ntfsAttr.Attr.NonResident.n64RealSize;
		stDR.pRunList = pNode = new RUNITEM_NODE;

		cchData = -1;
		memcpy(lpszData, &stDR, sizeof(stDR));

		dwCurPos += ntfsAttr.Attr.NonResident.wDatarunOffset;
		for(int i=0; ; i++)
		{
			CHAR chLenAndOff = 0;
			CHAR chLen = 0;
			CHAR chOffset = 0;

			RtlZeroMemory(pNode, sizeof(RUNITEM_NODE));

			memcpy(&chLenAndOff, &m_lpszMftRec[dwCurPos], sizeof(CHAR));
			dwCurPos += sizeof(CHAR);

			if(!chLenAndOff)
				break;

			chLen		= chLenAndOff & 0x0F;
			chOffset	= (chLenAndOff & 0xF0) >> 4;

			memcpy(&pNode->n64Len, &m_lpszMftRec[dwCurPos], chLen);
			dwCurPos += chLen;

			memcpy(&pNode->n64Offset,&m_lpszMftRec[dwCurPos],chOffset);
			dwCurPos += chOffset;

			pNode = pNode->pNext = new RUNITEM_NODE;
		}

	}

	return 1;
}