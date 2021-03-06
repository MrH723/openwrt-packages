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

#define Interval          5000        //5000ms or 5s between every sending
#define OnceSend          3

extern Configuration Parameter;
extern PortTable PortList;

//Get TTL(IPv4)/Hop Limits(IPv6) with common DNS request
size_t __stdcall DomainTest(const size_t Protocol)
{
//Initialization
	PSTR Buffer = nullptr, DNSQuery = nullptr;
	try {
		Buffer = new char[PACKET_MAXSIZE]();
		DNSQuery = new char[PACKET_MAXSIZE/4]();
	}
	catch (std::bad_alloc)
	{
		PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		delete[] Buffer;
		delete[] DNSQuery;
		WSACleanup();
		TerminateService();
		return EXIT_FAILURE;
	}
	memset(Buffer, 0, PACKET_MAXSIZE);
	memset(DNSQuery, 0, PACKET_MAXSIZE/4);
	SOCKET_DATA SetProtocol = {0};

//Set request protocol
	if (Protocol == AF_INET6) //IPv6
		SetProtocol.AddrLen = sizeof(sockaddr_in6);
	else //IPv4
		SetProtocol.AddrLen = sizeof(sockaddr_in);

//Make a DNS request with Doamin Test packet
	dns_hdr *TestHdr = (dns_hdr *)Buffer;
	TestHdr->ID = Parameter.DomainTestOptions.DomainTestID;
	TestHdr->Flags = htons(0x0100); //System Standard query
	TestHdr->Questions = htons(0x0001);
	size_t TestLength =  0;

//From Parameter
	if (Parameter.DomainTestOptions.DomainTestCheck)
	{
		TestLength = CharToDNSQuery(Parameter.DomainTestOptions.DomainTest, DNSQuery);
		if (TestLength > 0 && TestLength < PACKET_MAXSIZE - sizeof(dns_hdr))
		{
			memcpy(Buffer + sizeof(dns_hdr), DNSQuery, TestLength);
			dns_qry *TestQry = (dns_qry *)(Buffer + sizeof(dns_hdr) + TestLength);
			TestQry->Classes = htons(Class_IN);
			if (Protocol == AF_INET6)
				TestQry->Type = htons(AAAA_Records);
			else 
				TestQry->Type = htons(A_Records);
			delete[] DNSQuery;
		}
		else {
			delete[] Buffer;
			delete[] DNSQuery;
			return EXIT_FAILURE;
		}
	}

//Send
	size_t Times = 0;
	while (true)
	{
		if (Times == OnceSend)
		{
			Times = 0;
			if (Parameter.DNSTarget.IPv4 && Parameter.HopLimitOptions.IPv4TTL == 0 || //IPv4
				Parameter.DNSTarget.IPv6 && Parameter.HopLimitOptions.IPv6HopLimit == 0) //IPv6
			{
				Sleep(Interval); //5 seconds between every sending.
				continue;
			}

			Sleep((DWORD)Parameter.DomainTestOptions.DomainTestSpeed);
		}
		else {
		//Ramdom domain request
			if (!Parameter.DomainTestOptions.DomainTestCheck)
			{
				memset(Parameter.DomainTestOptions.DomainTest, 0,  PACKET_MAXSIZE/8);
				RamdomDomain(Parameter.DomainTestOptions.DomainTest, PACKET_MAXSIZE/8);
				TestLength = CharToDNSQuery(Parameter.DomainTestOptions.DomainTest, DNSQuery);
				memcpy(Buffer + sizeof(dns_hdr), DNSQuery, TestLength);
				
				dns_qry *TestQry = (dns_qry *)(Buffer + sizeof(dns_hdr) + TestLength);
				TestQry->Classes = htons(Class_IN);
				if (Protocol == AF_INET6)
					TestQry->Type = htons(AAAA_Records);
				else 
					TestQry->Type = htons(A_Records);
			}

			UDPRequest(Buffer, TestLength + sizeof(dns_hdr) + 4, SetProtocol, THREAD_MAXNUM*THREAD_PARTNUM, false);
			Sleep(Interval);
			Times++;
		}
	}

	delete[] Buffer;
	delete[] DNSQuery;
	return EXIT_SUCCESS;
}

