#pragma  once

#include <Windows.h>
#include <DbgHelp.h> 

#pragma comment(lib, "DbgHelp.lib")	//·ûºÅAPI

typedef struct _MY_SYMBOL_INFO
{
	int index;
	char  name[MAX_PATH];
	wchar_t  type[MAX_PATH];
	ULONG_PTR symbol_addr;
	ULONG_PTR module_addr;
	ULONG_PTR offset;
}MY_SYMBOL_INFO,*PMY_SYMBOL_INFO;

BOOL InitSymHandler(bool OutDebug = true);
BOOL GetFileSymbolsA(IN char * module_path, OUT char * symbols_file_name, unsigned int BufSize);
BOOL GetFileSymbolsW(IN wchar_t * module_path, OUT wchar_t * symbols_file_name, unsigned int BufSize);
LONG_PTR LoadSymModuleA(char * module_path, ULONG_PTR module_addr, DWORD ModuleSize);
LONG_PTR LoadSymModuleW(wchar_t * module_path, ULONG_PTR module_addr, DWORD ModuleSize);
BOOL GetTypeName(DWORD64 Address,ULONG TypeIndex,wchar_t NameBuf[200]);
BOOL UnLoadSymModule(ULONG_PTR LoadHandle);
BOOL UnSymHandler();