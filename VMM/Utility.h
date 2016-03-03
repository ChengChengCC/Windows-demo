#pragma once

#include "Common.h"


NTSTATUS NTAPI ZwQuerySystemInformation(
	ULONG_PTR SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
	);

typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY64
{
	ULONG Unknow1;
	ULONG Unknow2;
	ULONG Unknow3;
	ULONG Unknow4;
	PVOID Base;
	ULONG Size;
	ULONG Flags;
	USHORT Index;
	USHORT NameLength;
	USHORT LoadCount;
	USHORT ModuleNameOffset;
	char ImageName[256];
} SYSTEM_MODULE_INFORMATION_ENTRY64, *PSYSTEM_MODULE_INFORMATION_ENTRY64;



typedef struct _SYSTEM_MODULE_INFORMATION_ENTRY32
{
	ULONG  Reserved[2];  
	ULONG  Base;        
	ULONG  Size;         
	ULONG  Flags;        
	USHORT Index;       
	USHORT Unknown;     
	USHORT LoadCount;   
	USHORT ModuleNameOffset;
	CHAR   ImageName[256];   
} SYSTEM_MODULE_INFORMATION_ENTRY32, *PSYSTEM_MODULE_INFORMATION_ENTRY32;



#ifdef _WIN64
#define SYSTEM_MODULE_INFORMATION_ENTRY SYSTEM_MODULE_INFORMATION_ENTRY64
#else
#define SYSTEM_MODULE_INFORMATION_ENTRY SYSTEM_MODULE_INFORMATION_ENTRY32
#endif


typedef struct _tagSysModuleList {          //Ä£¿éÁ´½á¹¹
	ULONG ulCount;
	SYSTEM_MODULE_INFORMATION_ENTRY smi[1];
} MODULES, *PMODULES;

ULONG_PTR GetNtoskrnlBase();
ULONG_PTR GetSSDTBase();
ULONG_PTR GetSSDTFunctionAddress(ULONG TableIndex);