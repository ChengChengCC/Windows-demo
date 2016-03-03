#pragma once

#include "Common.h"
#include <ntimage.h>


#define SEC_IMAGE 0x01000000


typedef struct _SYSTEM_SERVICE_TABLE{
	LONG*  		ServiceTableBase; 
	PVOID  		ServiceCounterTableBase; 
	ULONG  	NumberOfServices; 
	PVOID  		ParamTableBase; 
} SYSTEM_SERVICE_TABLE, *PSYSTEM_SERVICE_TABLE;

// #ifdef _WIN64
// #define SYSTEM_SERVICE_TABLE  SYSTEM_SERVICE_TABLE64
// #define PSYSTEM_SERVICE_TABLE  PSYSTEM_SERVICE_TABLE64
// #else
// #define SYSTEM_SERVICE_TABLE  SYSTEM_SERVICE_TABLE32
// #define PSYSTEM_SERVICE_TABLE  PSYSTEM_SERVICE_TABLE32
// #endif


NTSTATUS ServiceCallInitialize();

NTSTATUS AddServiceCallHook(ULONG Index, UCHAR ParameterCount, PVOID Function);
NTSTATUS RemoveServiceCallHook(ULONG Index);


LONG GetSSDTApiFunIndex(IN LPSTR lpszFunName);

ULONG_PTR GetSSDTFunctionAddress(ULONG TableIndex);

NTSTATUS MapFileInUserSpace(IN LPWSTR lpszFileName,IN HANDLE ProcessHandle OPTIONAL,
	OUT PVOID *BaseAddress,
	OUT PSIZE_T ViewSize OPTIONAL);
NTSYSAPI PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(PVOID Base);

VOID SyscallEntryPoint();