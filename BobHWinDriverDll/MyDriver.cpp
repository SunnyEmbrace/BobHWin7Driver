﻿#include "pch.h"
#include "MyDriver.h"
#include "releaseHelper.h"
#include "resource.h"
#pragma comment (lib,"Advapi32.lib")
#include "iostream"

//加载驱动
BOOL LoadNTDriver(CONST WCHAR* lpszDriverName, CONST WCHAR* lpszDriverPath)
{
	TCHAR szDriverImagePath[256];
	//得到完整的驱动路径
	GetFullPathName(lpszDriverPath, 256, szDriverImagePath, NULL);

	//printf("szDriverImagePath %ls ! \n", szDriverImagePath);

	BOOL bRet = FALSE;

	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄

								 //打开服务控制管理器
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (hServiceMgr == NULL)
	{
		//OpenSCManager失败
		//printf("OpenSCManager() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		////OpenSCManager成功
		//printf("OpenSCManager() ok ! \n");
	}

	//创建驱动所对应的服务
	hServiceDDK = CreateService(hServiceMgr,
		lpszDriverName, //驱动程序的在注册表中的名字  
		lpszDriverName, // 注册表驱动程序的 DisplayName 值  
		SERVICE_ALL_ACCESS, // 加载驱动程序的访问权限  
		SERVICE_KERNEL_DRIVER,// 表示加载的服务是驱动程序  
		SERVICE_DEMAND_START, // 注册表驱动程序的 Start 值  
		SERVICE_ERROR_IGNORE, // 注册表驱动程序的 ErrorControl 值  
		szDriverImagePath, // 注册表驱动程序的 ImagePath 值  
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	DWORD dwRtn;
	//判断服务是否失败
	if (hServiceDDK == NULL)
	{
		dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_EXISTS)
		{
			//由于其他原因创建服务失败
			//printf("CrateService() Faild %d ! \n", dwRtn);
			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			//服务创建失败，是由于服务已经创立过
			//printf("CrateService() Faild Service is ERROR_IO_PENDING or ERROR_SERVICE_EXISTS! \n");
		}
		hServiceDDK = OpenService(hServiceMgr, lpszDriverName, SERVICE_ALL_ACCESS);
		if (hServiceDDK == NULL)
		{
			//如果打开服务也失败，则意味错误
			dwRtn = GetLastError();
			//printf("OpenService() Faild %d ! \n", dwRtn);
			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			//printf("OpenService() ok ! \n");
		}
	}
	else
	{
		//printf("CrateService() ok ! \n");
	}

	//开启此项服务
	bRet = StartService(hServiceDDK, NULL, NULL);
	if (!bRet)
	{
		DWORD dwRtn = GetLastError();
		if (dwRtn != ERROR_IO_PENDING && dwRtn != ERROR_SERVICE_ALREADY_RUNNING)
		{
			//printf("StartService() Faild %d ! \n", dwRtn);
			bRet = FALSE;
			goto BeforeLeave;
		}
		else
		{
			if (dwRtn == ERROR_IO_PENDING)
			{
				//设备被挂住
				//printf("StartService() Faild ERROR_IO_PENDING ! \n");
				bRet = FALSE;
				goto BeforeLeave;
			}
			else
			{
				//服务已经开启
				//printf("StartService() Faild ERROR_SERVICE_ALREADY_RUNNING ! \n");
				bRet = TRUE;
				goto BeforeLeave;

			}
		}
	}
	bRet = TRUE;
	//离开前关闭句柄
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


//卸载驱动程序  
BOOL UnloadNTDriver(CONST WCHAR* szSvrName)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;//SCM管理器的句柄
	SC_HANDLE hServiceDDK = NULL;//NT驱动程序的服务句柄
	SERVICE_STATUS SvrSta;
	//打开SCM管理器
	hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hServiceMgr == NULL)
	{
		//带开SCM管理器失败
		//printf("OpenSCManager() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		//带开SCM管理器失败成功
		//printf("OpenSCManager() ok ! \n");
	}
	//打开驱动所对应的服务
	hServiceDDK = OpenService(hServiceMgr, szSvrName, SERVICE_ALL_ACCESS);

	if (hServiceDDK == NULL)
	{
		//打开驱动所对应的服务失败
		//printf("OpenService() Faild %d ! \n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		//printf("OpenService() ok ! \n");
	}
	//停止驱动程序，如果停止失败，只有重新启动才能，再动态加载。  
	if (!ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta))
	{
		//printf("ControlService() Faild %d !\n", GetLastError());
	}
	else
	{
		//打开驱动所对应的失败
		//printf("ControlService() ok !\n");
	}
	//动态卸载驱动程序。  
	if (!DeleteService(hServiceDDK))
	{
		//卸载失败
		//printf("DeleteSrevice() Faild %d !\n", GetLastError());
	}
	else
	{
		//卸载成功
		//printf("DelServer:eleteSrevice() ok !\n");
	}
	bRet = TRUE;
BeforeLeave:
	//离开前关闭打开的句柄
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

