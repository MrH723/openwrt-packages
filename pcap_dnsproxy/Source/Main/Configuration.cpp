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

//Version define
#define INI_VERSION       0.3         //Version of configuration file
#define HOSTS_VERSION     0.3         //Version of hosts file

//Codepage define
#define ANSI              1 * 100     //ANSI Codepage(Own)
#define UTF_8             65001       //Microsoft Windows Codepage of UTF-8
#define UTF_16_LE         1200 * 100  //Microsoft Windows Codepage of UTF-16 Little Endian/LE(Make it longer than the PACKET_MAXSIZE)
#define UTF_16_BE         1201 * 100  //Microsoft Windows Codepage of UTF-16 Big Endian/BE(Make it longer than the PACKET_MAXSIZE)
#define UTF_32_LE         12000       //Microsoft Windows Codepage of UTF-32 Little Endian/LE
#define UTF_32_BE         12001       //Microsoft Windows Codepage of UTF-32 Big Endian/BE

//Next line type define
//CR/Carriage Return is 0x0D and LF/Line Feed is 0x0A
#define CR_LF             1
#define LF                2
#define CR                3

std::vector<HostsTable> HostsList[2], *Using = &HostsList[0], *Modificating = &HostsList[1];

extern std::wstring Path;
extern Configuration Parameter;

