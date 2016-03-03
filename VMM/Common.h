/***************************************************************************************
* AUTHOR : ¿¡»À
* DATE   : 2016-12-5
* MODULE : VTHideDbg.H
*
* IOCTRL Sample Driver
*
* Description:
*		Demonstrates communications between USER and KERNEL.
*
****************************************************************************************
* Copyright (C) 2010 ¿¡»À.
****************************************************************************************/
#pragma once
#ifndef CXX_VTHIDEDBG_H
#define CXX_VTHIDEDBG_H


#include <ntifs.h>
#include <devioctl.h>
#include <aux_klib.h>

#define DbgLog(Format, ...) DbgPrint("vmm[#%d][IRQL=0x%x](%s): " Format, KeGetCurrentProcessorNumber(), KeGetCurrentIrql(), __FUNCTION__, __VA_ARGS__);

#define TO_ULL(x)		(*(ULONGLONG *)(&x))
#define PA_PTR_INT64(x) (UINT64 *)(&((x).QuadPart))

//////////////////////////////////////////////////////////////////////////

#endif	//CXX_VTHIDEDBG_H
/* EOF */
