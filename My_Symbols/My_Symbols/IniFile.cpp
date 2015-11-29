#include "StdAfx.h"
#include "IniFile.h"


#define MAX_LENGTH 256
CIniFile::CIniFile()
{
	WCHAR szAppName[MAX_PATH];
	int  len;

	::GetModuleFileName(NULL, szAppName, sizeof(szAppName));    //�õ������ļ���
	len = wcslen(szAppName);
	for(int i=len; i>0; i--)                                    //������չ���е�'.'
	{
		if(szAppName[i] == L'.')
		{
			szAppName[i+1] = L'\0';                              //ɾ�����'.'�Ժ���ַ� ��  ".exe"
			break;
		}
	}
	wcscat_s(szAppName, L"ini");                                   //����  ".ini"
	IniFileName = szAppName;                                    //��ֵ���ļ��� �鿴 һ����Ա����  IniFileName
}

CIniFile::~CIniFile()
{

}

CString CIniFile::GetString(CString AppName,CString KeyName,CString Default)
{
	TCHAR buf[MAX_LENGTH];
	::GetPrivateProfileString(AppName, KeyName, Default, buf, sizeof(buf), IniFileName);  //���һ�����������ļ�����
	return buf;
}

int CIniFile::GetInt(CString AppName,CString KeyName,int Default)
{
	return ::GetPrivateProfileInt(AppName, KeyName, Default, IniFileName);
}

unsigned long CIniFile::GetDWORD(CString AppName,CString KeyName,unsigned long Default)
{
// 	TCHAR buf[MAX_LENGTH];
// 	CString temp;
// 	temp.Format("%u",Default);
// 	::GetPrivateProfileString(AppName, KeyName, temp, buf, sizeof(buf), IniFileName);
// 	return atol(buf);
	return 0;
}

BOOL CIniFile::SetString(CString AppName,CString KeyName,CString Data)
{
	return ::WritePrivateProfileString(AppName, KeyName, Data, IniFileName);
}

BOOL CIniFile::SetInt(CString AppName,CString KeyName,int Data)
{
	CString temp;
	temp.Format(L"%d", Data);
	return ::WritePrivateProfileString(AppName, KeyName, temp, IniFileName);
}

BOOL CIniFile::SetDouble(CString AppName,CString KeyName,double Data)
{
	CString temp;
	temp.Format(L"%f",Data);
	return ::WritePrivateProfileString(AppName, KeyName, temp, IniFileName);
}

BOOL CIniFile::SetDWORD(CString AppName,CString KeyName,unsigned long Data)
{
	CString temp;
	temp.Format(L"%u",Data);
	return ::WritePrivateProfileString(AppName, KeyName, temp, IniFileName);
}