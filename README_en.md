# TCP/UDP network testing tool implemented in C++

[![](https://img.shields.io/badge/license-MIT-green.svg)](https://github.com/wichue/nethello/blob/master/LICENSE)
[![](https://img.shields.io/badge/language-c++-red.svg)](https://en.cppreference.com/)
[![](https://img.shields.io/badge/platform-linux%20|%20windows-blue.svg)](https://github.com/wichue/nethello)
## original intention
The lightweight tcp / udp network test tool based on C + + 11 implementation includes common basic modules, event loop, network modules, etc., without other three-party library dependence, convenient compilation and deployment.
## Features
- Text chat, optional TCP/UDP protocol, test if the network is connected.
- Stress testing, optional TCP/UDP protocol, can set sending rate, packet length, testing duration, output interval, etc.
- File transfer, currently supports TCP protocol, allowing the client to send files to the server.

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

## 使用示例
- Text mode and stress testing default to TCP, support UDP (- u), file transfer only supports TCP.
### Text chat
```shell
./nethello -s -p 9090
./nethello -c 127.0.0.1 -p 9090
```
### Stress testing
```shell
./nethello -s -p 9090 -P
./nethello -c 127.0.0.1 -p 9090 -P
```
### File transfer
```shell
./nethello -s -p 9090 -F
./nethello -c 127.0.0.1 -p 9090 -F -S /root/gitcode/nethello/build/glibc-2.18.tar.gz -D /root/gitcode/nethello/build/file
```

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
      -P, --Press               Stress test mode
      -F, --File                File transmission mode

    Server specific:
      -s, --server              run in server mode
      -B, --bind      <host>    bind to a specific interface

    Client specific:
      -c, --client    <host>    run in client mode, connecting to <host>
      -b, --bandwidth           Set the send rate, in units MB/s
      -l, --length              The size of each package
      -t, --time      #         time in seconds to transmit for (default 10 secs)
      -S, --src                 --File(-F) model,Source file path, include file name
      -D, --dst                 --File(-F) model,Purpose file save path,exclusive file name
```
