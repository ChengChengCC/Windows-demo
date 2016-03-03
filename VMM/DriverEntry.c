/***************************************************************************************
* AUTHOR : 懒人
* DATE   : 2016-12-5
* MODULE : VTHideDbg.C
* 
* Command: 
*	Source of IOCTRL Sample Driver
*
* Description:
*		Demonstrates communications between USER and KERNEL.
*
****************************************************************************************
* Copyright (C) 2010 懒人.
****************************************************************************************/

//#######################################################################################
//# I N C L U D E S
//#######################################################################################

#ifndef CXX_VTHIDEDBG_H
#include "DriverEntry.h"
#endif

#include "Hook.h"
#include "vm.h"

//////////////////////////////////////////////////////////////////////////

//#######################################################################################
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@@@@@@@@				D R I V E R   E N T R Y   P O I N T						 @@@@@@@@
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//#######################################################################################
NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryString)
{
	
	NTSTATUS	Status = STATUS_SUCCESS;
	UNICODE_STRING   uniDeviceName;
	UNICODE_STRING   uniLinkName;
	PDEVICE_OBJECT   DeviceObject = NULL;
	ULONG_PTR        i  = 0;

	HANDLE hThread;

//	CHAR tmp = VmIsActive();

	RtlInitUnicodeString(&uniDeviceName,DEVICE_NAME);
	RtlInitUnicodeString(&uniLinkName,LINK_NAME);

	for (i=0;i<IRP_MJ_MAXIMUM_FUNCTION;i++)
	{
		DriverObject->MajorFunction[i] = DefaultPassThrough;
	}
	DriverObject->DriverUnload = UnloadDriver;

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlPassThrough;

	//创建设备对象
	Status = IoCreateDevice(DriverObject,0,&uniDeviceName,FILE_DEVICE_UNKNOWN,0,FALSE,&DeviceObject);

	if (!NT_SUCCESS(Status))
	{

		return Status;
	}

	Status = IoCreateSymbolicLink(&uniLinkName,&uniDeviceName);

	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(DeviceObject);

		return Status;
	}

	ServiceCallInitialize();

	Status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, VmStart, NULL);

	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
		

	ZwClose(hThread);
	return STATUS_SUCCESS;
}




NTSTATUS DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp)
{
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;

	IoCompleteRequest(Irp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}




NTSTATUS ControlPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp)
{

	return STATUS_SUCCESS;

}




VOID UnloadDriver(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING  uniLinkName;
	PDEVICE_OBJECT  CurrentDeviceObject;
	PDEVICE_OBJECT  NextDeviceObject;

	RtlInitUnicodeString(&uniLinkName,LINK_NAME);

	IoDeleteSymbolicLink(&uniLinkName);

	if (DriverObject->DeviceObject!=NULL)
	{
		CurrentDeviceObject = DriverObject->DeviceObject;

		while(CurrentDeviceObject!=NULL)
		{
			NextDeviceObject  = CurrentDeviceObject->NextDevice;
			IoDeleteDevice(CurrentDeviceObject);

			CurrentDeviceObject = NextDeviceObject;
		}
	}
	DbgPrint("UnloadDriver\r\n");
}