#include "VmExitHandler.h"
#include "vmx.h"
#include "VMInterrupt.h"
#include "amd64.h"


extern ULONG_PTR NtSyscallHandler;

NTSTATUS NTAPI HandleUnimplemented(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	DbgLog("vmx: unimplemented\n");
	DbgLog("vmx: exitcode = 0x%llx\n", Cpu->ExitReason);
	DbgLog("vmx: rip = 0x%llx\n", Cpu->rip);

	Cpu->rip += InstructionLength;

	return STATUS_NOT_IMPLEMENTED;
}



NTSTATUS NTAPI HandleException(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	ULONG32 Event, InjectEvent;
	ULONG64 ErrorCode, ExitQualification;
	PINTERRUPT_INFO_FIELD pEvent;
	PINTERRUPT_INJECT_INFO_FIELD pInjectEvent;

	Event = (ULONG32)__readvmx(VM_EXIT_INTR_INFO);
	pEvent = (PINTERRUPT_INFO_FIELD)&Event;

	InjectEvent = 0;
	pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;

	ErrorCode = __readvmx(VM_EXIT_INTR_ERROR_CODE);

	if (pEvent->ErrorCodeValid)
		__vmx_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);

	switch (pEvent->InterruptionType)
	{
	case INTERRUPT_NMI:
		DbgLog("vmx: HandleNmi()\n");
		VmInjectInterrupt(INTERRUPT_NMI, VECTOR_NMI_INTERRUPT, 0);
		break;

	case INTERRUPT_EXTERNAL:
		DbgLog("vmx: HandleExternalInterrupt()\n");
		break;

	case INTERRUPT_HARDWARE_EXCEPTION:
		switch (pEvent->Vector)
		{
		case VECTOR_DEBUG_EXCEPTION:
			//
			// INT1
			//
			DbgLog("vmx: int1 rip = 0x%llx\n", Cpu->rip);
			VmInjectInterrupt(pEvent->InterruptionType, pEvent->Vector, InstructionLength);
			break;

		case VECTOR_INVALID_OPCODE_EXCEPTION:
			//
			// INVALID OPCODE
			//
			DbgLog("vmx: Invalid opcode rip = 0x%llx\n", Cpu->rip);

			VmInjectInterrupt(pEvent->InterruptionType, pEvent->Vector, InstructionLength);
			break;

		case VECTOR_PAGE_FAULT_EXCEPTION:
			//
			// PAGE FAULT
			//
			ExitQualification = __readvmx(EXIT_QUALIFICATION);

			__writecr2(ExitQualification);
			//__vmx_vmwrite(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
			VmInjectInterrupt(pEvent->InterruptionType, pEvent->Vector, InstructionLength);
			break;

		default:
			DbgLog("vmx: Hardware Exception (vector=0x%x)\n", pEvent->Vector);
			break;
		}

		break;

	case INTERRUPT_SOFTWARE_EXCEPTION:
		switch (pEvent->Vector)
		{
		case VECTOR_BREAKPOINT_EXCEPTION:
			//
			// INT3
			//
			DbgLog("vmx: int3 rip = 0x%llx\n", Cpu->rip);

			VmInjectInterrupt(INTERRUPT_SOFTWARE_EXCEPTION, VECTOR_BREAKPOINT_EXCEPTION, InstructionLength);
			break;

		case VECTOR_OVERFLOW_EXCEPTION:
		default:
			DbgLog("vmx: Software Exception (vector=0x%x)\n", pEvent->Vector);
			break;
		}
		break;

	default:
		DbgLog("vmx: Unknown interruption type\n");
		break;
	}

	return STATUS_SUCCESS;
}



NTSTATUS NTAPI HandleCpuid(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	//
	// CPUID
	//
	int cpuInfo[4];
	__cpuidex(cpuInfo, (ULONG)Cpu->rax, (ULONG)Cpu->rcx);

	Cpu->rax = cpuInfo[0];
	Cpu->rbx = cpuInfo[1];
	Cpu->rcx = cpuInfo[2];
	Cpu->rdx = cpuInfo[3];
	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}