//Read parameter from configuration file
size_t Configuration::ReadParameter()
{
//Initialization
	FILE *Input = nullptr;
	PSTR Buffer = nullptr, Addition = nullptr;
	try {
		Buffer = new char[PACKET_MAXSIZE]();
		Addition = new char[PACKET_MAXSIZE]();
	}
	catch (std::bad_alloc)
	{
		::PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		delete[] Buffer;
		delete[] Addition;
		return EXIT_FAILURE;
	}
	memset(Buffer, 0, PACKET_MAXSIZE);
	memset(Addition, 0, PACKET_MAXSIZE);

//Open file
	std::wstring ConfigPath(Path);
	ConfigPath.append(_T("Config.ini"));
	_wfopen_s(&Input, ConfigPath.c_str(), _T("rb"));

	if (Input == nullptr)
	{
		::PrintError(Parameter_Error, _T("Cannot open configuration file(Config.ini)"), NULL, NULL);

		delete[] Buffer;
		delete[] Addition;
		return EXIT_FAILURE;
	}

	size_t Encoding = 0, NextLineType = 0, ReadLength = 0, AdditionLength = 0, Line = 1, Index = 0, Start = 0;
//Read data
	while(!feof(Input))
	{
		memset(Buffer, 0, PACKET_MAXSIZE);
		ReadLength = fread_s(Buffer, PACKET_MAXSIZE, sizeof(char), PACKET_MAXSIZE, Input);
		if (Encoding == NULL)
		{
			ReadEncoding(Buffer, ReadLength, Encoding, NextLineType); //Read encoding
			ReadEncoding(Buffer, ReadLength, Encoding, NextLineType); //Read next line type
			if (Encoding == UTF_8)
			{
				memcpy(Addition, Buffer + 3, PACKET_MAXSIZE - 3);
				memset(Buffer, 0, PACKET_MAXSIZE);
				memcpy(Buffer, Addition, PACKET_MAXSIZE - 3);
				memset(Addition, 0, PACKET_MAXSIZE);
				ReadLength -= 3;
			}
			else if (Encoding == UTF_16_LE || Encoding == UTF_16_BE)
			{
				memcpy(Addition, Buffer + 2, PACKET_MAXSIZE - 2);
				memset(Buffer, 0, PACKET_MAXSIZE);
				memcpy(Buffer, Addition, PACKET_MAXSIZE - 2);
				memset(Addition, 0, PACKET_MAXSIZE);
				ReadLength -= 2;
			}
			else if (Encoding == UTF_32_LE || Encoding == UTF_32_BE)
			{
				memcpy(Addition, Buffer + 4, PACKET_MAXSIZE - 4);
				memset(Buffer, 0, PACKET_MAXSIZE);
				memcpy(Buffer, Addition, PACKET_MAXSIZE - 4);
				memset(Addition, 0, PACKET_MAXSIZE);
				ReadLength -= 4;
			}
		}

		for (Index = 0, Start = 0;Index < ReadLength + 1;Index++)
		{
		//CR/Carriage Return and LF/Line Feed
			if ((Encoding == UTF_8 || Encoding == ANSI || 
				(Encoding == UTF_16_LE || Encoding == UTF_32_LE) && Buffer[Index + 1] == 0 || 
				(Encoding == UTF_16_BE || Encoding == UTF_32_BE) && Buffer[Index - 1] == 0) && 
				(Buffer[Index] == 0x0D || Buffer[Index] == 0x0A))
			{
				if (Index - Start > 8) //Minimum length of rules
				{
					if (Parameter.ReadParameterData(Addition, Line) == EXIT_FAILURE)
					{
						delete[] Buffer;
						delete[] Addition;
						return EXIT_FAILURE;
					}
				}

				memset(Addition, 0, PACKET_MAXSIZE);
				AdditionLength = 0;
				Start = Index;
			//Mark lines
				if (NextLineType == CR_LF || NextLineType == CR)
				{
					if (Buffer[Index] == 0x0D)
						Line++;
				}
				else {
					if (Buffer[Index] == 0x0A)
						Line++;
				}

				continue;
			}
		//ASCII data
			else if (Buffer[Index] != 0)
			{
				if (AdditionLength < PACKET_MAXSIZE)
				{
					Addition[AdditionLength] = Buffer[Index];
					AdditionLength++;
				}
				else {
					::PrintError(Parameter_Error, _T("Parameter data of a line is too long"), NULL, Line);

					delete[] Buffer;
					delete[] Addition;
					return EXIT_FAILURE;
				}
			}
		//Last line
			else if (Buffer[Index] == 0 && Index == ReadLength && ReadLength != PACKET_MAXSIZE)
			{
				Line++;
				if (Parameter.ReadParameterData(Addition, Line) == EXIT_FAILURE)
				{
					delete[] Buffer;
					delete[] Addition;
					return EXIT_FAILURE;
				}
			}
		}
	}
	fclose(Input);
	delete[] Buffer;
	delete[] Addition;

//Set default
	static const char *PaddingData = ("abcdefghijklmnopqrstuvwabcdefghi"); //Default Ping padding data(Microsoft Windows)
	static const char *LocalDNSName = ("pcap_dnsproxy.localhost.server"); //Localhost DNS server name
	if (ntohs(this->ICMPOptions.ICMPID) <= 0)
		this->ICMPOptions.ICMPID = htons((USHORT)GetCurrentProcessId()); //Default DNS ID is current process ID
	if (ntohs(this->ICMPOptions.ICMPSequence) <= 0)
		this->ICMPOptions.ICMPSequence = htons(0x0001); //Default DNS Sequence is 0x0001
	if (ntohs(this->DomainTestOptions.DomainTestID) <= 0)
		this->DomainTestOptions.DomainTestID = htons(0x0001); //Default Domain Test DNS ID is 0x0001
	if (this->DomainTestOptions.DomainTestSpeed <= 5000) //5s is least time between Domain Tests
		this->DomainTestOptions.DomainTestSpeed = 900000; //Default Domain Test request every 15 minutes.
	if (this->PaddingDataOptions.PaddingDataLength <= 0)
	{
		this->PaddingDataOptions.PaddingDataLength = 33;
		memcpy(this->PaddingDataOptions.PaddingData, PaddingData, this->PaddingDataOptions.PaddingDataLength - 1); //Load default padding data from Microsoft Windows Ping
	}
	if (this->LocalhostServerOptions.LocalhostServerLength <= 0)
		this->LocalhostServerOptions.LocalhostServerLength = CharToDNSQuery((PSTR)LocalDNSName, this->LocalhostServerOptions.LocalhostServer); //Default Localhost DNS server name

//Check parameters
	if (!this->DNSTarget.IPv4 && !this->DNSTarget.IPv6 || !this->TCPMode && this->TCPOptions)
	{
		::PrintError(Parameter_Error, _T("Base rule(s) error"), NULL, NULL);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

//Read parameter data from configuration file
size_t Configuration::ReadParameterData(const PSTR Buffer, const size_t Line)
{
//Initialization
	PSTR Target = nullptr, LocalhostServer = nullptr;
	try {
		Target = new char[PACKET_MAXSIZE/8]();
		LocalhostServer = new char[PACKET_MAXSIZE/8]();
	}
	catch (std::bad_alloc)
	{
		::PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		delete[] Target;
		delete[] LocalhostServer;
		return EXIT_FAILURE;
	}
	memset(Target, 0, PACKET_MAXSIZE/8);
	memset(LocalhostServer, 0, PACKET_MAXSIZE/8);
	std::string Data = Buffer;
	SSIZE_T Result = 0;
//inet_ntop() and inet_pton() was only support in Windows Vista and newer system[Roy Tam]
#ifdef _WIN64
#else //x86
	sockaddr_storage SockAddr = {0};
	int SockLength = 0;
#endif

//Base block
	if (Data.find("Version = ") == 0 && Data.length() < 18)
	{
		double ReadVersion = atof(Buffer + 10);
		if (ReadVersion != INI_VERSION)
		{
			::PrintError(Parameter_Error, _T("Configuration file version error"), NULL, NULL);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	}
	else if (Data.find("Print Error = ") == 0 && Data.length() < 16)
	{
		Result = atoi(Buffer + 14);
		if (Result == 0)
			this->PrintError = false;
	}
	else if (Data.find("Hosts = ") == 0 && Data.length() < 14)
	{
		Result = atoi(Buffer + 8);
		if (Result >= 5)
			this->Hosts = Result * 1000;
		else if (Result > 0 && Result < 5)
			this->Hosts = 5000; //5s is least time between auto-refreshing
		else 
			this->Hosts = 0; //Read Hosts OFF
	}
	else if (Data.find("IPv4 DNS Address = ") == 0 && Data.length() > 25 && Data.length() < 35)
	{
		if (Data.find('.') == std::string::npos) //IPv4 Address
		{
			::PrintError(Parameter_Error, _T("DNS server IPv4 Address format error"), NULL, Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}

	//IPv4 Address check
		memset(Target, 0, PACKET_MAXSIZE/8);
		memcpy(Target, Buffer + 19, Data.length() - 19);
	//inet_ntop() and inet_pton() was only support in Windows Vista and newer system[Roy Tam]
	#ifdef _WIN64
		Result = inet_pton(AF_INET, Target, &(this->DNSTarget.IPv4Target));
		if (Result == FALSE)
	#else //x86
		memset(&SockAddr, 0, sizeof(sockaddr_storage));
		SockLength = sizeof(sockaddr_in);
		if (WSAStringToAddressA(Target, AF_INET, NULL, (LPSOCKADDR)&SockAddr, &SockLength) == SOCKET_ERROR)
	#endif
		{
			::PrintError(Parameter_Error, _T("DNS server IPv4 Address format error"), WSAGetLastError(), Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	#ifdef _WIN64
		else if (Result == RETURN_ERROR)
		{
			::PrintError(Parameter_Error, _T("DNS server IPv4 Address convert failed"), WSAGetLastError(), Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	#else //x86
		this->DNSTarget.IPv4Target = ((PSOCKADDR_IN)&SockAddr)->sin_addr;
	#endif
		this->DNSTarget.IPv4 = true;
	}
	else if (Data.find("IPv4 Local DNS Address = ") == 0 && Data.length() > 31 && Data.length() < 41)
	{
		if (Data.find('.') == std::string::npos) //IPv4 Address
		{
			::PrintError(Parameter_Error, _T("DNS server IPv4 Address format error"), NULL, Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}

	//IPv4 Address check
		memset(Target, 0, PACKET_MAXSIZE/8);
		memcpy(Target, Buffer + 25, Data.length() - 25);
	//inet_ntop() and inet_pton() was only support in Windows Vista and newer system [Roy Tam]
	#ifdef _WIN64
		Result = inet_pton(AF_INET, Target, &(this->DNSTarget.Local_IPv4Target));
		if (Result == FALSE)
	#else //x86
		memset(&SockAddr, 0, sizeof(sockaddr_storage));
		SockLength = sizeof(sockaddr_in);
		if (WSAStringToAddressA(Target, AF_INET, NULL, (LPSOCKADDR)&SockAddr, &SockLength) == SOCKET_ERROR)
	#endif
		{
			::PrintError(Parameter_Error, _T("DNS server IPv4 Address format error"), WSAGetLastError(), Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	#ifdef _WIN64
		else if (Result == RETURN_ERROR)
		{
			::PrintError(Parameter_Error, _T("DNS server IPv4 Address convert failed"), WSAGetLastError(), Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	#else //x86
		this->DNSTarget.Local_IPv4Target = ((PSOCKADDR_IN)&SockAddr)->sin_addr;
	#endif
		this->DNSTarget.Local_IPv4 = true;
	}
	else if (Data.find("IPv6 DNS Address = ") == 0 && Data.length() > 21 && Data.length() < 59)
	{
		if (Data.find(':') == std::string::npos) //IPv6 Address
		{
			::PrintError(Parameter_Error, _T("DNS server IPv6 Address format error"), NULL, Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}

	//IPv6 Address check
		memset(Target, 0, PACKET_MAXSIZE/8);
		memcpy(Target, Buffer + 19, Data.length() - 19);
	//inet_ntop() and inet_pton() was only support in Windows Vista and newer system [Roy Tam]
	#ifdef _WIN64
		Result = inet_pton(AF_INET6, Target, &(this->DNSTarget.IPv6Target));
		if (Result == FALSE)
	#else //x86
		memset(&SockAddr, 0, sizeof(sockaddr_storage));
		SockLength = sizeof(sockaddr_in6);
		if (WSAStringToAddressA(Target, AF_INET6, NULL, (LPSOCKADDR)&SockAddr, &SockLength) == SOCKET_ERROR)
	#endif
		{
			::PrintError(Parameter_Error, _T("DNS server IPv6 Address format error"), NULL, Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	#ifdef _WIN64
		else if (Result == RETURN_ERROR)
		{
			::PrintError(Parameter_Error, _T("DNS server IPv6 Address convert failed"), WSAGetLastError(), Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	#else //x86
		this->DNSTarget.IPv6Target = ((PSOCKADDR_IN6)&SockAddr)->sin6_addr;
	#endif
		this->DNSTarget.IPv6 = true;
	}
	else if (Data.find("IPv6 Local DNS Address = ") == 0 && Data.length() > 27 && Data.length() < 65)
	{
		if (Data.find(':') == std::string::npos) //IPv6 Address
		{
			::PrintError(Parameter_Error, _T("DNS server IPv6 Address format error"), NULL, Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}

	//IPv6 Address check
		memset(Target, 0, PACKET_MAXSIZE/8);
		memcpy(Target, Buffer + 25, Data.length() - 25);
	//inet_ntop() and inet_pton() was only support in Windows Vista and newer system [Roy Tam]
	#ifdef _WIN64
		Result = inet_pton(AF_INET6, Target, &(this->DNSTarget.Local_IPv6Target));
		if (Result == FALSE)
	#else //x86
		memset(&SockAddr, 0, sizeof(sockaddr_storage));
		SockLength = sizeof(sockaddr_in6);
		if (WSAStringToAddressA(Target, AF_INET6, NULL, (LPSOCKADDR)&SockAddr, &SockLength) == SOCKET_ERROR)
	#endif
		{
			::PrintError(Parameter_Error, _T("DNS server IPv6 Address format error"), NULL, Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	#ifdef _WIN64
		else if (Result == RETURN_ERROR)
		{
			::PrintError(Parameter_Error, _T("DNS server IPv6 Address convert failed"), WSAGetLastError(), Line);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
	#else //x86
		this->DNSTarget.Local_IPv6Target = ((PSOCKADDR_IN6)&SockAddr)->sin6_addr;
	#endif
		this->DNSTarget.Local_IPv6 = true;
	}
	else if (Data.find("Operation Mode = ") == 0)
	{
		if (Data.find("Server") == 17)
			this->ServerMode = true;
	}
	else if (Data.find("Protocol = ") == 0)
	{
		if (Data.find("TCP") == 11)
			this->TCPMode = true;
	}

//Extend Test
	else if (Data.find("IPv4 TTL = ") == 0 && Data.length() > 11 && Data.length() < 15)
	{
		Result = atoi(Buffer + 11);
		if (Result > 0 && Result < 256)
			this->HopLimitOptions.IPv4TTL = Result;
	}
	else if (Data.find("IPv6 Hop Limits = ") == 0 && Data.length() > 18 && Data.length() < 22)
	{
		Result = atoi(Buffer + 18);
		if (Result > 0 && Result < 256)
			this->HopLimitOptions.IPv6HopLimit = Result;
	}
	else if (Data.find("Hop Limits/TTL Fluctuation = ") == 0 && Data.length() > 29 && Data.length() < 33)
	{
		Result = atoi(Buffer + 29);
		if (Result >= 0 && Result < 256)
			this->HopLimitOptions.HopLimitFluctuation = Result;
	}
	else if (Data.find("IPv4 Options Filter = ") == 0 && Data.length() < 24)
	{
		Result = atoi(Buffer + 22);
		if (Result == 1)
			this->IPv4Options = true;
	}
	else if (Data.find("ICMP Test = ") == 0 && Data.length() > 12 && Data.length() < 18)
	{
		Result = atoi(Buffer + 12);
		if (Result >= 5)
			this->ICMPOptions.ICMPSpeed = Result* 1000;
		else if (Result > 0 && Result < 5)
			this->ICMPOptions.ICMPSpeed = 5000; //5s is least time between ICMP Tests
		else 
			this->ICMPOptions.ICMPSpeed = 0; //ICMP Test OFF
	}
	else if (Data.find("ICMP ID = ") == 0 && Data.length() < 17)
	{
		Result = strtol(Buffer + 10, NULL, 16);
		if (Result > 0)
			this->ICMPOptions.ICMPID = htons((USHORT)Result);
	}
	else if (Data.find("ICMP Sequence = ") == 0 && Data.length() < 23)
	{
		Result = strtol(Buffer + 16, NULL, 16);
		if (Result > 0)
			this->ICMPOptions.ICMPSequence = htons((USHORT)Result);
	}
	else if (Data.find("TCP Options Filter = ") == 0 && Data.length() < 23)
	{
		Result = atoi(Buffer + 21);
		if (Result == 1)
			this->TCPOptions = true;
	}
	else if (Data.find("DNS Options Filter = ") == 0 && Data.length() < 23)
	{
		Result = atoi(Buffer + 21);
		if (Result == 1)
			this->DNSOptions = true;
	}
	else if (Data.find("Blacklist Filter = ") == 0 && Data.length() < 22)
	{
		Result = atoi(Buffer + 19);
		if (Result == 1)
			this->Blacklist = true;
	}

//Data block
	else if (Data.find("Domain Test = ") == 0 && Data.length() > 17 && Data.length() < 270) //Maximum length of whole level domain is 253 bytes(Section 2.3.1 in RFC 1035).
	{
		memcpy(this->DomainTestOptions.DomainTest, Buffer + 14, Data.length() - 14);
		this->DomainTestOptions.DomainTestCheck = true;
	}
	else if (Data.find("Domain Test ID = ") == 0 && Data.length() < 24)
	{
		Result = strtol(Buffer + 17, NULL, 16);
		if (Result > 0)
			this->DomainTestOptions.DomainTestID = htons((USHORT)Result);
	}
	else if (Data.find("Domain Test Speed = ") == 0 && Data.length() > 20 && Data.length() < 26)
	{
		Result = atoi(Buffer + 20);
		if (Result > 0)
			this->DomainTestOptions.DomainTestSpeed = Result * 1000;
	}
	else if (Data.find("ICMP PaddingData = ") == 0)
	{
		if (Data.length() > 36 && Data.length() < 84) //The length of ICMP padding data must between 18 bytes and 64 bytes.
		{
			this->PaddingDataOptions.PaddingDataLength = Data.length() - 18;
			memcpy(this->PaddingDataOptions.PaddingData, Buffer + 19, Data.length() - 19);
		}
		else if (Data.length() >= 84)
		{
			::PrintError(Parameter_Error, _T("The ICMP padding data is too long"), NULL, Line);
		}
	}
	else if (Data.find("Localhost Server Name = ") == 0 && Data.length() > 26 && Data.length() < 280) //Maximum length of whole level domain is 253 bytes(Section 2.3.1 in RFC 1035).
	{
		PINT Point = nullptr;
		try {
			Point = new int[PACKET_MAXSIZE/8]();
		}
		catch (std::bad_alloc)
		{
			::PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

			delete[] Target;
			delete[] LocalhostServer;
			return EXIT_FAILURE;
		}
		memset(Point, 0, sizeof(int)*(PACKET_MAXSIZE/8));

		size_t Index = 0;
		this->LocalhostServerOptions.LocalhostServerLength = Data.length() - 23;

	//Convert from char to DNS query
		LocalhostServer[0] = 46;
		memcpy(LocalhostServer + sizeof(char), Buffer + 24, this->LocalhostServerOptions.LocalhostServerLength);
		for (Index = 0;Index < Data.length() - 25;Index++)
		{
		//Preferred name syntax(Section 2.3.1 in RFC 1035)
			if (LocalhostServer[Index] == 45 || LocalhostServer[Index] == 46 || LocalhostServer[Index] == 95 || 
				LocalhostServer[Index] > 47 && LocalhostServer[Index] < 58 || LocalhostServer[Index] > 96 && LocalhostServer[Index] < 123)
			{
				if (LocalhostServer[Index] == 46)
				{
					Point[Result] = (int)Index;
					Result++;
				}
				continue;
			}
			else {
				::PrintError(Parameter_Error, _T("Localhost server name format error"), NULL, Line);
				this->LocalhostServerOptions.LocalhostServerLength = 0;
				break;
			}
		}

		if (this->LocalhostServerOptions.LocalhostServerLength > 2)
		{
			PSTR LocalhostServerName = nullptr;
			try {
				LocalhostServerName = new char[PACKET_MAXSIZE/8]();
			}
			catch (std::bad_alloc)
			{
				::PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

				delete[] Target;
				delete[] LocalhostServer;
				delete[] Point;
				return EXIT_FAILURE;
			}
			memset(LocalhostServerName, 0, PACKET_MAXSIZE/8);

			for (Index = 0;Index < (size_t)Result;Index++)
			{
				if (Index == Result - 1)
				{
					LocalhostServerName[Point[Index]] = (int)(this->LocalhostServerOptions.LocalhostServerLength - Point[Index]);
					memcpy(LocalhostServerName + Point[Index] + 1, LocalhostServer + Point[Index] + 1, this->LocalhostServerOptions.LocalhostServerLength - Point[Index]);
				}
				else {
					LocalhostServerName[Point[Index]] = Point[Index + 1] - Point[Index] - 1;
					memcpy(LocalhostServerName + Point[Index] + 1, LocalhostServer + Point[Index] + 1, Point[Index + 1] - Point[Index]);
				}
			}

			memcpy(this->LocalhostServerOptions.LocalhostServer, LocalhostServerName, this->LocalhostServerOptions.LocalhostServerLength + 1);
			delete[] Point;
			delete[] LocalhostServerName;
		}
	}

	delete[] Target;
	delete[] LocalhostServer;
	return EXIT_SUCCESS;
}

//Read hosts from hosts file
size_t Configuration::ReadHosts()
{
//Initialization
	FILE *Input = nullptr;
	PSTR Buffer = nullptr, Addition = nullptr;
	try {
		Buffer = new char[PACKET_MAXSIZE]();
		Addition = new char[PACKET_MAXSIZE]();
	}
	catch (std::bad_alloc)
	{
		::PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		delete[] Buffer;
		delete[] Addition;
		WSACleanup();
		TerminateService();
		return EXIT_FAILURE;
	}
	memset(Buffer, 0, PACKET_MAXSIZE);
	memset(Addition, 0, PACKET_MAXSIZE);

//Open file
	std::wstring HostsFilePath(Path);
	HostsFilePath.append(_T("Hosts.ini"));

	size_t Encoding = 0, NextLineType = 0, ReadLength = 0, AdditionLength = 0, Line = 1, Index = 0, Start = 0;
	bool Local = false;
	while (true)
	{
		_wfopen_s(&Input, HostsFilePath.c_str(), _T("rb"));
		if (Input == nullptr)
		{
			::PrintError(Hosts_Error, _T("Cannot open hosts file(Hosts.ini)"), NULL, NULL);

			CleanupHostsTable();
			Sleep((DWORD)this->Hosts);
			continue;
		}
		
		Encoding = 0, Line = 1, AdditionLength = 0;
//Read data
		while(!feof(Input))
		{
			Local = false;
			memset(Buffer, 0, PACKET_MAXSIZE);
			ReadLength = fread_s(Buffer, PACKET_MAXSIZE, sizeof(char), PACKET_MAXSIZE, Input);
			if (Encoding == NULL)
			{
				ReadEncoding(Buffer, ReadLength, Encoding, NextLineType); //Read encoding
				ReadEncoding(Buffer, ReadLength, Encoding, NextLineType); //Read next line type
				if (Encoding == UTF_8)
				{
					memcpy(Addition, Buffer + 3, PACKET_MAXSIZE - 3);
					memset(Buffer, 0, PACKET_MAXSIZE);
					memcpy(Buffer, Addition, PACKET_MAXSIZE - 3);
					memset(Addition, 0, PACKET_MAXSIZE);
					ReadLength -= 3;
				}
				else if (Encoding == UTF_16_LE || Encoding == UTF_16_BE)
				{
					memcpy(Addition, Buffer + 2, PACKET_MAXSIZE - 2);
					memset(Buffer, 0, PACKET_MAXSIZE);
					memcpy(Buffer, Addition, PACKET_MAXSIZE - 2);
					memset(Addition, 0, PACKET_MAXSIZE);
					ReadLength -= 2;
				}
				else if (Encoding == UTF_32_LE || Encoding == UTF_32_BE)
				{
					memcpy(Addition, Buffer + 4, PACKET_MAXSIZE - 4);
					memset(Buffer, 0, PACKET_MAXSIZE);
					memcpy(Buffer, Addition, PACKET_MAXSIZE - 4);
					memset(Addition, 0, PACKET_MAXSIZE);
					ReadLength -= 4;
				}
			}

			for (Index = 0, Start = 0;Index < ReadLength + 1;Index++)
			{
			//CR/Carriage Return and LF/Line Feed
				if ((Encoding == UTF_8 || Encoding == ANSI || 
					(Encoding == UTF_16_LE || Encoding == UTF_32_LE) && Buffer[Index + 1] == 0 || 
					(Encoding == UTF_16_BE || Encoding == UTF_32_BE) && Buffer[Index - 1] == 0) && 
					(Buffer[Index] == 0x0D || Buffer[Index] == 0x0A))
				{
					if (Index - Start > 6) //Minimum length of IPv6 addresses and regular expression
					{
						if (Parameter.ReadHostsData(Addition, Line, Local) == EXIT_FAILURE)
						{
							memset(Addition, 0, PACKET_MAXSIZE);
							continue;
						}
					}

					memset(Addition, 0, PACKET_MAXSIZE);
					AdditionLength = 0;
					Start = Index;
				//Mark lines
					if (NextLineType == CR_LF || NextLineType == CR)
					{
						if (Buffer[Index] == 0x0D)
							Line++;
					}
					else {
						if (Buffer[Index] == 0x0A)
							Line++;
					}

					continue;
				}
			//ASCII data
				else if (Buffer[Index] != 0)
				{
					if (AdditionLength < PACKET_MAXSIZE)
					{
						Addition[AdditionLength] = Buffer[Index];
						AdditionLength++;
					}
					else {
						::PrintError(Parameter_Error, _T("Hosts data of a line is too long"), NULL, Line);

						memset(Addition, 0, PACKET_MAXSIZE);
						AdditionLength = 0;
						continue;
					}
				}
			//Last line
				else if (Buffer[Index] == 0 && Index == ReadLength && ReadLength != PACKET_MAXSIZE)
				{
					if (Parameter.ReadHostsData(Addition, Line, Local) == EXIT_FAILURE)
					{
						memset(Addition, 0, PACKET_MAXSIZE);
						AdditionLength = 0;
						break;
					}
				}
			}
		}
		fclose(Input);

	//Update Hosts list
		if (!Modificating->empty())
		{
			Using->swap(*Modificating);
			for (std::vector<HostsTable>::iterator iter = Modificating->begin();iter != Modificating->end();iter++)
				delete[] iter->Response;
			Modificating->clear();
			Modificating->resize(0);
		}
		else { //Hosts Table is empty
			CleanupHostsTable();
		}
		
	//Auto-refresh
		Sleep((DWORD)this->Hosts);
	}

	delete[] Buffer;
	delete[] Addition;
	return EXIT_SUCCESS;
}

//Read hosts data from hosts file
size_t Configuration::ReadHostsData(const PSTR Buffer, const size_t Line, bool &Local)
{
	if (strlen(Buffer) < 3) //Minimum length of whole level domain is 3 bytes(Section 2.3.1 in RFC 1035).
		return EXIT_SUCCESS;
	std::string Data = Buffer;

	bool Whitelist = false;
//Base block
	if (Data.find("Version = ") == 0 && Data.length() < 18)
	{
		double ReadVersion = atof(Buffer + 10);
		if (ReadVersion != HOSTS_VERSION)
		{
			::PrintError(Hosts_Error, _T("Hosts file version error"), NULL, NULL);

			return EXIT_FAILURE;
		}
		else {
			return EXIT_SUCCESS;
		}
	}
//Main hosts list
	else if (Data.find("[Hosts]") == 0)
	{
		Local = false;
		return EXIT_SUCCESS;
	}
//Local hosts list
	else if (Data.find("[Local Hosts]") == 0)
	{
		if (this->DNSTarget.Local_IPv4 || this->DNSTarget.Local_IPv6)
		{
			Local = true;
			return EXIT_SUCCESS;
		}
		else {
			return EXIT_SUCCESS;
		}
	}
//Whitelist
	else if (Data.find("NULL ") == 0 || Data.find("NULL	") == 0)
	{
		Whitelist = true;
	}
//Comments
	else if (Data.find(35) == 0) //Number Sign/NS
	{
		return EXIT_SUCCESS;
	}

//Initialization
	PSTR Addr = nullptr;
	try {
		Addr = new char[PACKET_MAXSIZE/8]();
	}
	catch (std::bad_alloc)
	{
		::PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

		CleanupHostsTable();
		WSACleanup();
		TerminateService();
		return EXIT_FAILURE;
	}
	memset(Addr, 0, PACKET_MAXSIZE/8);
	size_t Index = 0, Front = 0, Rear = 0;
//inet_ntop() and inet_pton() was only support in Windows Vista and newer system [Roy Tam]
#ifdef _WIN64
	SSIZE_T Result = 0;
#endif

//Mark space and horizontal tab/HT
	if (Data.find(32) != std::string::npos) //Space
	{
		Index = Data.find(32);
		Front = Index;
		if (Data.rfind(32) > Index)
			Rear = Data.rfind(32);
		else 
			Rear = Front;
	}
	if (Data.find(9) != std::string::npos) //Horizontal Tab/HT
	{
		if (Index == 0)
			Index = Data.find(9);
		if (Index > Data.find(9))
			Front = Data.find(9);
		else 
			Front = Index;
		if (Data.rfind(9) > Index)
			Rear = Data.rfind(9);
		else 
			Rear = Front;
	}

//Whitelist block
	if (Whitelist)
	{
		HostsTable Temp;
		std::string WhitelistRegex;
		
		WhitelistRegex.append(Data, Rear + 1, Data.length() - Rear);
	//Mark patterns
		try {
			std::regex TempPattern(WhitelistRegex, std::regex_constants::extended);
			Temp.Pattern = TempPattern;
		}
		catch(std::regex_error)
		{
			::PrintError(Hosts_Error, _T("Regular expression pattern error"), NULL, Line);

			delete[] Addr;
			CleanupHostsTable();
			return EXIT_FAILURE;
		}

		Temp.White = true;
		Modificating->push_back(Temp);
	}
//Main hosts block
	else if (!Local)
	{
	//Read data
		if (Front > 2) //Minimum length of IPv6 addresses
		{
			HostsTable Temp;
			Temp.Local = false;
			size_t Vertical[THREAD_MAXNUM/4] = {0}, VerticalIndex = 0;

		//Multiple Addresses
			for (Index = 0;Index < Front;Index++)
			{
				if (Buffer[Index] == 124)
				{
					if (VerticalIndex > THREAD_MAXNUM/4)
					{
						::PrintError(Hosts_Error, _T("Too many Hosts IP addresses"), NULL, Line);

						delete[] Addr;
						CleanupHostsTable();
						return EXIT_FAILURE;
					}
					else if (Index - Vertical[VerticalIndex] < 2) //Minimum length of IPv6 address
					{
						::PrintError(Hosts_Error, _T("Multiple addresses format error"), NULL, Line);

						delete[] Addr;
						CleanupHostsTable();
						return EXIT_FAILURE;
					}
					else {
						VerticalIndex++;
						Vertical[VerticalIndex] = Index + 1;
					}
				}
			}
			VerticalIndex++;
			Vertical[VerticalIndex] = Front + 1;
			Temp.ResponseNum = VerticalIndex;

			Index = 0;
			if (VerticalIndex > 0)
			{
				memset(Addr, 0, PACKET_MAXSIZE/8);

			//Response initialization
				try {
					Temp.Response = new char[PACKET_MAXSIZE]();
				}
				catch (std::bad_alloc)
				{
					::PrintError(System_Error, _T("Memory allocation failed"), NULL, NULL);

					delete[] Addr;
					CleanupHostsTable();
					WSACleanup();
					TerminateService();
					return EXIT_FAILURE;
				}
				memset(Temp.Response, 0, PACKET_MAXSIZE);
			//inet_ntop() and inet_pton() was only support in Windows Vista and newer system [Roy Tam]
			#ifdef _WIN64
			#else //x86
				sockaddr_storage SockAddr = {0};
				int SockLength = 0;
			#endif

			//AAAA Records(IPv6)
				if (Data.find(58) != std::string::npos)
				{
					Temp.Protocol = AF_INET6;
					while (Index < VerticalIndex)
					{
					//Make a response
						dns_aaaa_record rsp = {0};
						rsp.Name = htons(0xC00C); //Pointer of same request
						rsp.Classes = htons(Class_IN); //Class IN
						rsp.TTL = htonl(600); //10 minutes
						rsp.Type = htons(AAAA_Records);
						rsp.Length = htons(sizeof(in6_addr));

					//Convert addresses
						memcpy(Addr, Buffer + Vertical[Index], Vertical[Index + 1] - Vertical[Index] - 1);
					//inet_ntop() and inet_pton() was only support in Windows Vista and newer system [Roy Tam]
					#ifdef _WIN64
						Result = inet_pton(AF_INET6, Addr, &rsp.Addr);
						if (Result == FALSE)
					#else //x86
						SockLength = sizeof(sockaddr_in6);
						if (WSAStringToAddressA(Addr, AF_INET6, NULL, (LPSOCKADDR)&SockAddr, &SockLength) == SOCKET_ERROR)
					#endif
						{
							::PrintError(Hosts_Error, _T("Hosts IPv6 address format error"), NULL, Line);
							delete[] Temp.Response;

							Index++;
							continue;
						}
					#ifdef _WIN64
						else if (Result == RETURN_ERROR)
						{
							::PrintError(Hosts_Error, _T("Hosts IPv6 address convert failed"), WSAGetLastError(), Line);
							delete[] Temp.Response;

							Index++;
							continue;
						}
					#else //x86
						rsp.Addr = ((LPSOCKADDR_IN6)&SockAddr)->sin6_addr;
					#endif
						memcpy(Temp.Response + Temp.ResponseLength, &rsp, sizeof(dns_aaaa_record));
						
					//Check length
						Temp.ResponseLength += sizeof(dns_aaaa_record);
						if (Temp.ResponseLength >= PACKET_MAXSIZE / 8 * 7 - sizeof(dns_hdr) - sizeof(dns_qry)) //Maximun length of domain levels(256 bytes), DNS Header and DNS Query
						{
							::PrintError(Hosts_Error, _T("Too many Hosts IP addresses"), NULL, Line);
							delete[] Temp.Response;

							Index++;
							continue;
						}

						memset(Addr, 0, PACKET_MAXSIZE/8);
						Index++;
					}
				}
			//A Records(IPv4)
				else {
					Temp.Protocol = AF_INET;
					while (Index < VerticalIndex)
					{
					//Make a response
						dns_a_record rsp = {0};
						rsp.Name = htons(0xC00C); //Pointer of same request
						rsp.Classes = htons(Class_IN); //Class IN
						rsp.TTL = htonl(600); //10 minutes
						rsp.Type = htons(A_Records);
						rsp.Length = htons(sizeof(in_addr));

					//Convert addresses
						memcpy(Addr, Buffer + Vertical[Index], Vertical[Index + 1] - Vertical[Index] - 1);
					//inet_ntop() and inet_pton() was only support in Windows Vista and newer system [Roy Tam]
					#ifdef _WIN64
						Result = inet_pton(AF_INET, Addr, &rsp.Addr);
						if (Result == FALSE)
					#else //x86
						SockLength = sizeof(sockaddr_in);
						if (WSAStringToAddressA(Addr, AF_INET, NULL, (LPSOCKADDR)&SockAddr, &SockLength) == SOCKET_ERROR)
					#endif
						{
							::PrintError(Hosts_Error, _T("Hosts IPv4 address format error"), NULL, Line);
								
							delete[] Temp.Response;
							Index++;
							continue;
						}
					#ifdef _WIN64
						else if (Result == RETURN_ERROR)
						{
							::PrintError(Hosts_Error, _T("Hosts IPv4 address convert failed"), WSAGetLastError(), Line);

							delete[] Temp.Response;
							Index++;
							continue;
						}
					#else //x86
						rsp.Addr = ((LPSOCKADDR_IN)&SockAddr)->sin_addr;
					#endif
						memcpy(Temp.Response + Temp.ResponseLength, &rsp, sizeof(dns_a_record));

					//Check length
						Temp.ResponseLength += sizeof(dns_a_record);
						if (Temp.ResponseLength >= PACKET_MAXSIZE / 8 * 7 - sizeof(dns_hdr) - sizeof(dns_qry)) //Maximun length of domain levels(256 bytes), DNS Header and DNS Query
						{
							::PrintError(Hosts_Error, _T("Too many Hosts IP addresses"), NULL, Line);

							delete[] Temp.Response;
							Index++;
							continue;
						}

						memset(Addr, 0, PACKET_MAXSIZE/8);
						Index++;
					}
				}

			//Mark patterns
				std::string Domain;
				Domain.append(Data, Rear + 1, Data.length() - Rear);
				try {
					std::regex TempPattern(Domain, std::regex_constants::extended);
					Temp.Pattern = TempPattern;
				}
				catch(std::regex_error)
				{
					::PrintError(Hosts_Error, _T("Regular expression pattern error"), NULL, Line);

					delete[] Temp.Response;
					delete[] Addr;
					CleanupHostsTable();
					return EXIT_FAILURE;
				}

			//Add to global HostsTable
				if (Temp.ResponseLength >= sizeof(dns_qry) + sizeof(in_addr)) //The shortest reply is a A Records with Question part
					Modificating->push_back(Temp);
			}
		}
	}
//Local hosts block
	else if (Local && (Parameter.DNSTarget.Local_IPv4 || Parameter.DNSTarget.Local_IPv6))
	{
		HostsTable Temp;

	//Mark patterns
		try {
			std::regex TempPattern(Data, std::regex_constants::extended);
			Temp.Pattern = TempPattern;
		}
		catch(std::regex_error)
		{
			::PrintError(Hosts_Error, _T("Regular expression pattern error"), NULL, Line);

			delete[] Addr;
			CleanupHostsTable();
			return EXIT_FAILURE;
		}

		Temp.Local = true;
		Modificating->push_back(Temp);
	}

	delete[] Addr;
	return EXIT_SUCCESS;
}

//Read encoding of file
inline void __stdcall ReadEncoding(const PSTR Buffer, const size_t Length, size_t &Encoding, size_t &NextLineType)
{
//Read next line type
	size_t Index = 0;
	for (Index = 4;Index < Length - 3;Index++)
	{
		if (Buffer[Index] == 0x0A && 
			((Encoding == ANSI || Encoding == UTF_8) && Buffer[Index - 1] == 0x0D || 
			(Encoding == UTF_16_LE || Encoding == UTF_16_BE) && Buffer[Index - 2] == 0x0D || 
			(Encoding == UTF_32_LE || Encoding == UTF_32_BE) && Buffer[Index - 4] == 0x0D))
		{
			NextLineType = CR_LF;
			return;
		}
		else if (Buffer[Index] == 0x0A && 
			((Encoding == ANSI || Encoding == UTF_8) && Buffer[Index - 1] != 0x0D || 
			(Encoding == UTF_16_LE || Encoding == UTF_16_BE) && Buffer[Index - 2] != 0x0D || 
			(Encoding == UTF_32_LE || Encoding == UTF_32_BE) && Buffer[Index - 4] != 0x0D))
		{
			NextLineType = LF;
			return;
		}
		else if (Buffer[Index] == 0x0D && 
			((Encoding == ANSI || Encoding == UTF_8) && Buffer[Index + 1] != 0x0A || 
			(Encoding == UTF_16_LE || Encoding == UTF_16_BE) && Buffer[Index + 2] != 0x0A || 
			(Encoding == UTF_32_LE || Encoding == UTF_32_BE) && Buffer[Index + 4] != 0x0A))
		{
			NextLineType = CR;
			return;
		}
	}

//8-bit Unicode Transformation Format/UTF-8 with BOM
	if (Buffer[0] == 0xFFFFFFEF && Buffer[1] == 0xFFFFFFBB && Buffer[2] == 0xFFFFFFBF)
		Encoding = UTF_8;
//32-bit Unicode Transformation Format/UTF-32 Little Endian/LE
	else if (Buffer[0] == 0xFFFFFFFF && Buffer[1] == 0xFFFFFFFE && Buffer[2] == 0 && Buffer[3] == 0)
		Encoding = UTF_32_LE;
//32-bit Unicode Transformation Format/UTF-32 Big Endian/BE
	else if (Buffer[0] == 0 && Buffer[1] == 0 && Buffer[2] == 0xFFFFFFFE && Buffer[3] == 0xFFFFFFFF)
		Encoding = UTF_32_BE;
//16-bit Unicode Transformation Format/UTF-16 Little Endian/LE
	else if (Buffer[0] == 0xFFFFFFFF && Buffer[1] == 0xFFFFFFFE)
		Encoding = UTF_16_LE;
//16-bit Unicode Transformation Format/UTF-16 Big Endian/BE
	else if (Buffer[0] == 0xFFFFFFFE && Buffer[1] == 0xFFFFFFFF)
		Encoding = UTF_16_BE;
//8-bit Unicode Transformation Format/UTF-8 without BOM/Microsoft Windows ANSI Codepages
	else 
		Encoding = ANSI;

	return;
}

//Cleanup Hosts Table
inline void __stdcall CleanupHostsTable()
{
//Delete all heap data
	for (std::vector<HostsTable>::iterator iter = Modificating->begin();iter != Modificating->end();iter++)
		delete[] iter->Response;
	for (std::vector<HostsTable>::iterator iter = Using->begin();iter != Using->end();iter++)
		delete[] iter->Response;

//Cleanup vector list
	Modificating->clear();
	Modificating->resize(0);
	Using->clear();
	Using->resize(0);
	return;
}
