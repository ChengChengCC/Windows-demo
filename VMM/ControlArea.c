#include "amd64.h"
#include "ControlArea.h"
#include "CPU.h"



LONG		CpuControlAreaCount;
LONG		CpuControlAreaSize;
PVIRT_CPU	*CpuControlArea;  //指针数组，每个处理器都有一个VIRT_CPU结构

NTSTATUS ControlAreaInitialize(LONG ProcessorCount)
{
	
	CpuControlAreaCount = ProcessorCount;
	CpuControlAreaSize	= ROUND_TO_PAGES(sizeof(PVIRT_CPU) * CpuControlAreaCount);
	CpuControlArea		=(PVIRT_CPU*)ExAllocatePoolWithTag(NonPagedPool, CpuControlAreaSize, 'CCTL');

	if (!CpuControlArea)
		return STATUS_NO_MEMORY;

	RtlSecureZeroMemory(CpuControlArea, CpuControlAreaSize);

	return STATUS_SUCCESS;
}



NTSTATUS ControlAreaInitializeProcessor(LONG ProcessorNumber)
{
	//
	// Allocate host stack region
	// 16 pages available for use
	//
	PVIRT_CPU cpu = NULL;
	SIZE_T StackSize = 16 * PAGE_SIZE;
	PUCHAR StackBase = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, StackSize, 'KSTK');

	if (!StackBase)
		return STATUS_NO_MEMORY;

	RtlSecureZeroMemory((PVOID)StackBase, StackSize);

	//
	// 设置当前CPU的VIRT_CPU结构，将指针放入CpuControlArea数组中
	//
	cpu = (PVIRT_CPU)(StackBase + StackSize - 8 - sizeof(VIRT_CPU));
	cpu->HostStackBase	= StackBase;
	cpu->Self			= cpu;

	CpuControlArea[ProcessorNumber] = cpu;

	//
	// Allocate all VMX regions
	//
	if (!NT_SUCCESS(AllocateVmxProcessorData(&cpu->VmxonVa, &cpu->VmxonPa, &cpu->VmxonSize)))
		return STATUS_NO_MEMORY;

	if (!NT_SUCCESS(AllocateVmxProcessorData(&cpu->VmcsVa, &cpu->VmcsPa, &cpu->VmcsSize)))
		return STATUS_NO_MEMORY;

	if (!NT_SUCCESS(AllocateVmxProcessorData(&cpu->MSRBitmapVa, &cpu->MSRBitmapPa, &cpu->MSRBitmapSize)))
		return STATUS_NO_MEMORY;

	// Bitmap needs to be zeroed
	//多余了，申请内存的时候已经初始化过了
	//RtlSecureZeroMemory(cpu->MSRBitmapVa, cpu->MSRBitmapSize);

	__try
	{
		if (__vmx_on(PA_PTR_INT64(cpu->VmxonPa)) > 0)
			return STATUS_UNSUCCESSFUL;

		if (__vmx_vmclear(PA_PTR_INT64(cpu->VmcsPa)) > 0)
			return STATUS_UNSUCCESSFUL;

		if (__vmx_vmptrld(PA_PTR_INT64(cpu->VmcsPa)) > 0)
			return STATUS_UNSUCCESSFUL;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// Rare case (or if physical address is invalid)
		return GetExceptionCode();
	}

	return STATUS_SUCCESS;
}



NTSTATUS AllocateVmxProcessorData(PVOID *VirtualAddress, PHYSICAL_ADDRESS *PhysicalAddress, SIZE_T *Size)
{
	VMX_BASIC_MSR msr;
	PVOID Address;
	PHYSICAL_ADDRESS l1, l2, l3;

	if (!VirtualAddress || !PhysicalAddress || !Size)
		return STATUS_INVALID_PARAMETER;

	//
	// Read the MSR information to get the base size
	// Default to 4096 bytes
	//

	TO_ULL(msr) = __readmsr(MSR_IA32_VMX_BASIC);

	//通过检查 IA32_VMX_BASIC 寄存器来获得 VMXON区域的大小，也可以获得cache类型
	if (*Size <= 0)
	{
		// In rare cases this isn't set (*COUGH* *VMWARE*)
		if (msr.szVmxOnRegion > 0)
			*Size = msr.szVmxOnRegion;
		else
			*Size = 0x1000;

		*Size = ROUND_TO_PAGES(*Size);
	}

	//
	// Allocate CONTIGUOUS physical memory
	// MmCached = Stored in CPU L1/L2/L3 cache if possible 
	//
	//这里申请内存的要求和NewBluePill中一样
	
	l1.QuadPart = 0;
	l2.QuadPart = -1;
	l3.QuadPart = 0x200000;

	Address = MmAllocateContiguousMemorySpecifyCache(*Size, l1, l2, l3, MmCached);  //Uninitialize

	if (!Address)
		return STATUS_NO_MEMORY;

	RtlSecureZeroMemory(Address, *Size);

	//
	// 设置开始的4个字节的DWORD
	//
	*(ULONG *)Address = msr.RevId;//用VMCS ID 值来设置 VMON 和 VMCS 区域的首位置

	
	*VirtualAddress	 = Address;
	*PhysicalAddress = MmGetPhysicalAddress(Address);

	return STATUS_SUCCESS;
}