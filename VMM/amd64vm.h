#pragma once
#include "Common.h"



NTSTATUS _StartVirtualization();
VOID _StopVirtualization();
CHAR _QueryVirtualization();

VOID _GuestEntry();
VOID _ExitHandler();
VOID _GuestExit();