#pragma once

#include "Common.h"
#include "amd64.h"

NTSTATUS InitializeSegmentSelector(PSEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase);
NTSTATUS FillGuestSelectorData(PVOID GdtBase, ULONG Segreg, USHORT Selector);
ULONG AdjustControls(ULONG Ctl, ULONG Msr);