//Internet Control Message Protocol/ICMP Echo(Ping) request
size_t __stdcall ICMPEcho()
{
//Initialization
	PSTR Buffer = nullptr;
	try {
		Buffer = new char[PACKET_MAXSIZE]();
	}
	catch (std::bad_alloc)
	{
		PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		WSACleanup();
		TerminateService();
		return EXIT_FAILURE;
	}
	memset(Buffer, 0, PACKET_MAXSIZE);
	sockaddr_storage SockAddr = {0};
	SYSTEM_SOCKET Request = 0;

//Make a ICMP request echo packet
	icmp_hdr *icmp = (icmp_hdr *)Buffer;
	icmp->Type = 8; //Echo(Ping) request type
	icmp->ID = htons(Parameter.ICMPOptions.ICMPID);
	icmp->Sequence = htons(Parameter.ICMPOptions.ICMPSequence);
	memcpy(Buffer + sizeof(icmp_hdr), Parameter.PaddingDataOptions.PaddingData, Parameter.PaddingDataOptions.PaddingDataLength - 1);
	icmp->Checksum = GetChecksum((PUSHORT)Buffer, sizeof(icmp_hdr) + Parameter.PaddingDataOptions.PaddingDataLength - 1);

	Request = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	SockAddr.ss_family = AF_INET;
	((PSOCKADDR_IN)&SockAddr)->sin_addr = Parameter.DNSTarget.IPv4Target;

//Check socket
	if (Request == INVALID_SOCKET)
	{
		PrintError(Winsock_Error, _T("ICMP Echo(Ping) request error"), WSAGetLastError(), NULL);

		delete[] Buffer;
		closesocket(Request);
		return EXIT_FAILURE;
	}

//Send
	size_t Times = 0;
	while (true)
	{
		sendto(Request, Buffer, (int)(sizeof(icmp_hdr) + Parameter.PaddingDataOptions.PaddingDataLength - 1), NULL, (PSOCKADDR)&SockAddr, sizeof(sockaddr_in));

		if (Times == OnceSend)
		{
			Times = 0;
			if (Parameter.HopLimitOptions.IPv4TTL == 0)
			{
				Sleep(Interval); //5 seconds between every sending.
				continue;
			}

			Sleep((DWORD)Parameter.ICMPOptions.ICMPSpeed);
		}
		else {
			Sleep(Interval);
			Times++;
		}
	}

	delete[] Buffer;
	closesocket(Request);
	return EXIT_SUCCESS;
}

//Internet Control Message Protocol Echo version 6/ICMPv6 Echo(Ping) request
size_t __stdcall ICMPv6Echo()
{
//Initialization
	PSTR Buffer = nullptr, ICMPv6Checksum = nullptr;
	try {
		Buffer = new char[PACKET_MAXSIZE]();
		ICMPv6Checksum = new char[PACKET_MAXSIZE]();
	}
	catch (std::bad_alloc)
	{
		PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		delete[] Buffer;
		delete[] ICMPv6Checksum;
		WSACleanup();
		TerminateService();
		return EXIT_FAILURE;
	}
	memset(Buffer, 0, PACKET_MAXSIZE);
	memset(ICMPv6Checksum, 0, PACKET_MAXSIZE);
	sockaddr_storage SockAddr = {0};
	SYSTEM_SOCKET Request = 0;
	memset(&SockAddr, 0, sizeof(sockaddr_storage));

//Make a IPv6 ICMPv6 request echo packet
	icmpv6_hdr *icmpv6 = (icmpv6_hdr *)Buffer;
	icmpv6->Type = ICMPV6_REQUEST;
	icmpv6->Code = 0;
	icmpv6->ID = htons(Parameter.ICMPOptions.ICMPID);
	icmpv6->Sequence = htons(Parameter.ICMPOptions.ICMPSequence);

//Validate local IPv6 address
	ipv6_psd_hdr *psd = (ipv6_psd_hdr *)ICMPv6Checksum;
	psd->Dst = Parameter.DNSTarget.IPv6Target;
	if (!GetLocalAddress(SockAddr, AF_INET6))
	{
		PrintError(Winsock_Error, _T("Get localhost address(es) failed"), NULL, NULL);

		delete[] Buffer;
		delete[] ICMPv6Checksum;
		return EXIT_FAILURE;
	}
//End

	psd->Src = ((PSOCKADDR_IN6)&SockAddr)->sin6_addr;
	memset(&SockAddr, 0, sizeof(sockaddr_storage));
	psd->Length = htonl((ULONG)(sizeof(icmpv6_hdr) + Parameter.PaddingDataOptions.PaddingDataLength - 1));
	psd->Next_Header = IPPROTO_ICMPV6;

	memcpy(ICMPv6Checksum + sizeof(ipv6_psd_hdr), icmpv6, sizeof(icmpv6_hdr));
	memcpy(ICMPv6Checksum + sizeof(ipv6_psd_hdr) + sizeof(icmpv6_hdr), &Parameter.PaddingDataOptions.PaddingData, Parameter.PaddingDataOptions.PaddingDataLength - 1);
	icmpv6->Checksum = htons(GetChecksum((PUSHORT)ICMPv6Checksum, sizeof(ipv6_psd_hdr) + sizeof(icmpv6_hdr) + Parameter.PaddingDataOptions.PaddingDataLength - 1));
	delete[] ICMPv6Checksum;

	Request = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	SockAddr.ss_family = AF_INET6;
	((PSOCKADDR_IN6)&SockAddr)->sin6_addr = Parameter.DNSTarget.IPv6Target;

//Check socket
	if (Request == INVALID_SOCKET)
	{
		PrintError(Winsock_Error, _T("ICMPv6 Echo(Ping) request error"), WSAGetLastError(), NULL);

		delete[] Buffer;
		closesocket(Request);
		return EXIT_FAILURE;
	}

//Send
	size_t Times = 0;
	while (true)
	{
		sendto(Request, Buffer, (int)(sizeof(icmpv6_hdr) + Parameter.PaddingDataOptions.PaddingDataLength - 1), NULL, (PSOCKADDR)&SockAddr, sizeof(sockaddr_in6));

		if (Times == OnceSend)
		{
			Times = 0;
			if (Parameter.HopLimitOptions.IPv6HopLimit == 0)
			{
				Sleep(Interval);
				continue;
			}

			Sleep((DWORD)Parameter.ICMPOptions.ICMPSpeed);
		}
		else {
			Times++;
			Sleep(Interval);
		}
	}

	delete[] Buffer;
	closesocket(Request);
	return EXIT_SUCCESS;
}

