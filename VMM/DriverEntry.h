#pragma  once

#include "Common.h"


#define DEVICE_NAME L"\\Device\\VTHideDbg"
#define LINK_NAME   L"\\DosDevices\\VTHideDbg"



NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString);

NTSTATUS DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp);
NTSTATUS ControlPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp);
VOID UnloadDriver(IN PDRIVER_OBJECT pDriverObj);