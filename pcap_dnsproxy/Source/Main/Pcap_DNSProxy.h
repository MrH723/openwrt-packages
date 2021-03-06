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


#include "Pcap_DNSProxy_Base.h"

//Code define
#define RETURN_ERROR      -1
#define PACKET_MAXSIZE    2048                      //Maximum length of packets(2048 bytes)
#define THREAD_MAXNUM     128                       //Maximum threads number
#define THREAD_PARTNUM    4                         //Parts of threads, also define localhost sockets number: 0 is IPv6/UDP, 1 is IPv4/UDP, 2 is IPv6/TCP, 3 is IPv4/TCP
#define LOCALSERVERNAME   _T("PcapDNSProxyService") //Name of local server service
#define SYSTEM_SOCKET     UINT_PTR                  //System Socket defined, not the same in x86(unsigned int) and x64(unsigned __int64) platform
#define TIME_OUT          1000                      //Timeout in seconds(1000 ms)

//Error Type define, please see PrintError.cpp
#define System_Error      1
#define Parameter_Error   2
#define Hosts_Error       3
#define Winsock_Error     4
#define WinPcap_Error     5

//Socket Data structure
typedef struct _socket_data_
{
	SYSTEM_SOCKET         Socket;
	sockaddr_storage      SockAddr;
	int                   AddrLen;
}SOCKET_Data, SOCKET_DATA;

//Configuration class
class Configuration {
public:
//Base block(Public)
	bool            PrintError;
	size_t          Hosts;
	SYSTEM_SOCKET   LocalSocket[THREAD_PARTNUM];
	struct _dns_target_ {
		bool        IPv4;
		in_addr     IPv4Target;
		bool        IPv6;
		in6_addr    IPv6Target;
		bool        Local_IPv4;
		in_addr     Local_IPv4Target;
		bool        Local_IPv6;
		in6_addr    Local_IPv6Target;
	}DNSTarget;
	bool            ServerMode;
	bool            TCPMode;
//Extend Test block(Public)
	struct _hoplimit_options_ {
		size_t      IPv4TTL;
		size_t      IPv6HopLimit;
		size_t      HopLimitFluctuation;
	}HopLimitOptions;
	bool            IPv4Options;
	struct _icmp_options_ {
		USHORT      ICMPID;
		USHORT      ICMPSequence;
		size_t      ICMPSpeed;
	}ICMPOptions;
	bool            TCPOptions;
	bool            DNSOptions;
	bool            Blacklist;
//Data block(Public)
	struct _domaintest_options_ {
		bool        DomainTestCheck;
		PSTR        DomainTest;
		USHORT      DomainTestID;
		size_t      DomainTestSpeed;
	}DomainTestOptions;
	struct _paddingdata_options_ {
		PSTR        PaddingData;
		size_t      PaddingDataLength;
	}PaddingDataOptions;
	struct _localhostserver_options_ {
		PSTR        LocalhostServer;
		size_t      LocalhostServerLength;
	}LocalhostServerOptions;
	::Configuration();
	~Configuration();

	size_t ReadParameter();
	size_t ReadHosts();

private:
	size_t ReadParameterData(const PSTR Buffer, const size_t Line);
	size_t ReadHostsData(const PSTR Buffer, const size_t Line, bool &Local);
};

//Hosts list class
class HostsTable {
public:
	bool             Local, White;
	size_t           Protocol;
	PSTR             Response;
	size_t           ResponseLength, ResponseNum;
	std::regex       Pattern;

	HostsTable();
};

//System&Request port list class
class PortTable {
public:
	SOCKET_DATA      *RecvData;    //System receive sockets/Addresses records
	PUSHORT          SendPort;     //Request ports records
	PortTable();
	~PortTable();

	size_t MatchToSend(const PSTR Buffer, const size_t Length, const USHORT RequestPort);
};

//PrintError.cpp
size_t __stdcall PrintError(const size_t Type, const PWSTR Message, const SSIZE_T Code, const size_t Line);

//Protocol.cpp
//ULONGLONG htonl64(ULONGLONG Val);
//ULONGLONG ntohl64(ULONGLONG Val);
//ULONG __stdcall GetFCS(const PSTR Buffer, const size_t Length);
USHORT __stdcall GetChecksum(const USHORT *Buffer, const size_t Length);
USHORT __stdcall ICMPv6Checksum(const PSTR Buffer, const size_t Length);
bool __stdcall CheckSpecialAddress(const void *vAddr, const size_t Protocol);
USHORT __stdcall UDPChecksum(const PSTR Buffer, const size_t Length, const size_t Protocol);
size_t __stdcall CharToDNSQuery(const PSTR FName, PSTR TName);
size_t __stdcall DNSQueryToChar(const PSTR TName, PSTR FName);
bool __stdcall GetLocalAddress(sockaddr_storage &SockAddr, const size_t Protocol);
size_t __stdcall LocalAddressToPTR(const size_t Protocol);
void __stdcall RamdomDomain(PSTR Domain, const size_t Length);

//Configuration.cpp
inline void __stdcall ReadEncoding(const PSTR Buffer, const size_t Length, size_t &Encoding, size_t &NextLineType);
inline void __stdcall CleanupHostsTable();

//Service.cpp
size_t __stdcall GetServiceInfo();
size_t WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
size_t WINAPI ServiceControl(const DWORD dwControlCode);
BOOL WINAPI ExecuteService();
void __stdcall TerminateService();
DWORD WINAPI ServiceProc(LPVOID lpParameter);
BOOL WINAPI UpdateServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwServiceSpecificExitCode, DWORD dwCheckPoint, DWORD dwWaitHint);

//Process.cpp
size_t __stdcall RequestProcess(const PSTR Send, const size_t Length, const SOCKET_DATA FunctionData, const size_t Protocol, const size_t Index);
inline size_t __stdcall CheckHosts(const PSTR Request, const size_t Length, PSTR Result, bool &Local);
size_t __stdcall TCPReceiveProcess(const SOCKET_DATA FunctionData, const size_t Index);

//Main.cpp
inline size_t __stdcall FirewallTest();

//Monitor.cpp
size_t __stdcall MonitorInitialization();
size_t __stdcall UDPMonitor(const SOCKET_DATA LocalhostData);
size_t __stdcall TCPMonitor(const SOCKET_DATA LocalhostData);

//Captrue.cpp
size_t __stdcall CaptureInitialization();
size_t __stdcall Capture(const pcap_if *pDrive);
size_t __stdcall IPLayer(const PSTR Recv, const size_t Length, const USHORT Protocol);
inline bool __stdcall ICMPCheck(const PSTR Buffer, const size_t Length, const size_t Protocol);
inline bool __stdcall TCPCheck(const PSTR Buffer);
inline bool __stdcall DTDNSOCheck(const PSTR Buffer, bool &SignHopLimit);
inline size_t __stdcall DNSMethod(const PSTR Recv, const size_t Length, const size_t Protocol, const bool Local);

//Request.cpp
size_t __stdcall DomainTest(const size_t Protocol);
size_t __stdcall ICMPEcho();
size_t __stdcall ICMPv6Echo();
size_t __stdcall TCPRequest(const PSTR Send, const size_t SendSize, PSTR Recv, const size_t RecvSize, const SOCKET_DATA TargetData, const bool Local);
size_t __stdcall UDPRequest(const PSTR Send, const size_t Length, const SOCKET_DATA TargetData, const size_t Index, const bool Local);

/*
//Console.cpp
BOOL WINAPI CtrlHandler(const DWORD fdwCtrlType);
*/
