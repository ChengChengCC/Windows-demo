
#include "amd64.h"
#include "VT-x.h"



NTSTATUS VTxHardwareStatus()   //利用CPUID 指令检测是否支持VMX
{
	/*
	mov eax , 0   ; 功能号0（main leaf）
	cpuid		  ；查询0号信息
	返回的相应信息是32位的
	因此 32位 放在eax , ebx , ecx ，以及edx寄存器中
	     64位 放在rax , rbx , rcx , 以及rdx寄存器中，高32位清零
	*/
	int cpuInfo[4];
	__cpuid(cpuInfo, 0);

	//
	// Are newer feature levels supported?
	//
	if (cpuInfo[0] < 1)
	{
		DbgLog("Error: Extended CPUID functions not implemented\n");
		return STATUS_NOT_SUPPORTED;
	}

	//
	// GenuineIntel check
	//
	/*
	功能号0页返回处理器厂商名，在Intel的机器上返回的是：
	ebx 寄存器是"Genu"
	ecx 寄存器是"ntel"
	eax 寄存器是"intel"
	组合起来就是"GenuineIntel"
	*/
	if (cpuInfo[1] != 'uneG' ||    //ebx
		cpuInfo[2] != 'letn' ||    //ecx
		cpuInfo[3] != 'Ieni')      //edx
	{
	//	DbgLog("Error: Processor is not 'GenuineIntel':\n");

		int buffer[4];
		buffer[0] = cpuInfo[1];
		buffer[1] = cpuInfo[3];
		buffer[2] = cpuInfo[2];
		buffer[3] = 0;
		DbgPrint("%s\n", &buffer);

		return STATUS_NOT_SUPPORTED;
	}

	//
	// Check CPUID values to see if virtualization is supported
	//
	__cpuid(cpuInfo, 1);

	//
	// BIT #5 VMX
	//
	/*
	通过 CPUID 指令检测是否支持VMX
	CPUID.01H:ECX[5]
	具体含义参考上面CPUID的指令解释，01H为main-leaf
	*/
	if ((cpuInfo[2] & (1 << 5)) == 0)
	{
		DbgLog("Error: VMX not supported\n");
		return STATUS_NOT_SUPPORTED;
	}

	return STATUS_SUCCESS;
}




NTSTATUS VTxEnableProcessors(LONG ProcessorCount)
{
	NTSTATUS Status			= STATUS_SUCCESS;
	LONG ProcessorIndex		= 0;

	for (; ProcessorIndex < ProcessorCount; ProcessorIndex++)
	{
		/*
		使当前线程在不同处理器上运行
		*/
		KAFFINITY OldAffinity	= KeSetSystemAffinityThreadEx((KAFFINITY)(1 << ProcessorIndex));
		KIRQL OldIrql		    = KeRaiseIrqlToDpcLevel();  


		Status = VTxSoftwareStatus(); 
		KeLowerIrql(OldIrql);
		KeRevertToUserAffinityThreadEx(OldAffinity);

		if (!NT_SUCCESS(Status))
			break;
	}

	if (!NT_SUCCESS(Status) || ProcessorIndex != ProcessorCount)
	{
		DbgLog("Error: Unable to enable virtualization on all processors\n");
		return Status;
	}

	return STATUS_SUCCESS;
}



NTSTATUS VTxSoftwareStatus()  //IA32_FEATURE_CONTROL   CR0   CR4
{
	//
	// Check the feature control bit MSR
	//
	IA32_FEATURE_CONTROL_MSR msr;
	CR0_REG cr0;
	TO_ULL(msr) = __readmsr(MSR_IA32_FEATURE_CONTROL);

	if (msr.Lock == 1)
	{
		// If the MSR is locked, it can't be modified
		// If 'EnableVmxon' is unset, virtualization is not possible
		if (msr.EnableVmxon == 0)
		{
			DbgLog("VMX is disabled in bios: MSR_IA32_FEATURE_CONTROL is 0x%llx\n", msr);
			return STATUS_NOT_SUPPORTED;
		}
	}
	else
	{
		// Force the lock to be on and enable VMXON
		msr.Lock		= 1;
		msr.VmxonInSmx	= 1;
		msr.EnableVmxon = 1;

		__writemsr(MSR_IA32_FEATURE_CONTROL, TO_ULL(msr));
	}

	//
	// Setup CR0 correctly (Protected mode and paging must be enabled)
	//
	
	TO_ULL(cr0) = __readcr0();

	/*
	CR0.PE=0时，表示处于实模式下，为1时，表示处于保护模式

	CR0.PG置1时将开始页式管理机制，
	开启页式管理前必须要打开保护模式，
	否则将产生#GP异常
	*/
	if (cr0.PE == 0 || cr0.PG == 0)
	{
		DbgLog("Error: Protected mode or paging is not set in CR0\n");
		return STATUS_NOT_SUPPORTED;
	}
	else
	{
		// Required by first processors that supported VMX
		/*
		CR0.NE[bit 5]决定x87 FPU单元使用哪种异常处理模式：
		native（原生）模式和DOS compatibility模式
		CR0.NE = 1时，使用native异常处理模式。当发生x87 FPU numeric 异常时由处理器直接处理；
		CR0.NE = 0时，使用DOS-compatibility异常处理模式。当发生x87 FPU numeric异常时，
					  处理器的FERR# pin 连接到外部的PIC（中断控制器，如8259或I/O APIC）的
					  IRQ13有效。再经8259中断控制器发出外部中断请求，处理器响应执行IRQ13中断服务例程。
		*/
		cr0.NE = 1;
	}

	__writecr0(TO_ULL(cr0));

	//
	// Virtual Machine Extensions Enable in CR4
	// BIT #13 VMXE
	//
	__try
	{
		__writecr4(__readcr4() | (1 << 13));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// Possible 'Privileged Instruction Exception' with CR4 bits
		return GetExceptionCode();
	}

	return STATUS_SUCCESS;
}