//Transmission and reception of TCP protocol(Independent)
size_t __stdcall TCPRequest(const PSTR Send, const size_t SendSize, PSTR Recv, const size_t RecvSize, const SOCKET_DATA TargetData, const bool Local)
{
//Initialization
	PSTR SendBuffer = nullptr, RecvBuffer = nullptr;
	try {
		SendBuffer = new char[PACKET_MAXSIZE]();
		RecvBuffer = new char[PACKET_MAXSIZE]();
	}
	catch (std::bad_alloc)
	{
		PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		delete[] SendBuffer;
		delete[] RecvBuffer;
		WSACleanup();
		TerminateService();
		return EXIT_FAILURE;
	}
	memset(SendBuffer, 0, PACKET_MAXSIZE);
	memset(RecvBuffer, 0, PACKET_MAXSIZE);
	sockaddr_storage SockAddr = {0};
	SYSTEM_SOCKET TCPSocket = 0;
	memcpy(RecvBuffer, Send, SendSize);

//Add length of request packet(It must be written in header when transpot with TCP protocol)
	USHORT DataLength = htons((USHORT)SendSize);
	memcpy(SendBuffer, &DataLength, sizeof(USHORT));
	memcpy(SendBuffer + sizeof(USHORT), RecvBuffer, SendSize);
	memset(RecvBuffer, 0, PACKET_MAXSIZE);

//Socket initialization
	if (TargetData.AddrLen == sizeof(sockaddr_in6)) //IPv6
	{
		TCPSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		if (Local && Parameter.DNSTarget.Local_IPv6)
			((PSOCKADDR_IN6)&SockAddr)->sin6_addr = Parameter.DNSTarget.Local_IPv6Target;
		else 
			((PSOCKADDR_IN6)&SockAddr)->sin6_addr = Parameter.DNSTarget.IPv6Target;
		SockAddr.ss_family = AF_INET6;
		((PSOCKADDR_IN6)&SockAddr)->sin6_port = htons(DNS_Port);
	}
	else { //IPv4
		TCPSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (Local && Parameter.DNSTarget.Local_IPv4)
			((PSOCKADDR_IN)&SockAddr)->sin_addr = Parameter.DNSTarget.Local_IPv4Target;
		else 
			((PSOCKADDR_IN)&SockAddr)->sin_addr = Parameter.DNSTarget.IPv4Target;
		SockAddr.ss_family = AF_INET;
		((PSOCKADDR_IN)&SockAddr)->sin_port = htons(DNS_Port);
	}

/*
//TCP KeepAlive Mode
	BOOL bKeepAlive = TRUE;
	if (setsockopt(TCPSocket, SOL_SOCKET, SO_KEEPALIVE, (PSTR)&bKeepAlive, sizeof(bKeepAlive)) == SOCKET_ERROR)
	{
		delete[] SendBuffer;
		delete[] RecvBuffer;
		closesocket(TCPSocket);
		return EXIT_FAILURE;
	}

	tcp_keepalive alive_in = {0};
	tcp_keepalive alive_out = {0};
	alive_in.keepalivetime = TIME_OUT;
	alive_in.keepaliveinterval = TIME_OUT*2;
	alive_in.onoff = TRUE;
	ULONG ulBytesReturn = 0;
	if (WSAIoctl(TCPSocket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL) == SOCKET_ERROR)
	{
		delete[] SendBuffer;
		delete[] RecvBuffer;
		closesocket(TCPSocket);
		return EXIT_FAILURE;
	}
*/

	if (TCPSocket == INVALID_SOCKET)
	{
		PrintError(Winsock_Error, _T("TCP request initialization failed"), WSAGetLastError(), NULL);

		delete[] SendBuffer;
		delete[] RecvBuffer;
		closesocket(TCPSocket);
		return EXIT_FAILURE;
	}

//Connect to server
	if (connect(TCPSocket, (PSOCKADDR)&SockAddr, TargetData.AddrLen) == SOCKET_ERROR) //The connection is RESET or other errors when connecting.
	{
		delete[] SendBuffer;
		delete[] RecvBuffer;
		closesocket(TCPSocket);
		return FALSE;
	}

//Send request
	if (send(TCPSocket, SendBuffer, (int)(SendSize + sizeof(USHORT)), NULL) == SOCKET_ERROR) //The connection is RESET or other errors when sending.
	{
		delete[] SendBuffer;
		delete[] RecvBuffer;
		closesocket(TCPSocket);
		return FALSE;
	}
	delete[] SendBuffer;

//Receive result
	SSIZE_T RecvLen = recv(TCPSocket, RecvBuffer, (int)RecvSize, NULL) - sizeof(USHORT);
	if (RecvLen <= FALSE) //The connection is RESET or other errors(including SOCKET_ERROR) when sending.
	{
		delete[] RecvBuffer;
		closesocket(TCPSocket);
		return FALSE;
	}
	memcpy(Recv, RecvBuffer + sizeof(USHORT), RecvLen);

	delete[] RecvBuffer;
	closesocket(TCPSocket);
	return RecvLen;
}