wchar_t* c2w(char* a)
{
	int iSize;
	wchar_t* pwszUnicode;

	//返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
	iSize = MultiByteToWideChar(CP_ACP, 0, a, -1, NULL, 0); //iSize =wcslen(pwsUnicode)+1=6
	pwszUnicode = (wchar_t*)malloc(iSize * sizeof(wchar_t)); //不需要 pwszUnicode = (wchar_t *)malloc((iSize+1)*sizeof(wchar_t))
	MultiByteToWideChar(CP_ACP, 0, a, -1, pwszUnicode, iSize);
	return pwszUnicode;
}


MyDriver::MyDriver()
{
	this->isInint = false;
	this->hdevice = NULL;
}

MyDriver::~MyDriver()
{

	CloseHandle(hdevice);
}

bool MyDriver::UnLoad()
{
	// 打开服务控制管理器数据库
	SC_HANDLE schSCManager = OpenSCManager(
		NULL,                   // 目标计算机的名称,NULL：连接本地计算机上的服务控制管理器
		NULL,                   // 服务控制管理器数据库的名称，NULL：打开 SERVICES_ACTIVE_DATABASE 数据库
		SC_MANAGER_ALL_ACCESS   // 所有权限
	);
	if (schSCManager == NULL) {
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// 打开服务
	SC_HANDLE hs = OpenService(
		schSCManager,           // 服务控件管理器数据库的句柄
		L"BobHWin7Read",            // 要打开的服务名
		SERVICE_ALL_ACCESS      // 服务访问权限：所有权限
	);
	if (hs == NULL) {
		CloseServiceHandle(hs);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// 删除服务
	if (DeleteService(hs) == 0) {
		CloseServiceHandle(hs);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	CloseServiceHandle(hs);
	CloseServiceHandle(schSCManager);
	return TRUE;

}

bool MyDriver::Inint()
{
	
	//安装驱动等代码

	UnloadNTDriver(L"BobHWin7Read");

	CReleaseDLL releasehelper;
	bool blRes;
	blRes = releasehelper.FreeResFile(IDR_SYS1, "SYS", "BobHWin7Driver.sys");

	LoadNTDriver(L"BobHWin7Read", L"BobHWin7Driver.sys");

	char cons[MAX_DLL_PATH + 7] = "\\??\\";

	char strFullPath[MAX_DLL_PATH] = { 0 };
	sprintf_s(strFullPath, "%s\\%s", releasehelper.m_filePath, "BobHWin7Driver.sys");

	strcat_s(cons, strFullPath);


	hdevice = CreateFile(L"\\\\.\\BobHWin7ReadLink", GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdevice == INVALID_HANDLE_VALUE)
	{
		remove(strFullPath);
		
		isInint = false;
		return false;
	}


	DWORD wow;
	DeviceIoControl(hdevice, BOBH_DELETEFILE, cons, sizeof(cons), &cons, sizeof(cons), &wow, NULL);

	
	isInint = true;
	return true;
}

void MyDriver::ForceDeleteFile(const char* path)
{
	char cons[MAX_DLL_PATH + 7] = "\\??\\";

	strcat_s(cons, path);

	DWORD wow;
	DeviceIoControl(hdevice, BOBH_DELETEFILE, cons, sizeof(cons), &cons, sizeof(cons), &wow, NULL);
}

DWORD MyDriver::GetPidByProcessName(const char* name)
{
	DWORD ret = 0, dwrite = 0;

	char Name[1024] = { 0 };

	RtlCopyMemory(Name, name, 1024);

	DeviceIoControl(hdevice, BOBH_GETPROCESSID, (LPVOID)Name, sizeof(Name), &ret, sizeof(ret), &dwrite, NULL);

	return ret;
}

void MyDriver::ProtectProcess(DWORD pid)
{
	DWORD Count = 0;
	DWORD Pid = pid;
	DeviceIoControl(hdevice, BOBH_PROTECT, &Pid, sizeof(Pid), &Pid, sizeof(Pid), &Count, NULL);
}

void MyDriver::StopProtectProcess()
{
	DWORD Pid = 0;
	DeviceIoControl(hdevice, BOBH_UNPROTECT, &Pid, sizeof(Pid), &Pid, sizeof(Pid), &Pid, NULL);
}

ULONG64 MyDriver::GetModuleBaseAddress(DWORD pid, const char* name)
{
	UModuleBase iostruct = { 0 };
	iostruct.Pid = pid;

	RtlCopyMemory(iostruct.ModuleName, name, 1024);

	DWORD Count = 0;
	ULONG64 dllbase = 0;
	DeviceIoControl(hdevice, BOBH_GETMODULEADDRESS, &iostruct, sizeof(iostruct), &dllbase, sizeof(dllbase), &Count, NULL);

	return dllbase;
}

void MyDriver::HideProcess(DWORD pid)
{
	DWORD Count = 0;
	DWORD Pid = pid;
	DeviceIoControl(hdevice, BOBH_HIDEPROCESS, &Pid, sizeof(Pid), &Pid, sizeof(Pid), &Count, NULL);

}
