#include "Nt.h"
#include <ntimage.h>
#include "NtType.h"
#include "Hook.h"

ULONG_PTR   g_Ori_NtClose = NULL;
ULONG_PTR	g_Ori_NtQueryInformationProcess = NULL;
ULONG_PTR	g_Ori_NtQueryObject = NULL;
ULONG_PTR	g_Ori_NtQuerySystemInformation = NULL;
ULONG_PTR	g_Ori_NtReadVirtualMemory = NULL;
ULONG_PTR	g_Ori_NtSetInformationThread = NULL;
ULONG_PTR	g_Ori_NtSystemDebugControl = NULL;


NTSTATUS Nt_Initialize()
{
	g_Ori_NtClose = GetSSDTFunctionAddress(GetSSDTApiFunIndex("NtClose"));
    g_Ori_NtQueryInformationProcess = GetSSDTFunctionAddress(GetSSDTApiFunIndex("NtQueryInformationProcess"));
    g_Ori_NtQueryObject = GetSSDTFunctionAddress(GetSSDTApiFunIndex("NtQueryObject"));
	g_Ori_NtQuerySystemInformation = GetSSDTFunctionAddress(GetSSDTApiFunIndex("NtQuerySystemInformation"));
	g_Ori_NtReadVirtualMemory = GetSSDTFunctionAddress(GetSSDTApiFunIndex("NtReadVirtualMemory"));
	g_Ori_NtSetInformationThread = GetSSDTFunctionAddress(GetSSDTApiFunIndex("NtSetInformationThread"));
	g_Ori_NtSystemDebugControl = GetSSDTFunctionAddress(GetSSDTApiFunIndex("NtSystemDebugControl"));

	if (!MmIsAddressValid((PVOID)g_Ori_NtClose)
		||!MmIsAddressValid((PVOID)g_Ori_NtQueryInformationProcess)
		||!MmIsAddressValid((PVOID)g_Ori_NtQueryObject)
		||!MmIsAddressValid((PVOID)g_Ori_NtQuerySystemInformation)
		||!MmIsAddressValid((PVOID)g_Ori_NtReadVirtualMemory)
		||!MmIsAddressValid((PVOID)g_Ori_NtSetInformationThread)
		||!MmIsAddressValid((PVOID)g_Ori_NtSystemDebugControl))
	{
		return STATUS_UNSUCCESSFUL;
	}


	return STATUS_SUCCESS;
}