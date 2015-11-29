
#include "stdafx.h"
#include "Symbols.h"

//wchar_t SYMBOLSPATH[MAX_PATH] = {0};
extern CString SYMBOLSPATH;
wchar_t g_symbols_full_path[MAX_PATH]={0};
HANDLE hProcess = NULL;

//��ʼ�� ���ſ⺯��
BOOL InitSymHandler(bool OutDebug){


	wchar_t symbols_path[MAX_PATH * 2] = { 0 };
	//�õ������ ���̾�� (���浽ȫ��)
	
	hProcess = GetCurrentProcess(); 
	if (NULL == hProcess || hProcess == INVALID_HANDLE_VALUE)
	{ 
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	}
	if (NULL == hProcess || hProcess == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	

	//��ʼ�� ���ſ�Ŀ¼���� �õ�·��
	ZeroMemory(symbols_path, sizeof(wchar_t) * MAX_PATH * 2);

	if (SYMBOLSPATH.IsEmpty() || FALSE == PathFileExists(SYMBOLSPATH))
	{
		//��ȡĿ¼	 û���򴴽�
		ZeroMemory(g_symbols_full_path, sizeof(wchar_t) * MAX_PATH);
		if (FALSE == SymGetSearchPath(hProcess, (PSTR)g_symbols_full_path, MAX_PATH))
		{
			int n = GetLastError();
			return FALSE;
		}
		if (NULL >= wcslen(g_symbols_full_path))
		{ 
			return FALSE;
		}

		if (FALSE == PathFileExists(g_symbols_full_path))
		{
			if (FALSE == CreateDirectory(g_symbols_full_path, NULL))
			{
				return FALSE;
			}
		}
		wsprintf(symbols_path, L"srv*%s *http://msdl.microsoft.com/download/symbols", g_symbols_full_path);

	}
	else
	{
		wsprintf(g_symbols_full_path, L"srv*%s *http://msdl.microsoft.com/download/symbols", SYMBOLSPATH);
		CopyMemory(symbols_path, SYMBOLSPATH, sizeof(wchar_t) * wcslen(SYMBOLSPATH));
	
	}
	//���÷���ѡ��
	
	if (NULL == SymSetOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | (true == OutDebug ? SYMOPT_DEBUG : NULL))){ return FALSE; }
	
	//------------------------------------��ʼ�����ſ�
	if (FALSE == SymInitialize(hProcess, (PSTR)g_symbols_full_path, FALSE)){
		return FALSE;
	}

	return TRUE;
}




//��ȡ�ļ��Ƿ���� ���ڵĻ�,ͨ����������·��
BOOL GetFileSymbolsA(IN char * module_path, OUT char * symbols_file_name, unsigned int BufSize)
{

	//������ʼ��
	
	wchar_t sym_file_name[MAX_PATH] = { NULL };

	SYMSRV_INDEX_INFO Info = { NULL };
	SYMSRV_INDEX_INFO Info2 = { NULL };
	Info.sizeofstruct = sizeof(SYMSRV_INDEX_INFO);
	size_t converted;
	
	//�õ��ļ�����  �����ж�pdb�ļ��Ƿ����
	if (FALSE == SymSrvGetFileIndexInfo(module_path, &Info, NULL))
	{
		int n = GetLastError();
		return FALSE;
	}
	//ע��ansi �� unicode 
	char tmp_symbols_path[MAX_PATH] = {0};
	wcstombs(tmp_symbols_path,SYMBOLSPATH.GetBuffer(),MAX_PATH);
	//wcscpy_s(tmp_symbols_path,SYMBOLSPATH.GetBuffer());
	if (FALSE == SymFindFileInPath(hProcess, tmp_symbols_path, Info.pdbfile, (PVOID)&Info.guid, Info.age, 0, 8, symbols_file_name, 0, 0))
	{
		int n = GetLastError();
		return FALSE;
	}
	
	return TRUE;
}



BOOL GetFileSymbolsW(wchar_t * module_path, wchar_t * symbols_file_name, unsigned int BufSize){
//	if (MAX_PATH >= BufSize){ return FALSE; }

	//������ʼ��
	char tmp_module_path[MAX_PATH] = { NULL };
	wchar_t sym_file_name[MAX_PATH] = { NULL };

	SYMSRV_INDEX_INFO Info = { NULL };
	Info.sizeofstruct = sizeof(SYMSRV_INDEX_INFO);

	
	//�õ��ļ�����  �����ж�pdb�ļ��Ƿ����
	if (FALSE == SymSrvGetFileIndexInfo((PSTR)module_path, &Info, NULL)){
		return FALSE;
	}
	if (FALSE == SymFindFileInPath(hProcess,(PSTR)SYMBOLSPATH.GetBuffer(), Info.pdbfile, (PVOID)&Info.guid, Info.age, 0, 8, (PSTR)symbols_file_name, 0, 0)){
		return FALSE;
	}
	return TRUE;
}



//���ط����ļ�
LONG_PTR LoadSymModuleA(char * module_path, ULONG_PTR module_addr, DWORD ModuleSize)
{

	//����SymLoadModule���������Ӧ���ſ�-----------------------------------------------
#ifdef _WIN64
	return SymLoadModule64(hProcess, NULL, module_path, NULL, module_addr, ModuleSize);
#endif
	
	return SymLoadModule(hProcess, NULL, module_path, NULL, module_addr, ModuleSize);
	
	
}

LONG_PTR LoadSymModuleW(wchar_t * module_path, ULONG_PTR module_addr, DWORD ModuleSize)
{

	char tmp_path[MAX_PATH] = {0};
	wcstombs(tmp_path,module_path,MAX_PATH);

#ifdef _WIN64
	return SymLoadModule64(hProcess, NULL, tmp_path, NULL, (ULONG_PTR)module_addr, ModuleSize);
#endif
	return SymLoadModule(hProcess, NULL, tmp_path, NULL, (ULONG_PTR)module_addr, ModuleSize);
}



//������ (����Ҫ������õ�һ����������)
//�������Ź���  ��ģ������)
BOOL EnumSymProc(LONG_PTR load_handle, PSYM_ENUMERATESYMBOLS_CALLBACKW proc_callback,PVOID user_context)
{
	if (NULL == proc_callback){ return FALSE; }
	return SymEnumSymbolsW(hProcess, load_handle, NULL, proc_callback, user_context);
}


BOOL UnLoadSymModule(ULONG_PTR LoadHandle)
{

#ifdef _WIN64
		return SymUnloadModule64(hProcess, (ULONG_PTR)LoadHandle);
#endif // _WIN64
		return SymUnloadModule(hProcess, (ULONG_PTR)LoadHandle);
}


SYMBOL_INFOW   FindSymInfo = { NULL };//��ʱ�Ĳ��ҽ����ű���

//��ȡһ�����͵�����
BOOL GetTypeName(DWORD64 Address,ULONG TypeIndex,wchar_t NameBuf[200])
{
	wchar_t * pTypeName = NULL;
	if (TRUE == SymGetTypeInfo(hProcess, Address, TypeIndex, IMAGEHLP_SYMBOL_TYPE_INFO::TI_GET_SYMNAME, &pTypeName)){
		__try{

			if (NULL != pTypeName && 200 > wcslen(pTypeName)){
				CopyMemory(NameBuf, pTypeName, wcslen(pTypeName) * sizeof(wchar_t)+sizeof(wchar_t));
				return TRUE;
			}
		}__except (EXCEPTION_EXECUTE_HANDLER){ return FALSE; }
	}
	return FALSE;
}

BOOL UnSymHandler()
{
	return SymCleanup(hProcess);
}


