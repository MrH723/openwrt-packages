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

Configuration Parameter;

extern std::wstring Path;

//The Main function of program
int main(int argc, char *argv[])
{
/*
//Handle the system signal
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
*/

//Get Path and Winsock initialization
	if (GetServiceInfo() == EXIT_FAILURE)
		return EXIT_FAILURE;

//Read configuration file and WinPcap initialization
	if (Parameter.ReadParameter() == EXIT_FAILURE || CaptureInitialization() == EXIT_FAILURE)
	{
		WSACleanup();
		return EXIT_FAILURE;
	}

//Get Localhost DNS PTR Records
	std::thread IPv6LocalAddressThread(LocalAddressToPTR, AF_INET6);
	std::thread IPv4LocalAddressThread(LocalAddressToPTR, AF_INET);
	IPv6LocalAddressThread.detach();
	IPv4LocalAddressThread.detach();

//Read Hosts
	if (Parameter.Hosts > 0)
	{
		std::thread HostsThread(&Configuration::ReadHosts, std::ref(Parameter));
		HostsThread.detach();
	}

//Windows Firewall Test
	if (FirewallTest() == EXIT_FAILURE)
	{
		PrintError(Winsock_Error, _T("Windows Firewall Test failed"), NULL, NULL);

		WSACleanup();
		return EXIT_FAILURE;
	}

//Service initialization and start service
	SERVICE_TABLE_ENTRY ServiceTable[] = 
	{
		{LOCALSERVERNAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};

	if (!StartServiceCtrlDispatcher(ServiceTable))
	{
		PrintError(System_Error, _T("Service start failed(It's probably a Firewall Test, please restart service and check once again)"), GetLastError(), NULL);

		WSACleanup();
		return EXIT_FAILURE;
	}

	WSACleanup();
	return EXIT_SUCCESS;
}

//Windows Firewall Test
inline size_t __stdcall FirewallTest()
{
	SYSTEM_SOCKET FirewallSocket = 0;
	sockaddr_storage SockAddr = {0};

//Socket initialization
	srand((UINT)time((time_t *)NULL));
	if (Parameter.DNSTarget.IPv6) //IPv6
	{
		FirewallSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		SockAddr.ss_family = AF_INET6;
		((PSOCKADDR_IN6)&SockAddr)->sin6_addr = in6addr_any;
		((PSOCKADDR_IN6)&SockAddr)->sin6_port = htons((USHORT)(rand() % 65535 + 1));

	//Bind local socket
		if (FirewallSocket == INVALID_SOCKET || bind(FirewallSocket, (PSOCKADDR)&SockAddr, sizeof(sockaddr_in6)) == SOCKET_ERROR)
		{
			closesocket(FirewallSocket);
			return EXIT_FAILURE;
		}
	}
	else { //IPv4
		FirewallSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		SockAddr.ss_family = AF_INET;
		((PSOCKADDR_IN)&SockAddr)->sin_addr.S_un.S_addr = INADDR_ANY;
		((PSOCKADDR_IN)&SockAddr)->sin_port = htons((USHORT)(rand() % 65535 + 1)); //Maximum of USHORT

	//Bind local socket
		if (FirewallSocket == INVALID_SOCKET || bind(FirewallSocket, (PSOCKADDR)&SockAddr, sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			closesocket(FirewallSocket);
			return EXIT_FAILURE;
		}
	}

	closesocket(FirewallSocket);
	return EXIT_SUCCESS;
}
