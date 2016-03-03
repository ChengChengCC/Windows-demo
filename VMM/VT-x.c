
#include "amd64.h"
#include "VT-x.h"



NTSTATUS VTxHardwareStatus()   //����CPUID ָ�����Ƿ�֧��VMX
{
	/*
	mov eax , 0   ; ���ܺ�0��main leaf��
	cpuid		  ����ѯ0����Ϣ
	���ص���Ӧ��Ϣ��32λ��
	��� 32λ ����eax , ebx , ecx ���Լ�edx�Ĵ�����
	     64λ ����rax , rbx , rcx , �Լ�rdx�Ĵ����У���32λ����
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
	���ܺ�0ҳ���ش���������������Intel�Ļ����Ϸ��ص��ǣ�
	ebx �Ĵ�����"Genu"
	ecx �Ĵ�����"ntel"
	eax �Ĵ�����"intel"
	�����������"GenuineIntel"
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
	ͨ�� CPUID ָ�����Ƿ�֧��VMX
	CPUID.01H:ECX[5]
	���庬��ο�����CPUID��ָ����ͣ�01HΪmain-leaf
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
		ʹ��ǰ�߳��ڲ�ͬ������������
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
	CR0.PE=0ʱ����ʾ����ʵģʽ�£�Ϊ1ʱ����ʾ���ڱ���ģʽ

	CR0.PG��1ʱ����ʼҳʽ������ƣ�
	����ҳʽ����ǰ����Ҫ�򿪱���ģʽ��
	���򽫲���#GP�쳣
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
		CR0.NE[bit 5]����x87 FPU��Ԫʹ�������쳣����ģʽ��
		native��ԭ����ģʽ��DOS compatibilityģʽ
		CR0.NE = 1ʱ��ʹ��native�쳣����ģʽ��������x87 FPU numeric �쳣ʱ�ɴ�����ֱ�Ӵ���
		CR0.NE = 0ʱ��ʹ��DOS-compatibility�쳣����ģʽ��������x87 FPU numeric�쳣ʱ��
					  ��������FERR# pin ���ӵ��ⲿ��PIC���жϿ���������8259��I/O APIC����
					  IRQ13��Ч���پ�8259�жϿ����������ⲿ�ж����󣬴�������Ӧִ��IRQ13�жϷ������̡�
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