//Transmission of UDP protocol
size_t __stdcall UDPRequest(const PSTR Send, const size_t Length, const SOCKET_DATA TargetData, const size_t Index, const bool Local)
{
//Initialization
	PSTR Buffer = nullptr;
	try {
		Buffer = new char[PACKET_MAXSIZE]();
	}
	catch (std::bad_alloc)
	{
		PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		WSACleanup();
		TerminateService();
		return EXIT_FAILURE;
	}
	memset(Buffer, 0, PACKET_MAXSIZE);
	sockaddr_storage SockAddr = {0};
	SYSTEM_SOCKET UDPSocket = 0;
	memcpy(Buffer, Send, Length);

//Socket initialization
	if (TargetData.AddrLen == sizeof(sockaddr_in6)) //IPv6
	{
		UDPSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (Local && Parameter.DNSTarget.Local_IPv6)
			((PSOCKADDR_IN6)&SockAddr)->sin6_addr = Parameter.DNSTarget.Local_IPv6Target;
		else 
			((PSOCKADDR_IN6)&SockAddr)->sin6_addr = Parameter.DNSTarget.IPv6Target;
		SockAddr.ss_family = AF_INET6;
		((PSOCKADDR_IN6)&SockAddr)->sin6_port = htons(DNS_Port);
	}
	else { //IPv4
		UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (Local && Parameter.DNSTarget.Local_IPv4)
			((PSOCKADDR_IN)&SockAddr)->sin_addr = Parameter.DNSTarget.Local_IPv4Target;
		else 
			((PSOCKADDR_IN)&SockAddr)->sin_addr = Parameter.DNSTarget.IPv4Target;
		SockAddr.ss_family = AF_INET;
		((PSOCKADDR_IN)&SockAddr)->sin_port = htons(DNS_Port);
	}

	if (UDPSocket == INVALID_SOCKET)
	{
		PrintError(Winsock_Error, _T("UDP request initialization failed"), WSAGetLastError(), NULL);

		delete[] Buffer;
		closesocket(UDPSocket);
		return EXIT_FAILURE;
	}

//Send request
	if (sendto(UDPSocket, Buffer, (int)Length, 0, (PSOCKADDR)&SockAddr, TargetData.AddrLen) == SOCKET_ERROR)
	{
		PrintError(Winsock_Error, _T("UDP request error"), WSAGetLastError(), NULL);

		delete[] Buffer;
		closesocket(UDPSocket);
		return EXIT_FAILURE;
	}
	delete[] Buffer;

//Mark port(s) to list
	if (Index < THREAD_MAXNUM*THREAD_PARTNUM)
	{
		if (getsockname(UDPSocket, (PSOCKADDR)&SockAddr, (PINT)&(TargetData.AddrLen)) != 0)
		{
			closesocket(UDPSocket);
			return EXIT_FAILURE;
		}
		if (TargetData.AddrLen == sizeof(sockaddr_in6)) //IPv6
			PortList.SendPort[Index] = ((PSOCKADDR_IN6)&SockAddr)->sin6_port;
		else //IPv4
			PortList.SendPort[Index] = ((PSOCKADDR_IN)&SockAddr)->sin_port;
	}

	closesocket(UDPSocket);
	return EXIT_SUCCESS;
}
