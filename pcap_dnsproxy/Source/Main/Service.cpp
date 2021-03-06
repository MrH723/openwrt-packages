// This code is part of Pcap_DNSProxy
// Copyright (C) 2012-2014 Chengr28
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "Pcap_DNSProxy.h"

std::wstring Path, ErrorLogPath;
static DWORD ServiceCurrentStatus = 0;
static BOOL bServiceRunning = false;
SERVICE_STATUS_HANDLE hServiceStatus = nullptr; 
HANDLE hServiceEvent = nullptr;

extern Configuration Parameter;

//Get infomation of service
size_t __stdcall GetServiceInfo()
{
//Get service information
	SC_HANDLE SCM = nullptr, Service = nullptr;
	DWORD nResumeHandle = 0;

	if((SCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) == nullptr)
		return EXIT_FAILURE;
 
	Service = OpenService(SCM, LOCALSERVERNAME, SERVICE_ALL_ACCESS);
	if(Service == nullptr)
		return EXIT_FAILURE;

	LPQUERY_SERVICE_CONFIG ServicesInfo = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LPTR, PACKET_MAXSIZE*4); //Max buffer of QueryServiceConfig() is 8KB/8192 Bytes.
	if (ServicesInfo == nullptr)
		return EXIT_FAILURE;

	if (QueryServiceConfig(Service, ServicesInfo, PACKET_MAXSIZE*4, &nResumeHandle) == FALSE)
	{
		LocalFree(ServicesInfo);
		return EXIT_FAILURE;
	}

	Path = ServicesInfo->lpBinaryPathName;
	LocalFree(ServicesInfo);

//Path process
	size_t Index = Path.rfind(_T("\\"));
	static const WCHAR wBackslash[] = _T("\\");
	Path.erase(Index + 1, Path.length());

	for (Index = 0;Index < Path.length();Index++)
	{
		if (Path[Index] == _T('\\'))
		{
			Path.insert(Index, wBackslash);
			Index++;
		}
	}

//Get path of error log file and delete the old one
	ErrorLogPath.append(Path);
	ErrorLogPath.append(_T("Error.log"));
	DeleteFile(ErrorLogPath.c_str());
	Parameter.PrintError = true;

//Winsock initialization
	WSADATA WSAData = {0};
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0 || LOBYTE(WSAData.wVersion) != 2 || HIBYTE(WSAData.wVersion) != 2)
	{
		PrintError(Winsock_Error, _T("Winsock initialization failed"), WSAGetLastError(), NULL);

		WSACleanup();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

//Service Main function
size_t WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	hServiceStatus = RegisterServiceCtrlHandler(LOCALSERVERNAME, (LPHANDLER_FUNCTION)ServiceControl);
	if(!hServiceStatus || !UpdateServiceStatus(SERVICE_START_PENDING, NO_ERROR, 0, 1, TIME_OUT*3))
		return FALSE;

	hServiceEvent = CreateEvent(0, TRUE, FALSE, 0);
	if(!hServiceEvent || !UpdateServiceStatus(SERVICE_START_PENDING, NO_ERROR, 0, 2, TIME_OUT) || !ExecuteService())
		return FALSE;

	ServiceCurrentStatus = SERVICE_RUNNING;
	if(!UpdateServiceStatus(SERVICE_RUNNING, NO_ERROR, 0, 0, 0))
		return FALSE;

	WaitForSingleObject(hServiceEvent, INFINITE);
	CloseHandle(hServiceEvent);
	return EXIT_SUCCESS;
}

//Service controller
size_t WINAPI ServiceControl(const DWORD dwControlCode)
{
	switch(dwControlCode)
	{
		case SERVICE_CONTROL_SHUTDOWN:
		{
			WSACleanup();
			TerminateService();
			return EXIT_SUCCESS;
		}
		case SERVICE_CONTROL_STOP:
		{
			ServiceCurrentStatus = SERVICE_STOP_PENDING;
			UpdateServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0, 1, TIME_OUT*3);
			WSACleanup();
			TerminateService();
			return EXIT_SUCCESS;
		}
		default:
		{
			break;
		}
	}

	UpdateServiceStatus(ServiceCurrentStatus, NO_ERROR, 0, 0, 0);
	return 0;
}

//Start Main process
BOOL __stdcall ExecuteService()
{
	DWORD dwThreadID = 0;
	HANDLE hServiceThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ServiceProc, NULL, 0, &dwThreadID);

	if(hServiceThread != nullptr)
	{
		bServiceRunning = TRUE;
		return TRUE;
	}

	return FALSE;
}

//Service Main process thread
DWORD WINAPI ServiceProc(LPVOID lpParameter)
{
	if (!bServiceRunning || MonitorInitialization() == EXIT_FAILURE)
	{
		WSACleanup();
		TerminateService();
		return FALSE;
	}

	WSACleanup();
	TerminateService();
	return EXIT_SUCCESS;
}

//Change status of service
BOOL __stdcall UpdateServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwServiceSpecificExitCode, DWORD dwCheckPoint, DWORD dwWaitHint)
{
	SERVICE_STATUS ServiceStatus = {0};
	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwCurrentState = dwCurrentState;

	if (dwCurrentState == SERVICE_START_PENDING)
		ServiceStatus.dwControlsAccepted = 0;
	else
		ServiceStatus.dwControlsAccepted = (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);

	if (dwServiceSpecificExitCode == 0)
		ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	else
		ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;

	ServiceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
	ServiceStatus.dwCheckPoint = dwCheckPoint;
	ServiceStatus.dwWaitHint = dwWaitHint;

	if(!SetServiceStatus(hServiceStatus, &ServiceStatus))
	{
		WSACleanup();
		TerminateService();
		return FALSE;
 	}

	return TRUE;
}

//Terminate service
void __stdcall TerminateService()
{
	bServiceRunning = FALSE;
	SetEvent(hServiceEvent);
	UpdateServiceStatus(SERVICE_STOPPED, NO_ERROR, 0, 0, 0);
	return;
}
