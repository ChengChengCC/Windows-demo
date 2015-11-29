#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <TlHelp32.h>
#include <process.h>
#include "My_SymbolsDlg.h"

using namespace  std;

#define SystemModuleInformation       11 //���ܺ�  11  ��������ģ��

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY {
	HANDLE Section;			//
	PVOID MappedBase;		//ӳ���ַ
	PVOID Base;				//��ڻ�ַ��
	ULONG Size;				//ģ���С
	ULONG Flags;			//ģ�����ͣ�
	USHORT LoadOrderIndex;	//��������˳��
	USHORT InitOrderIndex;	//��ʼ������˳��
	USHORT LoadCount;		//��������
	USHORT PathLength;		//·������
	CHAR   ImageName[256];	//��������
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;



typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG Count; //�Ѿ����ص�  ģ������
	SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;



typedef struct _MODULE_INFO{
	int          Index;     //ģ��ļ������
	char         ImageName[128];     //ģ�������
	char         FullPath[MAX_PATH];  //ģ���·��
	char		 SymbolsPath[MAX_PATH]; //���ű�·��
	char         OrigName[128]; //����ԭ�ļ���
	PVOID        BaseAddr;         //ģ��Ļ�ַ
	int          Size;         //ģ��Ĵ�С
	bool		 bSymbols;    //�Ƿ���ڷ���
}MODULE_INFO, *PMODULE_INFO;





typedef NTSTATUS(__stdcall * PFNNTQUERYSYSTEMINFORMATION)(
	IN DWORD SystemInformationClass ,
	IN OUT  PVOID  SystemInformation, 
	IN  ULONG   SystemInformationLength ,
	OUT PULONG  ReturnLength  OPTIONAL
	);


class CModuleList
{
public:
	
	CModuleList(CWnd* dlg = NULL);
	//дCMy_SymbolsDlg * m_dlg���뱨��
	CWnd* m_dlg;
	BOOL GetKernelModuleList();
	BOOL GetApplicatonModuleList();
	BOOL GetAllModuleList();
	static  unsigned __stdcall thread_GetAllModuleList(LPVOID lParam);

	
private:
	ULONG_PTR g_module_count;
	
};

