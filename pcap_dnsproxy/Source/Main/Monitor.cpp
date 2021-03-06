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

extern Configuration Parameter;

//Local DNS server initialization
size_t __stdcall MonitorInitialization()
{
//Get Hop Limits with common DNS request
	if (Parameter.DNSTarget.IPv6)
	{
		std::thread IPv6TestDoaminThread(DomainTest, AF_INET6);
		IPv6TestDoaminThread.detach();
	}
	if (Parameter.DNSTarget.IPv4)
	{
		std::thread IPv4TestDoaminThread(DomainTest, AF_INET);
		IPv4TestDoaminThread.detach();
	}

//Get Hop Limits with ICMP Echo
	if (Parameter.ICMPOptions.ICMPSpeed > 0)
	{
		if (Parameter.DNSTarget.IPv4)
		{
			std::thread ICMPThread(ICMPEcho);
			ICMPThread.detach();
		}

		if (Parameter.DNSTarget.IPv6)
		{
			std::thread ICMPv6Thread(ICMPv6Echo);
			ICMPv6Thread.detach();
		}
	}

	SOCKET_DATA LocalhostData = {0};
//Set localhost socket(IPv6/UDP)
	LocalhostData.Socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	Parameter.LocalSocket[0] = LocalhostData.Socket;
	if (Parameter.ServerMode)
		((PSOCKADDR_IN6)&LocalhostData.SockAddr)->sin6_addr = in6addr_any;
	else 
		((PSOCKADDR_IN6)&LocalhostData.SockAddr)->sin6_addr = in6addr_loopback;
	LocalhostData.SockAddr.ss_family = AF_INET6;
	((PSOCKADDR_IN6)&LocalhostData.SockAddr)->sin6_port = htons(DNS_Port);
	LocalhostData.AddrLen = sizeof(sockaddr_in6);

	std::thread IPv6UDPMonitor(UDPMonitor, LocalhostData);
	memset(&LocalhostData, 0, sizeof(SOCKET_DATA));

//Set localhost socket(IPv6/TCP)
	LocalhostData.Socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	Parameter.LocalSocket[2] = LocalhostData.Socket;
	if (Parameter.ServerMode)
		((PSOCKADDR_IN6)&LocalhostData.SockAddr)->sin6_addr = in6addr_any;
	else 
		((PSOCKADDR_IN6)&LocalhostData.SockAddr)->sin6_addr = in6addr_loopback;
	LocalhostData.SockAddr.ss_family = AF_INET6;
	((PSOCKADDR_IN6)&LocalhostData.SockAddr)->sin6_port = htons(DNS_Port);
	LocalhostData.AddrLen = sizeof(sockaddr_in6);

	std::thread IPv6TCPMonitor(TCPMonitor, LocalhostData);
	memset(&LocalhostData, 0, sizeof(SOCKET_DATA));

//Set localhost socket(IPv4/UDP)
	LocalhostData.Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	Parameter.LocalSocket[1] = LocalhostData.Socket;
	if (Parameter.ServerMode)
		((PSOCKADDR_IN)&LocalhostData.SockAddr)->sin_addr.S_un.S_addr = INADDR_ANY;
	else 
		((PSOCKADDR_IN)&LocalhostData.SockAddr)->sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);
	LocalhostData.SockAddr.ss_family = AF_INET;
	((PSOCKADDR_IN)&LocalhostData.SockAddr)->sin_port = htons(DNS_Port);
	LocalhostData.AddrLen = sizeof(sockaddr_in);

	std::thread IPv4UDPMonitor(UDPMonitor, LocalhostData);
	memset(&LocalhostData, 0, sizeof(SOCKET_DATA));

//Set localhost socket(IPv4/TCP)
	LocalhostData.Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	Parameter.LocalSocket[3] = LocalhostData.Socket;
	if (Parameter.ServerMode)
		((PSOCKADDR_IN)&LocalhostData.SockAddr)->sin_addr.S_un.S_addr = INADDR_ANY;
	else 
		((PSOCKADDR_IN)&LocalhostData.SockAddr)->sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);
	LocalhostData.SockAddr.ss_family = AF_INET;
	((PSOCKADDR_IN)&LocalhostData.SockAddr)->sin_port = htons(DNS_Port);
	LocalhostData.AddrLen = sizeof(sockaddr_in);
			
	std::thread IPv4TCPMonitor(TCPMonitor, LocalhostData);
	memset(&LocalhostData, 0, sizeof(SOCKET_DATA));

/*
//Unblock Mode
	ULONG SocketMode = 1;
	if (ioctlsocket(Parameter.LocalhostSocket ,FIONBIO, &SocketMode) == SOCKET_ERROR)
		return EXIT_FAILURE;

//Preventing other sockets from being forcibly bound to the same address and port.
	int Val = 1;
	if (setsockopt(Parameter.LocalhostSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (PSTR)&Val, sizeof(int)) == SOCKET_ERROR)
		return EXIT_FAILURE;
*/

//Join threads
	if (IPv6UDPMonitor.joinable())
		IPv6UDPMonitor.join();
	if (IPv4UDPMonitor.joinable())
		IPv4UDPMonitor.join();
	if (IPv6TCPMonitor.joinable())
		IPv6TCPMonitor.join();
	if (IPv4TCPMonitor.joinable())
		IPv4TCPMonitor.join();

	WSACleanup();
	return EXIT_SUCCESS;
}

