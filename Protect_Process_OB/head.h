#pragma once
#include <ntifs.h>

NTKERNELAPI UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process);
#define DEVICE_NAME				L"\\Device\\lyx_Protect"
#define DEVICE_LINK_NAME	L"\\DosDevices\\lyx_Protect"
#define IOCTL_IO_PROTECT_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IO_OUTPUT_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

VOID delete_obcallback();
VOID setob_callback(ULONG);
ULONG get_is_callback();