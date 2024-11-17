简体中文 | [English](./README_en.md)

# C++实现的TCP/UDP网络测试工具

[![](https://img.shields.io/badge/license-MIT-green.svg)](https://github.com/wichue/nethello/blob/master/LICENSE)
[![](https://img.shields.io/badge/language-c++-red.svg)](https://en.cppreference.com/)
[![](https://img.shields.io/badge/platform-linux%20|%20windows-blue.svg)](https://github.com/wichue/nethello)
## 初衷
基于C++11实现的轻量级tcp/udp/raw网络测试工具，包含常用基础模块、事件循环、网络模块等，无其他三方库依赖，方便编译和部署。
## 项目特点
- tcp/udp客户端和服务端均可绑定IP和port。
- 文本聊天，可选tcp/udp协议，测试网络是否连通。
- 压力测试，可选tcp/udp协议，可设置发送速率，每包长度，测试时长，输出间隔等。
- 文件传输，目前支持tcp协议，由客户端发送文件至服务端。
- 原始套接字测试，raw socket 实现文本聊天和性能测试，暂时仅支持linux。

## 编译和安装
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
以VS2019为例：
- 创建新项目，空项目，右键源文件，添加-现有项，把源码文件添加进来。
- 右键项目名，属性，配置属性，C/C++，常规，附加包含目录，把依赖的头文件目录添加进来。
- 右键项目名，属性，配置属性，C/C++，预处理器，预处理器定义，添加预处理宏定义：
```shell
_CRT_SECURE_NO_WARNINGS
_WINSOCK_DEPRECATED_NO_WARNINGS
_CRT_NONSTDC_NO_DEPRECATE
```
- 右键项目名，生成即可。

## 使用示例
- 文本模式和压力测试默认tcp，支持udp(-u)，文件传输只支持tcp。
### 文本聊天
```shell
./nethello -s -p 9090
./nethello -c 127.0.0.1 -p 9090
```
- tcp文本聊天

![text_tcp](https://github.com/wichue/nethello/blob/master/doc/text_tcp.png)

- udp文本聊天

![text_udp](https://github.com/wichue/nethello/blob/master/doc/text_udp.png)
### 压力测试
```shell
./nethello -s -p 9090 -P
./nethello -c 127.0.0.1 -p 9090 -P
```
- tcp性能测试

![perf_tcp](https://github.com/wichue/nethello/blob/master/doc/perf_tcp.png)
- udp性能测试

![perf_udp](https://github.com/wichue/nethello/blob/master/doc/perf_udp.png)
### 文件传输
```shell
./nethello -s -p 9090 -F
./nethello -c 127.0.0.1 -p 9090 -F -S /root/gitcode/nethello/build/2GB.txt -D /root/gitcode/nethello/build/file
```
![file](https://github.com/wichue/nethello/blob/master/doc/file.png)
### 原始套接字
- raw socket文本聊天

```shell
# 相互发
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d
```
![raw_text](https://github.com/wichue/nethello/blob/master/doc/raw_text.png)
- raw socket性能测试

```shell
# 发送端
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d -P
# 接收端
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d -P -l 0
```
![raw_perf](https://github.com/wichue/nethello/blob/master/doc/raw_perf.png)
## 命令行参数
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
