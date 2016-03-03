#pragma once
#include "Common.h"


typedef struct _VIRT_CPU
{
	PVOID				Self;			// Pointer to this structure

	ULONG32				ProcessorId;	// Processor number
	ULONG32				ThreadId;		// Thread number

	PVOID				VmxonVa;		// VMXON region virtual address
	PHYSICAL_ADDRESS	VmxonPa;		// VMXON region physical address
	SIZE_T				VmxonSize;		// Size of the region

	PVOID				VmcsVa;			// VMCX region virtual address
	PHYSICAL_ADDRESS	VmcsPa;			// VMCS region physical address
	SIZE_T				VmcsSize;		// Size of the region

	PVOID				MSRBitmapVa;	// MSR bitmap virtual address
	PHYSICAL_ADDRESS	MSRBitmapPa;	// MSR bitmap physical address
	SIZE_T				MSRBitmapSize;	// Size of the region

	PVOID				HostStackBase;	// Stack base of the host entry point
	SIZE_T				HostStackSize;	// Stack size

	ULONG64				ExitReason;		// VM exit reason code
	KIRQL				ExitIRQL;		// IRQL at HandleVmExit, set before VMRESUME
	KIRQL				PreviousIRQL;	// Old IRQL value

	ULONG64				rip;			// Technically not a register, handle it differently
	ULONG64				rflags;			// Same as above

	union
	{
		struct
		{
			ULONG64 rax;
			ULONG64 rcx;
			ULONG64 rdx;
			ULONG64 rbx;
			ULONG64 rsp;
			ULONG64 rbp;
			ULONG64 rsi;
			ULONG64 rdi;
			ULONG64 r8;
			ULONG64 r9;
			ULONG64 r10;
			ULONG64 r11;
			ULONG64 r12;
			ULONG64 r13;
			ULONG64 r14;
			ULONG64 r15;
		};

		ULONG64 Registers[16];
	};

} VIRT_CPU, *PVIRT_CPU;



VOID CpuSetupVMCS(PVIRT_CPU Cpu, PVOID GuestRsp);
NTSTATUS Virtualize(PVIRT_CPU pCpu);


FORCEINLINE VOID CpuSetRegister(PVIRT_CPU Cpu, ULONG Index, ULONG64 Value)
{
#ifdef _DEBUG
	if (Index >= ARRAYSIZE(Cpu->Registers))
		__debugbreak();
#endif

	Cpu->Registers[Index] = Value;
}

FORCEINLINE ULONG64 CpuGetRegister(PVIRT_CPU Cpu, ULONG Index)
{
#ifdef _DEBUG
	if (Index >= ARRAYSIZE(Cpu->Registers))
		__debugbreak();
#endif

	return Cpu->Registers[Index];
}
