#include "head.h"

NTSTATUS DispatchCreate(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDriverObj);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS DispatchClose(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDriverObj);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS DispatchIoctl(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDriverObj);
	NTSTATUS Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION  IoStackLocation = IoGetCurrentIrpStackLocation(pIrp);
	PVOID InputData = NULL, OutputData = NULL;
	ULONG InputDataLength = 0, OutputDataLength = 0, IoControlCode = 0;
	IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
	InputData = pIrp->AssociatedIrp.SystemBuffer;
	OutputData = pIrp->AssociatedIrp.SystemBuffer;
	InputDataLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
	OutputDataLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

	switch (IoControlCode)
	{
	case IOCTL_IO_PROTECT_PROCESS:
	{
		DbgPrint("data:%d size:%d\n", *(PULONG)InputData, InputDataLength);
		setob_callback(*(PULONG)InputData);

		Status = STATUS_SUCCESS;
		break;
	}
	case IOCTL_IO_OUTPUT_MEMORY:
	{

		*(PULONG)OutputData = get_is_callback();
		OutputDataLength = sizeof(ULONG);

		Status = STATUS_SUCCESS;
		break;
	}
	default:
		Status = STATUS_UNSUCCESSFUL;
		break;
	}
	if (Status == STATUS_SUCCESS)
	{
		pIrp->IoStatus.Information = OutputDataLength;
	}
	else
	{
		pIrp->IoStatus.Information = 0;
	}
		
	pIrp->IoStatus.Status = Status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS driver_unload(PDRIVER_OBJECT pdrive)
{
	delete_obcallback();
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, DEVICE_LINK_NAME);
	IoDeleteSymbolicLink(&symLinkName);
	IoDeleteDevice(pdrive->DeviceObject);
	DbgPrint("Undriver Success...\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pdriver, PUNICODE_STRING path)
{
	DbgPrint("Driver Load..\n");
	
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING link_name = { 0 };
	UNICODE_STRING device_name = { 0 };
	PDEVICE_OBJECT pdevice = NULL;

	pdriver->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	pdriver->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	pdriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchIoctl;
	pdriver->DriverUnload = driver_unload;

	RtlInitUnicodeString(&device_name, DEVICE_NAME);
	status = IoCreateDevice(pdriver, 0, &device_name, FILE_DEVICE_UNKNOWN, 0, FALSE, &pdevice);
	if (!NT_SUCCESS(status)) DbgPrint("IoCreateDevice failed\n");
	RtlInitUnicodeString(&link_name, DEVICE_LINK_NAME);
	status = IoCreateSymbolicLink(&link_name, &device_name);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pdevice);
		DbgPrint("IoCreateSymbolicLink failed\n");
	}

	*(PULONG)((ULONG64)pdriver->DriverSection + 0x68) |= 0x20;
	return status;
}