#pragma once

#include "Common.h"

NTSTATUS ControlAreaInitialize(LONG ProcessorCount);

NTSTATUS ControlAreaInitializeProcessor(LONG ProcessorNumber);


NTSTATUS AllocateVmxProcessorData(PVOID *VirtualAddress, PHYSICAL_ADDRESS *PhysicalAddress, SIZE_T *Size);