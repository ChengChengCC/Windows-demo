#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <TlHelp32.h>
#include <process.h>
#include "My_SymbolsDlg.h"

using namespace  std;

#define SystemModuleInformation       11 //功能号  11  遍历驱动模块

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY {
	HANDLE Section;			//
	PVOID MappedBase;		//映射基址
	PVOID Base;				//入口基址？
	ULONG Size;				//模块大小
	ULONG Flags;			//模块类型？
	USHORT LoadOrderIndex;	//加载命令顺序
	USHORT InitOrderIndex;	//初始化命令顺序
	USHORT LoadCount;		//加载数量
	USHORT PathLength;		//路径长度
	CHAR   ImageName[256];	//驱动名称
} SYSTEM_MODULE_INFORMATION_ENTRY, *PSYSTEM_MODULE_INFORMATION_ENTRY;



typedef struct _SYSTEM_MODULE_INFORMATION {
	ULONG Count; //已经加载的  模块总数
	SYSTEM_MODULE_INFORMATION_ENTRY Module[1];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;



typedef struct _MODULE_INFO{
	int          Index;     //模块的加载序号
	char         ImageName[128];     //模块的名称
	char         FullPath[MAX_PATH];  //模块的路径
	char		 SymbolsPath[MAX_PATH]; //符号表路径
	char         OrigName[128]; //保存原文件名
	PVOID        BaseAddr;         //模块的基址
	int          Size;         //模块的大小
	bool		 bSymbols;    //是否存在符号
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
	//写CMy_SymbolsDlg * m_dlg编译报错。
	CWnd* m_dlg;
	BOOL GetKernelModuleList();
	BOOL GetApplicatonModuleList();
	BOOL GetAllModuleList();
	static  unsigned __stdcall thread_GetAllModuleList(LPVOID lParam);

	
private:
	ULONG_PTR g_module_count;
	
};

