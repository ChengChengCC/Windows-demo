#pragma once

#include <Windows.h>
#include <WinIoCtl.h>



typedef union _INTEGER_64 {
	struct {
		DWORD LowPart;
		LONG HighPart;
	};
	struct {
		DWORD LowPart;
		LONG HighPart;
	} u;
	LONGLONG QuadPart;
} INTEGER_64, PINTEGER_64;


/*
//
/////������
//
*/

class CSector
{
public:
	CSector(void);
	~CSector(void);



	BOOL CSector::Open(int Num = 0);
	BOOL CSector::Initial();
	BOOL CSector::Read(LONGLONG nSectorOfStart, LPWSTR lpszSector, int nNumberOfRead = 1);
	BOOL CSector::Write(LONGLONG nSectorOfStart, LPWSTR lpszBuffer, int nNumberOfWrite = 1);

	BOOL CSector::_Read(LONGLONG nByteOfStart, WCHAR* lpszBuf, int cchBuf);
	BOOL CSector::_Write(LONGLONG nByteOfStart, LPWSTR lpszBuf, int cchBuf);


public:
		DWORD			m_dwBPS;			//ÿ�����ֽ�
		DWORD			m_dwSPT;			//ÿ�ŵ�����
		DWORD			m_dwTPC;			//ÿ����ŵ�
		DWORD			m_dwSPC;			//ÿ��������


private:
	HANDLE				m_hDevice;

};

