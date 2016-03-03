#include "NewSysCall.h"
#include "Nt.h"
#include "NtType.h"

extern ULONG_PTR    g_Ori_NtClose;
extern ULONG_PTR	g_Ori_NtQueryInformationProcess;
extern ULONG_PTR	g_Ori_NtQueryObject;
extern ULONG_PTR	g_Ori_NtQuerySystemInformation ;
extern ULONG_PTR	g_Ori_NtReadVirtualMemory;
extern ULONG_PTR	g_Ori_NtSetInformationThread;
extern ULONG_PTR	g_Ori_NtSystemDebugControl;


NTSTATUS NTAPI hk_NtReadVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, SIZE_T NumberOfBytesToRead, PSIZE_T NumberOfBytesRead)
{

   	  return ((PfnNtReadVirtualMemory)g_Ori_NtReadVirtualMemory)(ProcessHandle, BaseAddress, 
		  Buffer, NumberOfBytesToRead,NumberOfBytesRead);
}
