#pragma once

#include "Common.h"


VOID VmStart(PVOID StartContext);


NTSTATUS StartVirtualization(PVOID GuestRsp);

CHAR VmIsActive();