#include "Vm.h"
#include "VT-x.h"
#include "CPU.h"
#include "ControlArea.h"
#include "amd64vm.h"



extern PVIRT_CPU	*CpuControlArea; 

VOID VmStart(PVOID StartContext)
{
	
	KMUTEX Mutex;
	NTSTATUS Status =STATUS_UNSUCCESSFUL;
	LONG i = 0;
	UNREFERENCED_PARAMETER(StartContext);

	Status = VTxHardwareStatus();  //利用cpuid指令检测是否支持VMX


	if (!NT_SUCCESS(Status))
	{
		DbgLog("Intel VT-x is not supported (0x%X)\n", Status);
		return;
	}

	//开启VMX operation的的前期设置  
	Status = VTxEnableProcessors(KeNumberProcessors);

	if (!NT_SUCCESS(Status))
	{
		DbgLog("Unable to prepare processors for virtualization (0x%X)\n", Status);
		return;
	}

	//
	// Synchronize
	//
	KeInitializeMutex(&Mutex, 0);
	KeWaitForSingleObject(&Mutex, Executive, KernelMode, FALSE, NULL);

	//
	// Control area for saving states and VM information
	//
	Status = ControlAreaInitialize(KeNumberProcessors);

	if (!NT_SUCCESS(Status))
	{
		DbgLog("Unable to initialize control area (0x%X)\n", Status);
		return;
	}

	//
	// Start virtualization
	//
	DbgLog("Virtualizing %d processors...\n", KeNumberProcessors);

	for (i = 0; i < KeNumberProcessors; i++) 
	{
		KAFFINITY oldAffinity	= KeSetSystemAffinityThreadEx((KAFFINITY)(1 << i));
		KIRQL oldIrql			= KeRaiseIrqlToDpcLevel();

		_StartVirtualization();

		KeLowerIrql(oldIrql);
		KeRevertToUserAffinityThreadEx(oldAffinity);
	}

	DbgLog("Done\n");

	KeReleaseMutex(&Mutex, FALSE);
	

}




NTSTATUS StartVirtualization(PVOID GuestRsp)  //在汇编中压栈了，传入的  mov rcx esp
{
	ULONG ProcessorId	= KeGetCurrentProcessorNumber();
	NTSTATUS status		= ControlAreaInitializeProcessor(ProcessorId);
	PVIRT_CPU cpu		= CpuControlArea[ProcessorId];

	if (!NT_SUCCESS(status))
	{
		DbgLog("Failed ControlAreaInitializeProcessor 0x%x\n", status);
		return status;
	}

	CpuSetupVMCS(cpu, GuestRsp);

	status = Virtualize(cpu);

	if (!NT_SUCCESS(status))
	{
		DbgLog("Failed Virtualize\n");
		return status;
	}

	return STATUS_SUCCESS;
}


CHAR VmIsActive()
{
	__try
	{
		return _QueryVirtualization();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

	return FALSE;
}