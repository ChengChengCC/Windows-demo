#include "amd64.h"
#include "ControlArea.h"
#include "CPU.h"



LONG		CpuControlAreaCount;
LONG		CpuControlAreaSize;
PVIRT_CPU	*CpuControlArea;  //ָ�����飬ÿ������������һ��VIRT_CPU�ṹ

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
	// ���õ�ǰCPU��VIRT_CPU�ṹ����ָ�����CpuControlArea������
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
	//�����ˣ������ڴ��ʱ���Ѿ���ʼ������
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

	//ͨ����� IA32_VMX_BASIC �Ĵ�������� VMXON����Ĵ�С��Ҳ���Ի��cache����
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
	//���������ڴ��Ҫ���NewBluePill��һ��
	
	l1.QuadPart = 0;
	l2.QuadPart = -1;
	l3.QuadPart = 0x200000;

	Address = MmAllocateContiguousMemorySpecifyCache(*Size, l1, l2, l3, MmCached);  //Uninitialize

	if (!Address)
		return STATUS_NO_MEMORY;

	RtlSecureZeroMemory(Address, *Size);

	//
	// ���ÿ�ʼ��4���ֽڵ�DWORD
	//
	*(ULONG *)Address = msr.RevId;//��VMCS ID ֵ������ VMON �� VMCS �������λ��

	
	*VirtualAddress	 = Address;
	*PhysicalAddress = MmGetPhysicalAddress(Address);

	return STATUS_SUCCESS;
}