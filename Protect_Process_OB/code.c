#include "head.h"

PVOID RegistrationHandle = NULL;
ULONG protect_pid = 0;
ULONG is_callback = 0;

ULONG get_is_callback()
{
	ULONG ret = is_callback;
	is_callback = 0;
	return ret;
}
VOID delete_obcallback()
{
	if (RegistrationHandle != NULL)ObUnRegisterCallbacks(RegistrationHandle);
}

OB_PREOP_CALLBACK_STATUS PobPreOperationCallback(PVOID RegistrationContext,
	POB_PRE_OPERATION_INFORMATION OperationInformation)
{
	ULONG pid = (ULONG)PsGetProcessId((PEPROCESS)OperationInformation->Object);

	if (protect_pid == pid && OperationInformation->ObjectType == *PsProcessType)
	{
		if (OperationInformation->Operation == OB_OPERATION_HANDLE_CREATE)
		{
			if (OperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & 0x1 == 0x1)
			{
				OperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~0x1;
				is_callback = 1;
			}
		}
	}
	return OB_PREOP_SUCCESS;
}

VOID setob_callback(ULONG pid)
{
	protect_pid = pid;

	NTSTATUS status;
	OB_CALLBACK_REGISTRATION obreg = { 0 };
	OB_OPERATION_REGISTRATION opreg = { 0 };
	obreg.Version = OB_FLT_REGISTRATION_VERSION;
	obreg.OperationRegistrationCount = 1;
	obreg.RegistrationContext = NULL;
	RtlInitUnicodeString(&obreg.Altitude, L"422000");

	opreg.ObjectType = PsProcessType;
	opreg.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
	opreg.PreOperation = PobPreOperationCallback;
	obreg.OperationRegistration = &opreg;
	status = ObRegisterCallbacks(&obreg, &RegistrationHandle);
}