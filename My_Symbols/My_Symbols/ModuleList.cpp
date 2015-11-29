#include "stdafx.h"

#include "ModuleList.h"


vector<MODULE_INFO>  g_all_module_info;



CModuleList::CModuleList(CWnd *dlg)
{
	m_dlg = (CMy_SymbolsDlg*)dlg;

}


BOOL CModuleList::GetAllModuleList()
{
	unsigned tid_thread;
	g_all_module_info.clear();
	HANDLE hThread = (HANDLE)_beginthreadex(NULL,0,thread_GetAllModuleList,this,0,&tid_thread);
	if (INVALID_HANDLE_VALUE == hThread)
	{
		return FALSE;
	}
	return TRUE;
}



unsigned  CModuleList::thread_GetAllModuleList(LPVOID lParam)
{
	CModuleList* module = (CModuleList*)lParam;
	if (FALSE == module->GetKernelModuleList() 
		||FALSE == module->GetApplicatonModuleList())
	{
		::MessageBox(NULL,L"EnumModuleError",L"ERROR",0);
		return -1;
	}
	//::SendMessage(module->m_dlg.m_hWnd,MY_GETALLMODULE,NULL,NULL);
	module->m_dlg->PostMessage(MY_GETALLMODULE,NULL,NULL);
	return 0;
}




