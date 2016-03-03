#pragma once

#include "Common.h"
#include "Nt.h"



typedef NTSTATUS (*PfnNtClose)(HANDLE Handle);

typedef NTSTATUS (*PfnNtQueryInformationProcess)
	(HANDLE ProcessHandle, 
	 PROCESSINFOCLASS ProcessInformationClass, 
	 PVOID ProcessInformation, 
	 ULONG ProcessInformationLength, 
	 PULONG ReturnLength);

typedef NTSTATUS (*PfnNtQueryObject)
	(HANDLE Handle, 
	 OBJECT_INFORMATION_CLASS ObjectInformationClass, 
	 PVOID ObjectInformation,
	 ULONG ObjectInformationLength,
	 PULONG ReturnLength);

typedef NTSTATUS (*PfnNtQuerySystemInformation) 
	(SYSTEM_INFORMATION_CLASS SystemInformationClass,
	 PVOID SystemInformation,
	 ULONG SystemInformationLength,
	 PULONG ReturnLength);

typedef NTSTATUS (*PfnNtReadVirtualMemory)
	(HANDLE ProcessHandle,
	PVOID BaseAddress,
	PVOID Buffer,
	SIZE_T NumberOfBytesToRead,
	PSIZE_T NumberOfBytesRead);

typedef NTSTATUS (*PfnNtSetInformationThread)
	(HANDLE ThreadHandle,
	 THREADINFOCLASS ThreadInformationClass,
	 PVOID ThreadInformation,
	 ULONG ThreadInformationLength);

typedef NTSTATUS (*PfnNtSystemDebugControl)
	(DEBUG_CONTROL_CODE ControlCode, 
	PVOID InputBuffer, 
	ULONG InputBufferLength, 
	PVOID OutputBuffer,
	ULONG OutputBufferLength,
	PULONG ReturnLength);