﻿Pcap_DNSProxy
=====
A local DNS server base on WinPcap.

### 关于分支
本分支 master 为 Pcap_DNSProxy 项目的主分支，用于存放源代码和说明文档。**编译版本请移步：**
* [Release_x86 分支](https://github.com/chengr28/pcap_dnsproxy/tree/Release_x86)<br />
* [Release_x64 分支](https://github.com/chengr28/pcap_dnsproxy/tree/Release_x64)<br />

### 使用方法
* Windows版参见 [Wiki](https://github.com/chengr28/pcap_dnsproxy/wiki) 中 [ReadMe](https://github.com/chengr28/pcap_dnsproxy/wiki/ReadMe) 之内容
* Linux版参见 [Wiki](https://github.com/chengr28/pcap_dnsproxy/wiki) 中 [ReadMe_Linux](https://github.com/chengr28/pcap_dnsproxy/wiki/ReadMe_Linux) 之内容
* Mac版参见 [Wiki](https://github.com/chengr28/pcap_dnsproxy/wiki) 中 [ReadMe_Mac](https://github.com/chengr28/pcap_dnsproxy/wiki/ReadMe_Mac) 之内容

### 简介
* **Pcap_DNSProxy 是一个基于WinPcap制作的用于忽略DNS投毒污染的小工具，后期也加入了对包含正则表达式的Hosts的支持**<br />
* 现在网络上有很多使用TCP协议进行解析的工具，以此来躲避DNS投毒污染。但事实上已经出现有使用TCP协议请求域名解析时被RESET/连接重置的情况，而使用UDP协议则又会被DNS投毒污染，导致始终无法获得正确的域名解析。**本工具主要工作在UDP协议上，可以将伪造的数据包完全过滤，同时UDP协议比起TCP协议更具有占用资源低和发送转发接收速度快等特点。本工具同时也支持使用TCP协议进行请求，而且在被连接重置时会自动切换到UDP协议，可以使请求者获得正确的域名解析**<br />
* **而支持正则表达式的Hosts，则可以为使用者提供更加便捷的途径设定域名所对应的地址，避免修改系统文件的麻烦**<br />
* 本工具使用 C/C++ 编写而成，使用 Visual Studio 2012(Update 3)/VC++ 11.0(Windows)、GCC 4.7.2/g++(Linux) 和 Xcode 5.0.1/Apple LLVM 5.0(Mac) 进行编译，完全支持 Unicode<br />

### 特点
* 同时支持IPv4/IPv6协议，也可单独开启
* 同时支持TCP/UDP协议
* 原生64位/x64目标平台编译版本(Windows)
* 无需安装VC++运行库，本地代码编译不含任何托管代码(Windows)
* 作为服务工作于系统底层
* 多线程模型
* 正则表达式支持由 C++ STL(Windows)/系统自带的正则库(Linux/Mac) 提供
* 使用WinPcap(Windows)/LibPcap(Linux)利用系统底层驱动抓取数据包，多种过滤方式忽略接收到的伪造数据包
* 支持服务器模式，相当于搭建了一个小型的DNS服务器，能为其他设备提供解析服务
* 支持境内DNS服务器解析，可提高境内服务器的访问速度
* 丰富的配置选项，配置和Hosts文件支持 ANSI、UTF-8(/BOM)、UTF-16(LE/BE) 和 UTF-32(LE/BE) 编码以及 Windows/Unix/Macintosh 换行格式
* 错误报告功能

### 支持平台
* 本工具只支持原生IPv4/IPv6网络，**非原生IPv6切勿开启IPv6功能**
* 网络设备类型为 Ethernet 或直接使用 PPPoE 协议均可
* Windows 平台
    * **所有 Windows NT(4) 以及更新内核的版本（32位/x86版本）** 和 **Windows Vista 以及更新的版本（64位/x64版本）**
    * 支持最新版本 [WinPcap](http://www.winpcap.org/install/default.htm)
* Linux 平台
    * 支持 [编译所需依赖包](https://github.com/chengr28/pcap_dnsproxy/wiki/ReadMe_Linux) 的Linux发行版
    * 支持最新版本 [Libpcap](http://www.tcpdump.org)
* Mac 平台
    * **采用Intel平台处理器的 Mac OS X 10.5 Leopard 以及更新的版本**
