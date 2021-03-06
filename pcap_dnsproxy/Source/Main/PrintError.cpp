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

extern std::wstring ErrorLogPath;
extern Configuration Parameter;

//Print errors to log file
size_t __stdcall PrintError(const size_t Type, const PWSTR Message, const SSIZE_T Code, const size_t Line)
{
//Print Error(s): ON/OFF
	if (!Parameter.PrintError)
		return FALSE;

//Read from file
	FILE *Output = nullptr;
	_wfopen_s(&Output, ErrorLogPath.c_str(), _T("a"));

//Get current date&time
	tm Time = {0};
	time_t tTime = 0;
	time(&tTime);
	localtime_s(&Time, &tTime);
/*
//Windows API
	SYSTEMTIME Time = {0};
	GetLocalTime(&Time);
	fwprintf_s(Output, _T("%u%u/%u %u:%u:%u -> %s.\n"), Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond, pBuffer); //Windows API

//Convert to ASCII
	char TimeBuf[PACKET_MAXSIZE] = {0};
	asctime_s(TimeBuf, &Time);
	fwprintf_s(Output, _T("%s -> %s.\n"), TimeBuf, pBuffer);
*/

	if (Output != nullptr)
	{
	// Error Type
	// 01: System Error
	// 02: Parameter Error
	// 03: Hosts Error
	// 04: Winsock Error
	// 05: WinPcap Error
		switch (Type)
		{
		//System Error
		//About Service Error code, see http://msdn.microsoft.com/en-us/library/windows/desktop/ms686324(v=vs.85).aspx
			case System_Error:
			{
				if (Code == NULL)
					fwprintf_s(Output, _T("%d-%02d-%02d %02d:%02d:%02d -> System Error: %ls.\n"), Time.tm_year + 1900, Time.tm_mon + 1, Time.tm_mday, Time.tm_hour, Time.tm_min, Time.tm_sec, Message);
				else 
					fwprintf_s(Output, _T("%d-%02d-%02d %02d:%02d:%02d -> System Error: %ls, Error code is %d\n"), Time.tm_year + 1900, Time.tm_mon + 1, Time.tm_mday, Time.tm_hour, Time.tm_min, Time.tm_sec, Message, (int)Code);
			}break;
		//Parameter Error
			case Parameter_Error:
			{
				fwprintf_s(Output, _T("%d-%02d-%02d %02d:%02d:%02d -> Parameter Error: %ls"), Time.tm_year + 1900, Time.tm_mon + 1, Time.tm_mday, Time.tm_hour, Time.tm_min, Time.tm_sec, Message);
				if (Line != NULL)
					fwprintf_s(Output, _T(" in Line %d"), (int)Line);
				if (Code != NULL)
					fwprintf_s(Output, _T(", Error Code is %d"), (int)Code);

				fwprintf_s(Output, _T(".\n"));
			}break;
		//Hosts Error
			case Hosts_Error:
			{
				fwprintf_s(Output, _T("%d-%02d-%02d %02d:%02d:%02d -> Hosts Error: %ls"), Time.tm_year + 1900, Time.tm_mon + 1, Time.tm_mday, Time.tm_hour, Time.tm_min, Time.tm_sec, Message);
				if (Line != NULL)
					fwprintf_s(Output, _T(" in Line %d"), (int)Line);
				if (Code != NULL)
					fwprintf_s(Output, _T(", Error Code is %d"), (int)Code);

				fwprintf_s(Output, _T(".\n"));
			}break;
		//Winsock Error
		//About Winsock Error code, see http://msdn.microsoft.com/en-us/library/windows/desktop/ms740668(v=vs.85).aspx
			case Winsock_Error:
			{
				if (Code == NULL)
					fwprintf_s(Output, _T("%d-%02d-%02d %02d:%02d:%02d -> Winsock Error: %ls.\n"), Time.tm_year + 1900, Time.tm_mon + 1, Time.tm_mday, Time.tm_hour, Time.tm_min, Time.tm_sec, Message);
				else 
					fwprintf_s(Output, _T("%d-%02d-%02d %02d:%02d:%02d -> Winsock Error: %ls, Error code is %d\n"), Time.tm_year + 1900, Time.tm_mon + 1, Time.tm_mday, Time.tm_hour, Time.tm_min, Time.tm_sec, Message, (int)Code);
			}break;
		//WinPcap Error
			case WinPcap_Error:
				fwprintf_s(Output, _T("%d-%02d-%02d %02d:%02d:%02d -> WinPcap Error: %ls.\n"), Time.tm_year + 1900, Time.tm_mon + 1, Time.tm_mday, Time.tm_hour, Time.tm_min, Time.tm_sec, Message);
				break;
			default:
				fclose(Output);
				return EXIT_FAILURE;
		}

		fclose(Output);
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}