BOOL CModuleList::GetKernelModuleList()
{

	HMODULE h_ntdll = LoadLibrary(L"Ntdll.dll");
	WCHAR * pBuf = NULL;
	ULONG Needlen = 0;
	char  Text[1024] = {0};
	char * ptr_text_pos = NULL;
	if(INVALID_HANDLE_VALUE == h_ntdll)
	{
		return FALSE;
	}
	g_all_module_info.clear();
	PFNNTQUERYSYSTEMINFORMATION pfnNtQuerySystemInformation = (PFNNTQUERYSYSTEMINFORMATION)GetProcAddress(h_ntdll,"NtQuerySystemInformation");
	if (NULL == pfnNtQuerySystemInformation)
	{
		return FALSE;
	}
	__try{
		pfnNtQuerySystemInformation(SystemModuleInformation, pBuf, NULL, &Needlen);
		if (0 >= Needlen)
		{ 
			return FALSE;
		}
		pBuf = new WCHAR[Needlen];
		ZeroMemory(pBuf, Needlen * 2);
		pfnNtQuerySystemInformation(SystemModuleInformation, pBuf, Needlen * 2, NULL);
	}
	__except (EXCEPTION_EXECUTE_HANDLER){ return FALSE; }

	PSYSTEM_MODULE_INFORMATION  ptr_sys_module_info = (PSYSTEM_MODULE_INFORMATION)pBuf;
	g_module_count = ptr_sys_module_info->Count;
	for (int i=0;i<g_module_count;i++)
	{
		MODULE_INFO module_info = {0};
		SYSTEM_MODULE_INFORMATION_ENTRY entry = ptr_sys_module_info->Module[i];
		module_info.Index = entry.LoadOrderIndex;
		module_info.BaseAddr = entry.Base;
		module_info.Size = entry.Size;
		CopyMemory(module_info.OrigName,entry.ImageName,entry.PathLength);
		wsprintfA(Text, "%s", entry.ImageName);
		ptr_text_pos = strrchr(Text, '\\');
		if (NULL != ptr_text_pos){
			ptr_text_pos++;
			//更新名称
			strcpy_s(module_info.ImageName, ptr_text_pos);
		}
		else{
			//失败的话 就写未知
			strcpy_s(module_info.ImageName,"未知" );
		}
		ptr_text_pos = NULL;


		//以 \\??\\ 开始 表示 非系统目录下 没有 环境变量
		ptr_text_pos = strstr(Text, "\\??\\");
		if (NULL != ptr_text_pos){
			ptr_text_pos = ptr_text_pos + strlen("\\??\\");
			//更新名称
			strcpy_s(module_info.FullPath,ptr_text_pos);
			goto GoToSetPathRet;
		}

		char TextTwo[1024];//保存环境变量 名称
		char EnvironmentPath[1024];//保存环境变量 路径

		//得到环境变量
		ptr_text_pos = Text;//\SystemRoot\system32\ntkrnlpa.exe
		ptr_text_pos++;//SystemRoot\system32\ntkrnlpa.exe
		char * ToPos = strchr(ptr_text_pos, '\\');//找到位置 \system32\ntkrnlpa.exe
		if (NULL >= ToPos){//找不到 则直接写入默认的然后退出
			//更新路径				
			//strcpy_s(module_info.FullPath,entry.ImageName);
			CopyMemory(module_info.FullPath,entry.ImageName,sizeof(entry.ImageName));
			goto GoToSetPathRet;
		}

		ZeroMemory(TextTwo, sizeof(char) * 1024);
		ZeroMemory(EnvironmentPath, sizeof(char) * 1024);
		//拷贝环境变量名称
		CopyMemory(TextTwo, ptr_text_pos, (int)((int)ToPos - (int)ptr_text_pos));
		//TextTwo = "\SystemRoot"
		ToPos++;//指向环境变量 以外字符
		//得到环境变量路径
		if (NULL >= GetEnvironmentVariableA(TextTwo, EnvironmentPath, sizeof(char) * 500)){
			//失败则 忽略 直接使用 System32  SysWOW64 目录
			ZeroMemory(EnvironmentPath, sizeof(char) * 1024);
			ToPos = strstr(Text, "System32");
			if (NULL == ToPos){//如果不包含System32字符  那么就直接使用默认的
				ToPos = strstr(Text, "SysWOW64");
				if (NULL == ToPos){//如果不包含System32字符  那么就直接使用默认的
					//更新路径
					strcpy_s(module_info.FullPath,entry.ImageName);
					goto GoToSetPathRet;
				}
				GetSystemWow64DirectoryA(EnvironmentPath, 1024);
				ToPos = ToPos + strlen("SysWOW64");
				ToPos++;
			}
			else{
				GetSystemDirectoryA(EnvironmentPath, 1024);
				ToPos = ToPos + strlen("System32");
				ToPos++;
			}
		}
		ZeroMemory(TextTwo, sizeof(char) * 500);
		wsprintfA(TextTwo, "%s\\%s", EnvironmentPath, ToPos);
		//更新路径
	
		//之前FullPath写小了，导致栈溢出
		strcpy_s(module_info.FullPath,TextTwo);
GoToSetPathRet:;
		g_all_module_info.push_back(module_info);
		continue;
	}

	return TRUE;
}




BOOL CModuleList::GetApplicatonModuleList()
{
	HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (INVALID_HANDLE_VALUE == hSnap)
	{
		return FALSE;
	}
	MODULE_INFO module_info = {0};//先初始化一个数据指针 用于返回 和循环使用
	int module_count = 0;//用于 返回 得到的模块总数
	MODULEENTRY32 module_entry;
	BOOL _Ok = FALSE;
	module_entry.dwSize = sizeof(MODULEENTRY32);
	if (TRUE == Module32First(hSnap, &module_entry)){
		do{
			ZeroMemory((void*)&module_info,sizeof(MODULE_INFO));
			module_info.Index = module_count;
			size_t converted_number;
			wcstombs_s(&converted_number,module_info.FullPath,module_entry.szExePath,MAX_PATH);
			wcstombs_s(&converted_number,module_info.ImageName,module_entry.szModule,32);
			wcstombs_s(&converted_number,module_info.OrigName,module_entry.szModule,32);
			module_info.BaseAddr = module_entry.modBaseAddr;
			module_info.Size = module_entry.modBaseSize;
			g_all_module_info.push_back(module_info);
			module_count++;
		} while (Module32Next(hSnap, &module_entry));
	}
	CloseHandle(hSnap);
	return TRUE;
}