NTSTATUS NTAPI HandleInvd(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	//
	// Invalidate Internal Caches
	//
	__invd();

	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}



NTSTATUS NTAPI HandleRdpmc(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	//
	// Read Performance Monitor Counter
	//
	LARGE_INTEGER pmc;
	pmc.QuadPart = __readpmc((ULONG)Cpu->rcx);

	//
	// Update registers
	//
	Cpu->rax = pmc.LowPart;
	Cpu->rdx = pmc.HighPart;
	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}



NTSTATUS NTAPI HandleRdtsc(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	//
	// EDX:EAX <- TimeStampCounter
	//
	LARGE_INTEGER tsc;
	tsc.QuadPart = __rdtsc();

	//
	// Update registers
	//
	Cpu->rax = tsc.LowPart;
	Cpu->rdx = tsc.HighPart;
	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}



NTSTATUS NTAPI HandleVmCall(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	DbgLog("VMCALL: RIP = 0x%llx\n", Cpu->rip);

	//
	// Does RCX contain the magic value?
	//
	if (Cpu->rcx != 0x5644626748696465)
	{
		//
		// Raise interrupt and act like the instruction doesn't exist
		//
		VmInjectInterrupt(INTERRUPT_HARDWARE_EXCEPTION, VECTOR_INVALID_OPCODE_EXCEPTION, InstructionLength);
		return STATUS_SUCCESS;
	}

	//
	// Handle the function identified in RAX
	//
	switch ((ULONG)Cpu->rax)
	{
	case 0:
		{
			// QueryVirtualization (Active)
			Cpu->rax = TRUE;
		}
		break;

	case 0xFFFFFFFF:
		{
			// StopVirtualization
			__debugbreak();
		}
		break;
	}

	Cpu->rip += InstructionLength;
	return STATUS_SUCCESS;
}


NTSTATUS NTAPI HandleVmInstruction(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	DbgLog("VmInstruction: rip = 0x%llx\n", Cpu->rip);

	//
	// Indicate failure by setting the 'VM_FAIL_INVALID' flag
	//
	Cpu->rflags |= 0x1;
	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}




