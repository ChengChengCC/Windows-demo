
#include "Common.h"
#include "Utility.h"
#include "Hook.h"
#include "Nt.h"
#include "NewSysCall.h"
#include "amd64.h"

ULONG_PTR NtSyscallHandler;     //保存真正的KiSystemCall64
ULONG_PTR GuestSyscallHandler;

ULONG_PTR NtKernelBase;
ULONG_PTR NtKernelSSDT;

LONG SyscallHookEnabled[4096];  //FALSE: 已经hook  TRUE:没有hook
CHAR SyscallParamTable[4096];
PVOID SyscallPointerTable[4096];





NTSTATUS ServiceCallInitialize()
{
	
	NTSTATUS status ;//= AuxKlibInitialize();  //Aux_kilb.h
	ULONG Index = 0;
	
	NtKernelBase = GetNtoskrnlBase();
	NtKernelSSDT = GetSSDTBase();

	DbgLog("NtOSBase: 0x%llx\n", NtKernelBase);
	DbgLog("NtSSDT: 0x%llx\n", NtKernelSSDT);

	if (!NtKernelBase || !NtKernelSSDT)
		return STATUS_UNSUCCESSFUL;

	
	NtSyscallHandler	= (ULONG64)__readmsr(MSR_LSTAR);    //KiSystemCall64
	GuestSyscallHandler = (ULONG64)&SyscallEntryPoint;

	
	RtlSecureZeroMemory(SyscallHookEnabled, sizeof(SyscallHookEnabled));
	RtlSecureZeroMemory(SyscallParamTable, sizeof(SyscallParamTable));
	RtlSecureZeroMemory(SyscallPointerTable, sizeof(SyscallPointerTable));

	//
	// Init the function pointers
	//
	status = Nt_Initialize();

	if (!NT_SUCCESS(status))
		return status;

	//	AddServiceCallHook(0xE, 1, (PVOID)&hk_NtClose);
	//AddServiceCallHook(0x3E, 5, (PVOID)&hk_NtReadVirtualMemory);
	AddServiceCallHook(GetSSDTApiFunIndex("NtReadVirtualMemory"), 5, (PVOID)hk_NtReadVirtualMemory);
	//	AddServiceCallHook(0x35, 4, (PVOID)&hk_NtQuerySystemInformation);
	return STATUS_SUCCESS;
}



NTSTATUS AddServiceCallHook(ULONG Index, UCHAR ParameterCount, PVOID Function)
{

	KIRQL irql;

	if (Index >= ARRAYSIZE(SyscallHookEnabled))
		return STATUS_INVALID_PARAMETER_1;

	if (ParameterCount > 15)
		return STATUS_INVALID_PARAMETER_2;

	irql = KeGetCurrentIrql();

	if (irql < DISPATCH_LEVEL)
		irql = KeRaiseIrqlToDpcLevel();

	InterlockedExchange(&SyscallHookEnabled[Index], FALSE);
	SyscallParamTable[Index]	= ParameterCount;
	SyscallPointerTable[Index]	= Function;

	
	if (Function) 
		InterlockedExchange(&SyscallHookEnabled[Index], TRUE);

	
	if (KeGetCurrentIrql() > irql)
		KeLowerIrql(irql);

	return STATUS_SUCCESS;
}


NTSTATUS RemoveServiceCallHook(ULONG Index)
{
	return AddServiceCallHook(Index, 0, NULL);
}



