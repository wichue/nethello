# TCP/UDP network testing tool implemented in C++

[![](https://img.shields.io/badge/license-MIT-green.svg)](https://github.com/wichue/nethello/blob/master/LICENSE)
[![](https://img.shields.io/badge/language-c++-red.svg)](https://en.cppreference.com/)
[![](https://img.shields.io/badge/platform-linux%20|%20windows-blue.svg)](https://github.com/wichue/nethello)
## original intention
The lightweight tcp / udp network test tool based on C + + 11 implementation includes common basic modules, event loop, network modules, etc., without other three-party library dependence, convenient compilation and deployment.
## Features
- Text chat, optional TCP/UDP protocol, test if the network is connected.
- Performance testing, optional TCP/UDP protocol, can set sending rate, packet length, testing duration, output interval, etc.
- File transfer, currently supports TCP protocol, allowing the client to send files to the server.
- Raw socket testing, implementing text chat and performance testing using raw sockets, currently only supports Linux.

## Compile and Install
### linux
```shell
git clone git@github.com:wichue/nethello.git
cd nethello
mkdir build
cd build
cmake ..
make
sudo make install
```
### windows
Taking VS2019 as an example:
- Create a new project, empty project, right-click on the source file, add - existing item, and add the source code file.
- Right click on the project name, properties, configuration properties, C/C++, general, add the included directory, and add the dependent header file directory.
- Right click on project name, properties, configuration properties, C/C++, preprocessor, preprocessor definition, add preprocessor macro definition:
```shell
_CRT_SECURE_NO_WARNINGS
_WINSOCK_DEPRECATED_NO_WARNINGS
_CRT_NONSTDC_NO_DEPRECATE
```
- Right click on the project name and generate it。

## Use the example
- Text mode and performance testing default to TCP, support UDP (- u), file transfer only supports TCP.
### Text chat
```shell
./nethello -s -p 9090
./nethello -c 127.0.0.1 -p 9090
```
- tcp Text chat

![text_tcp](https://github.com/wichue/nethello/blob/master/doc/text_tcp.png)
- udp Text chat

![text_udp](https://github.com/wichue/nethello/blob/master/doc/text_udp.png)
### Performance testing
```shell
./nethello -s -p 9090 -P
./nethello -c 127.0.0.1 -p 9090 -P
```
- tcp Performance testing

![perf_tcp](https://github.com/wichue/nethello/blob/master/doc/perf_tcp.png)
- udp Performance testing

![perf_udp](https://github.com/wichue/nethello/blob/master/doc/perf_udp.png)
### File transfer
```shell
./nethello -s -p 9090 -F
./nethello -c 127.0.0.1 -p 9090 -F -S /root/gitcode/nethello/build/2GB.txt -D /root/gitcode/nethello/build/file
```
![file](https://github.com/wichue/nethello/blob/master/doc/file.png)
### Raw sockets and NPCAP
- text chat

```shell
# Mutual exchange
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d
```
![raw_text](https://github.com/wichue/nethello/blob/master/doc/raw_text.png)
- performance testing

```shell
# receiving end
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d -P -l 0
# Sending end
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d -P
```
![raw_perf](https://github.com/wichue/nethello/blob/master/doc/raw_perf.png)

- file transfer
```shell
# receiving end
./nethello -r -s -F -I eth0
# Sending end
./nethello -r -c 123 -F -I eth0 -S /root/gitcode/nethello/build/500M.txt -D /root/gitcode/nethello/build/file
```

## Windows original socket word npcap configuration instructions
https://npcap.com/#download
Download: npcap-sdk-1.13.zip， Extract and retrieve the Lib and Include folders.
Vs configuration
```
VS Attribute - C/C++- Attach Include Directory, Put \Add the 'Include' directory.
VS Properties Debugging Environment, Add PATH= \Lib;% PATH%
VS Properties Connector General Additional Library Catalog, Add \Lib
Replace with a complete directory.
```
-The - I option of Windows recognizes the network card description, and fuzzy matching based on fields is sufficient. For example, a USB network card contains a 'USB' field- I USB can recognize the network card.
-If prompted for missing Packet.dll and wpcap.dll, copy the libraries suitable for the current operating system environment to the executable file directory.
-If NPCAP testing for Windows is not required, in order to reduce library dependencies, the WIN-USE-NPCAP_LIB macro definition in the config. h file can be modified to 0.


## Command line parameters
```shell
      -v, --version             show version information and quit
      -h, --help                show this message and quit

	  Server or Client:
      -p, --port      #         server port to listen on/connect to
      -i, --interval  #         seconds between periodic bandwidth reports
      -u, --udp                 use UDP rather than TCP\n"
      -s, --save                log output to file,without this option,log output to console.
      -T, --Text                Text chat mode(default model)
      -P, --Perf                Performance test mode
      -F, --File                File transmission mode
      -B, --bind      <host>    bind to a specific interface

    Server specific:
      -s, --server              run in server mode

    Client specific:
      -c, --client    <host>    run in client mode, connecting to <host>
      -b, --bandwidth           Set the send rate, in units MB/s
      -l, --length              The size of each package
      -t, --time      #         time in seconds to transmit for (default 10 secs)
      -S, --src                 --File(-F) model,Source file path, include file name
      -D, --dst                 --File(-F) model,Purpose file save path,exclusive file name
      -n, --number              client bind port

    raw socket:
      -r, --raw                 run raw socket, only for -T and -P mode
      -I, --interface           local net card, only for -r
      -M, --dstmac              destination mac address, only for -r
```