//Local DNS server with UDP protocol
size_t __stdcall UDPMonitor(const SOCKET_DATA LocalhostData)
{
	if (!Parameter.DNSTarget.IPv6 && LocalhostData.AddrLen == sizeof(sockaddr_in6) || //IPv6
		!Parameter.DNSTarget.IPv4 && LocalhostData.AddrLen == sizeof(sockaddr_in)) //IPv4
	{
		closesocket(LocalhostData.Socket);
		return FALSE;
	}

//Socket initialization
	if (LocalhostData.Socket == INVALID_SOCKET)
	{
		PrintError(Winsock_Error, _T("UDP Monitor socket initialization failed"), WSAGetLastError(), NULL);

		closesocket(LocalhostData.Socket);
		return EXIT_FAILURE;
	}

	if (bind(LocalhostData.Socket, (PSOCKADDR)&LocalhostData.SockAddr, LocalhostData.AddrLen) == SOCKET_ERROR)
	{
		PrintError(Winsock_Error, _T("Bind UDP Monitor socket error"), WSAGetLastError(), NULL);

		closesocket(LocalhostData.Socket);
		return EXIT_FAILURE;
	}

//Initialization
	PSTR Buffer = nullptr;
	try {
		Buffer = new char[PACKET_MAXSIZE*THREAD_MAXNUM]();
	}
	catch (std::bad_alloc)
	{
		PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		closesocket(LocalhostData.Socket);
		return EXIT_FAILURE;
	}
	memset(Buffer, 0, PACKET_MAXSIZE*THREAD_MAXNUM);

//Start Monitor
	SSIZE_T RecvLength = 0;
	size_t Index = 0;
	while (true)
	{
		memset(Buffer + PACKET_MAXSIZE*Index, 0, PACKET_MAXSIZE);
		RecvLength = recvfrom(LocalhostData.Socket, Buffer + PACKET_MAXSIZE*Index, PACKET_MAXSIZE, NULL, (PSOCKADDR)&(LocalhostData.SockAddr), (int *)&(LocalhostData.AddrLen));

		if (RecvLength >= (SSIZE_T)(sizeof(dns_hdr) + sizeof(dns_qry)))
		{
			if (LocalhostData.AddrLen == sizeof(sockaddr_in6))
			{
				std::thread RecvProcess(RequestProcess, Buffer + PACKET_MAXSIZE*Index, RecvLength, LocalhostData, IPPROTO_UDP, Index);
				RecvProcess.detach();
			}
			if (LocalhostData.AddrLen == sizeof(sockaddr_in))
			{
				std::thread RecvProcess(RequestProcess, Buffer + PACKET_MAXSIZE*Index, RecvLength, LocalhostData, IPPROTO_UDP, Index + THREAD_MAXNUM);
				RecvProcess.detach();
			}

			Index = (Index + 1)%THREAD_MAXNUM;
		}
	}

	delete[] Buffer;
	closesocket(LocalhostData.Socket);
	return EXIT_SUCCESS;
}

//Local DNS server with TCP protocol
size_t __stdcall TCPMonitor(const SOCKET_DATA LocalhostData)
{
	if (!Parameter.DNSTarget.IPv6 && LocalhostData.AddrLen == sizeof(sockaddr_in6) || //IPv6
		!Parameter.DNSTarget.IPv4 && LocalhostData.AddrLen == sizeof(sockaddr_in)) //IPv4
	{
		closesocket(LocalhostData.Socket);
		return FALSE;
	}

//Socket initialization
	if (LocalhostData.Socket == INVALID_SOCKET)
	{
		PrintError(Winsock_Error, _T("TCP Monitor socket initialization failed"), WSAGetLastError(), NULL);

		closesocket(LocalhostData.Socket);
		return EXIT_FAILURE;
	}

	if(bind(LocalhostData.Socket, (PSOCKADDR)&LocalhostData.SockAddr, LocalhostData.AddrLen) == SOCKET_ERROR)
	{
		PrintError(Winsock_Error, _T("Bind TCP Monitor socket error"), WSAGetLastError(), NULL);

		closesocket(LocalhostData.Socket);
		return EXIT_FAILURE;
	}

	if (listen(LocalhostData.Socket, SOMAXCONN) == SOCKET_ERROR)
	{
		PrintError(Winsock_Error, _T("TCP Monitor socket listening initialization failed"), WSAGetLastError(), NULL);

		closesocket(LocalhostData.Socket);
		return EXIT_FAILURE;
	}

//Start Monitor
	SOCKET_DATA ClientData = {0};
	size_t Index = 0;
	while (true)
	{
		memset(&ClientData, 0, sizeof(SOCKET_DATA));
		ClientData.Socket = accept(LocalhostData.Socket, (PSOCKADDR)&ClientData.SockAddr, (PINT)&(LocalhostData.AddrLen));
		if (ClientData.Socket == INVALID_SOCKET)
		{
			closesocket(ClientData.Socket);
			continue;
		}

		ClientData.AddrLen = LocalhostData.AddrLen;
		std::thread TCPReceiveThread(TCPReceiveProcess, ClientData, Index);
		TCPReceiveThread.detach();

		Index = (Index + 1)%THREAD_MAXNUM;
	}

	closesocket(LocalhostData.Socket);
	return EXIT_SUCCESS;
}
