#pragma once

#include "Common.h"


NTSTATUS NTAPI hk_NtReadVirtualMemory(
	HANDLE ProcessHandle, 
	PVOID BaseAddress, 
	PVOID Buffer, 
	SIZE_T NumberOfBytesToRead, 
	PSIZE_T NumberOfBytesRead);