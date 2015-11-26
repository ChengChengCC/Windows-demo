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
/////扇区类
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
		DWORD			m_dwBPS;			//每扇区字节
		DWORD			m_dwSPT;			//每磁道扇区
		DWORD			m_dwTPC;			//每柱面磁道
		DWORD			m_dwSPC;			//每柱面扇区


private:
	HANDLE				m_hDevice;

};

