#include <stdio.h>
#include <windows.h>  
#include "sys.h"
#include <TlHelp32.h>
#define IOCTL_IO_PROTECT_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IO_OUTPUT_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

BOOL LoadNTDriver(char* name, char* path)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;
	SC_HANDLE hServiceDDK = NULL;
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		bRet = FALSE;
		goto BeforeLeave;
	}
	hServiceDDK = CreateServiceA(
		hServiceMgr,
		name,
		name,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		path,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);
	DWORD dwRtn;
	if (hServiceDDK == NULL)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS)
		{
			bRet = FALSE;
			goto BeforeLeave;
		}

		hServiceDDK = OpenServiceA(hServiceMgr, name, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			dwRtn = GetLastError();
			bRet = FALSE;
			goto BeforeLeave;
		}
	}
	bRet = StartService(hServiceDDK, NULL, NULL);
	if (!bRet)
	{
		DWORD dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_ALREADY_RUNNING)
		{
			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			if (dwRtn == ERROR_IO_PENDING)//设备被挂住
			{
				bRet = FALSE;
				goto BeforeLeave;
			}
			else//服务已经开启
			{
				bRet = TRUE;
				goto BeforeLeave;
			}
		}
	}
	bRet = TRUE;
BeforeLeave:
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return bRet;
}

BOOL UnloadNTDriver(char* szSvrName)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;
	SC_HANDLE hServiceDDK = NULL;
	SERVICE_STATUS SvrSta;
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		bRet = FALSE;
		goto BeforeLeave;
	}
	hServiceDDK = OpenServiceA(hServiceMgr, szSvrName, SERVICE_ALL_ACCESS);
	if (hServiceDDK == NULL)
	{
		bRet = FALSE;
		goto BeforeLeave;
	}
	ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta);
	
	bRet = TRUE;
BeforeLeave:
	if (hServiceDDK)CloseServiceHandle(hServiceDDK);
	if (hServiceMgr)CloseServiceHandle(hServiceMgr);
	return bRet;
}

void add_exe(WCHAR* string, int size)
{
	WCHAR buf[] = L".exe";
	if (!wcsstr(string, buf))wcscat_s(string, size, buf);
}

void exit_(int a)
{
	printf("==>error code:%d\n", a);
	system("pause");
	exit(0);
}
DWORD getpid_byname(WCHAR* name)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 entry = { 0 };
	entry.dwSize = sizeof(PROCESSENTRY32);
	Process32First(snap, &entry);
	do
	{
		if (wcscmp(entry.szExeFile, name) == 0)
		{
			CloseHandle(snap);
			return entry.th32ProcessID;
		}
	} while (Process32Next(snap, &entry));
	CloseHandle(snap);
	return 0;
}
void main()
{
	char path[500] = { 0 };
	char pdriver_name[] = "\\Protect_Process_OB.sys";
	GetCurrentDirectoryA(sizeof(path), path);
	strcat_s(path, sizeof(path), pdriver_name);
	
	HANDLE hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, CREATE_ALWAYS, FILE_SHARE_READ, NULL);
	WriteFile(hFile, _code1, sizeof(_code1), 0, NULL);
	CloseHandle(hFile);

	WCHAR process_name[256] = { 0 };
	ULONG pid = 0;
	printf("请输入要保护的进程名，回车确定...（区分大小写哦~）\n");
	
	do
	{
		scanf_s("%ws", process_name);
		add_exe(process_name, sizeof(process_name));
		pid = (HANDLE)getpid_byname(process_name);
		if (!pid)printf("该进程不存在，请重新输入...\n");
	} while (pid == 0);

	BOOL ret = LoadNTDriver("Protect_Process_OB", path);
	if (!ret)exit_(1);

	OVERLAPPED overlapped = { 0 };
	ULONG outbuf = 0;
	HANDLE hDevice = CreateFileA("\\\\.\\lyx_Protect", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
	if (hDevice == INVALID_HANDLE_VALUE)exit_(2);
	else
	{
		ret = DeviceIoControl(hDevice, IOCTL_IO_PROTECT_PROCESS, &pid, sizeof(ULONG), 0, 0, 0, &overlapped);
		if (ret)
		{
			GetAsyncKeyState(VK_F12);
			Sleep(1000);
			printf("protection is on\n");
			printf("按下F12结束保护（同时会卸载驱动）\n\n");
			while (TRUE)
			{
				DeviceIoControl(hDevice, IOCTL_IO_OUTPUT_MEMORY, 0, 0, &outbuf, sizeof(outbuf), 0, &overlapped);
				if (outbuf !=0)
				{
					printf("==>拦截到一次 - code:%d\n", outbuf);
					outbuf = 0;
				}
				if (GetAsyncKeyState(VK_F12) != 0)break;
				Sleep(100);
			}
		}
		else exit_(3);
	}
	CloseHandle(hDevice);
	UnloadNTDriver("Protect_Process_OB");
	printf("==>protection is off\n");
	system("pause");
}