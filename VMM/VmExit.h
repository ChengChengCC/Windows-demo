#pragma once

#include "Common.h"
#include "CPU.h"
#include "amd64.h"


typedef NTSTATUS (NTAPI * VmExitCallback)(PVIRT_CPU Cpu, ULONG InstructionLength);

VOID HandleVmExit(PVIRT_CPU Cpu, PGUEST_REGS GuestRegs);