NTSTATUS NTAPI HandleCrAccess(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	size_t exitQualificationVal = __readvmx(EXIT_QUALIFICATION);
	PMOV_CR_QUALIFICATION exitQualification = (PMOV_CR_QUALIFICATION)&exitQualificationVal;

	//
	// Handle the move type
	//
	if (exitQualification->AccessType == VMX_CONTROL_REG_ACCESS_TYPE_MOV_TO_CR)
	{
		//
		// Get the register value
		//
		ULONG64 reg = CpuGetRegister(Cpu, exitQualification->Register);

		//
		// Moving value TO the control register
		//
		switch (exitQualification->ControlRegister)
		{
		case CR0:	__vmx_vmwrite(GUEST_CR0, reg);	break;
		case CR3:	__vmx_vmwrite(GUEST_CR3, reg);	break;
		case CR4:	__vmx_vmwrite(GUEST_CR4, reg);	break;
		case CR8:	Cpu->ExitIRQL = (KIRQL)reg;		break;
		default:	__debugbreak();					break;
		}
	}
	else if (exitQualification->AccessType == VMX_CONTROL_REG_ACCESS_TYPE_MOV_FROM_CR)
	{
		//
		// Read value of the control register
		//
		ULONG64 cr = 0;

		switch (exitQualification->ControlRegister)
		{
		case CR0:	__vmx_vmread(GUEST_CR0, &cr);	break;
		case CR3:	__vmx_vmread(GUEST_CR3, &cr);	break;
		case CR4:	__vmx_vmread(GUEST_CR4, &cr);	break;
		case CR8:	cr = Cpu->ExitIRQL;				break;
		default:	__debugbreak();					break;
		}

		//
		// Moving value FROM the control register
		//
		CpuSetRegister(Cpu, exitQualification->Register, cr);
	}
	else
	{
		// NOTE: Possible instructions used: CLTS/LMSW
		//
		// Invalid
		//
		__debugbreak();
	}

	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI HandleDrAccess(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	DbgLog("DrAccess -- Src: 0x%llx\n", Cpu->rip);

	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI HandleMsrRead(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	LARGE_INTEGER msr;
	ULONG ecx = (ULONG)Cpu->rcx;

	switch (ecx)
	{
	case MSR_IA32_SYSENTER_CS:	msr.QuadPart = __readvmx(GUEST_SYSENTER_CS);	break;
	case MSR_IA32_SYSENTER_ESP:	msr.QuadPart = __readvmx(GUEST_SYSENTER_ESP);	break;
	case MSR_IA32_SYSENTER_EIP:	msr.QuadPart = __readvmx(GUEST_SYSENTER_EIP);	break;
	case MSR_GS_BASE:			msr.QuadPart = __readvmx(GUEST_GS_BASE);		break;
	case MSR_FS_BASE:			msr.QuadPart = __readvmx(GUEST_FS_BASE);		break;

	case MSR_LSTAR:				msr.QuadPart = NtSyscallHandler;DbgPrint("PG LSTAR - 0x%p\n", Cpu->rip);break;

	default:					msr.QuadPart = __readmsr(ecx);					break;
	}

	Cpu->rax = msr.LowPart;
	Cpu->rdx = msr.HighPart;
	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI HandleMsrWrite(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	LARGE_INTEGER msr;

	ULONG ecx		= (ULONG)Cpu->rcx;
	msr.LowPart		= (ULONG)Cpu->rax;
	msr.HighPart	= (ULONG)Cpu->rdx;

	switch (ecx)
	{
	case MSR_IA32_SYSENTER_CS:
		//__writemsr(MSR_IA32_SYSENTER_CS, Msr.QuadPart);
		__vmx_vmwrite(GUEST_SYSENTER_CS, msr.QuadPart);
		break;

	case MSR_IA32_SYSENTER_ESP:
		//__writemsr(MSR_IA32_SYSENTER_ESP, Msr.QuadPart);
		__vmx_vmwrite(GUEST_SYSENTER_ESP, msr.QuadPart);
		break;

	case MSR_IA32_SYSENTER_EIP:
		//__writemsr(MSR_IA32_SYSENTER_EIP, Msr.QuadPart);
		__vmx_vmwrite(GUEST_SYSENTER_EIP, msr.QuadPart);
		break;

	case MSR_GS_BASE:
		//__writemsr(MSR_GS_BASE, Msr.QuadPart);
		__vmx_vmwrite(GUEST_GS_BASE, msr.QuadPart);
		break;

	case MSR_FS_BASE:
		//__writemsr(MSR_FS_BASE, Msr.QuadPart);
		__vmx_vmwrite(GUEST_FS_BASE, msr.QuadPart);
		break;

	case MSR_LSTAR:
		// Ignore all writes
		break;

	default:
		__writemsr(ecx, msr.QuadPart);
		break;
	}

	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI HandleRdtscp(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	//
	// EDX:EAX <- TimeStampCounter
	// ECX <- IA32_TSC_AUX[31:0]
	//
	LARGE_INTEGER tsc;
	UINT32 procId;
	tsc.QuadPart = __rdtscp(&procId);

	//
	// Update registers
	//
	Cpu->rax = tsc.LowPart;
	Cpu->rdx = tsc.HighPart;
	Cpu->rcx = procId;
	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI HandleXsetbv(PVIRT_CPU Cpu, ULONG InstructionLength)
{
	//
	// XCR[ECX] <- EDX:EAX
	//
	LARGE_INTEGER controlValue;
	controlValue.LowPart	= (ULONG)Cpu->rax;
	controlValue.HighPart	= (ULONG)Cpu->rdx;

	//
	// Set extended control register
	//
	/*����ָ����뱨��
	_xsetbv((ULONG)Cpu->rcx, controlValue.QuadPart);
	*/
	Cpu->rip += InstructionLength;

	return STATUS_SUCCESS;
}