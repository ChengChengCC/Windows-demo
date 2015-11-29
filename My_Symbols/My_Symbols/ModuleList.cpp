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
			//��������
			strcpy_s(module_info.ImageName, ptr_text_pos);
		}
		else{
			//ʧ�ܵĻ� ��дδ֪
			strcpy_s(module_info.ImageName,"δ֪" );
		}
		ptr_text_pos = NULL;


		//�� \\??\\ ��ʼ ��ʾ ��ϵͳĿ¼�� û�� ��������
		ptr_text_pos = strstr(Text, "\\??\\");
		if (NULL != ptr_text_pos){
			ptr_text_pos = ptr_text_pos + strlen("\\??\\");
			//��������
			strcpy_s(module_info.FullPath,ptr_text_pos);
			goto GoToSetPathRet;
		}

		char TextTwo[1024];//���滷������ ����
		char EnvironmentPath[1024];//���滷������ ·��

		//�õ���������
		ptr_text_pos = Text;//\SystemRoot\system32\ntkrnlpa.exe
		ptr_text_pos++;//SystemRoot\system32\ntkrnlpa.exe
		char * ToPos = strchr(ptr_text_pos, '\\');//�ҵ�λ�� \system32\ntkrnlpa.exe
		if (NULL >= ToPos){//�Ҳ��� ��ֱ��д��Ĭ�ϵ�Ȼ���˳�
			//����·��				
			//strcpy_s(module_info.FullPath,entry.ImageName);
			CopyMemory(module_info.FullPath,entry.ImageName,sizeof(entry.ImageName));
			goto GoToSetPathRet;
		}

		ZeroMemory(TextTwo, sizeof(char) * 1024);
		ZeroMemory(EnvironmentPath, sizeof(char) * 1024);
		//����������������
		CopyMemory(TextTwo, ptr_text_pos, (int)((int)ToPos - (int)ptr_text_pos));
		//TextTwo = "\SystemRoot"
		ToPos++;//ָ�򻷾����� �����ַ�
		//�õ���������·��
		if (NULL >= GetEnvironmentVariableA(TextTwo, EnvironmentPath, sizeof(char) * 500)){
			//ʧ���� ���� ֱ��ʹ�� System32  SysWOW64 Ŀ¼
			ZeroMemory(EnvironmentPath, sizeof(char) * 1024);
			ToPos = strstr(Text, "System32");
			if (NULL == ToPos){//���������System32�ַ�  ��ô��ֱ��ʹ��Ĭ�ϵ�
				ToPos = strstr(Text, "SysWOW64");
				if (NULL == ToPos){//���������System32�ַ�  ��ô��ֱ��ʹ��Ĭ�ϵ�
					//����·��
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
		//����·��
	
		//֮ǰFullPathдС�ˣ�����ջ���
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
	MODULE_INFO module_info = {0};//�ȳ�ʼ��һ������ָ�� ���ڷ��� ��ѭ��ʹ��
	int module_count = 0;//���� ���� �õ���ģ������
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