LONG GetSSDTApiFunIndex(IN LPSTR lpszFunName)
{
	LONG Index = -1;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	PVOID    MapBase = NULL;
	PIMAGE_NT_HEADERS  NtHeader;
	PIMAGE_EXPORT_DIRECTORY ExportTable;
	ULONG*  FunctionAddresses;
	ULONG*  FunctionNames;
	USHORT* FunIndexs;
	ULONG   ulFunIndex;
	ULONG   i;
	CHAR*   FunName;
	SIZE_T  ViewSize=0;
	ULONG_PTR FunAddress;
	WCHAR wzNtdll[] = L"\\SystemRoot\\System32\\ntdll.dll";

	Status = MapFileInUserSpace(wzNtdll, NtCurrentProcess(), &MapBase, &ViewSize);
	if (!NT_SUCCESS(Status))
	{

		return STATUS_UNSUCCESSFUL;

	}
	else
	{
		__try{
			NtHeader = RtlImageNtHeader(MapBase);  
			if (NtHeader && NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress){
				ExportTable =(IMAGE_EXPORT_DIRECTORY *)((ULONG_PTR)MapBase + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
				FunctionAddresses = (ULONG*)((ULONG_PTR)MapBase + ExportTable->AddressOfFunctions);
				FunctionNames = (ULONG*)((ULONG_PTR)MapBase + ExportTable->AddressOfNames);
				FunIndexs = (USHORT*)((ULONG_PTR)MapBase + ExportTable->AddressOfNameOrdinals);
				for(i = 0; i < ExportTable->NumberOfNames; i++)
				{
					FunName = (LPSTR)((ULONG_PTR)MapBase + FunctionNames[i]);
					if (_stricmp(FunName, lpszFunName) == 0) 
					{
						ulFunIndex = FunIndexs[i]; 
						FunAddress = (ULONG_PTR)((ULONG_PTR)MapBase + FunctionAddresses[ulFunIndex]);
						Index=*(ULONG*)(FunAddress+4);  //64位是4 ，32位是1
						break;
					}
				}
			}
		}__except(EXCEPTION_EXECUTE_HANDLER)
		{

		}
	}

	if (Index == -1)
	{
		DbgPrint("%s Get Index Error\n", lpszFunName);
	}

	ZwUnmapViewOfSection(NtCurrentProcess(), MapBase);
	return Index;
}



NTSTATUS MapFileInUserSpace(IN LPWSTR lpszFileName,IN HANDLE ProcessHandle OPTIONAL,
	OUT PVOID *BaseAddress,
	OUT PSIZE_T ViewSize OPTIONAL)
{
	NTSTATUS Status = STATUS_INVALID_PARAMETER;
	HANDLE   hFile = NULL;
	HANDLE   hSection = NULL;
	OBJECT_ATTRIBUTES oa;
	SIZE_T MapViewSize = 0;
	IO_STATUS_BLOCK Iosb;
	UNICODE_STRING uniFileName;

	if (!lpszFileName || !BaseAddress){
		return Status;
	}

	RtlInitUnicodeString(&uniFileName, lpszFileName);
	InitializeObjectAttributes(&oa,
		&uniFileName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		NULL
		);

	Status = IoCreateFile(&hFile,
		GENERIC_READ | SYNCHRONIZE,
		&oa,
		&Iosb,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0,
		CreateFileTypeNone,
		NULL,
		IO_NO_PARAMETER_CHECKING
		);

	if (!NT_SUCCESS(Status))
	{
		DbgPrint("ZwCreateFile Failed! Error=%08x\n",Status);
		return Status;
	}

	oa.ObjectName = NULL;
	Status = ZwCreateSection(&hSection,
		SECTION_QUERY | SECTION_MAP_READ,
		&oa,
		NULL,
		PAGE_WRITECOPY,
		SEC_IMAGE,
		hFile
		);
	ZwClose(hFile);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("ZwCreateSection Failed! Error=%08x\n",Status);
		return Status;

	}

	if (!ProcessHandle){
		ProcessHandle = NtCurrentProcess();
	}

	Status = ZwMapViewOfSection(hSection, 
		ProcessHandle, 
		BaseAddress, 
		0, 
		0, 
		0, 
		ViewSize ? ViewSize : &MapViewSize, 
		ViewUnmap, 
		0, 
		PAGE_WRITECOPY
		);
	ZwClose(hSection);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("ZwMapViewOfSection Failed! Error=%08x\n",Status);
		return Status;
	}

	return Status;
}




ULONG_PTR GetSSDTFunctionAddress(ULONG TableIndex)
{
	ULONG_PTR Addr = 0;
	PSYSTEM_SERVICE_TABLE SSDT = (PSYSTEM_SERVICE_TABLE) NtKernelSSDT;

#ifdef _WIN64

	Addr = (ULONG_PTR)SSDT->ServiceTableBase + (SSDT->ServiceTableBase[TableIndex] >> 4);
#else
	Addr = (ULONG_PTR)SSDT->ServiceTableBase[TableIndex];
#endif

	if (!MmIsAddressValid((PVOID)Addr))
	{
		DbgPrint("FAILED INDEX IN GetSSDTFunctionAddress: 0x%X - 0x%p\n", TableIndex, Addr);
		return 0;
	}

	return Addr;
}