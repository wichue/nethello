简体中文 | [English](./README_en.md)

# C++实现的TCP/UDP网络测试工具

[![](https://img.shields.io/badge/license-MIT-green.svg)](https://github.com/wichue/nethello/blob/master/LICENSE)
[![](https://img.shields.io/badge/language-c++-red.svg)](https://en.cppreference.com/)
[![](https://img.shields.io/badge/platform-linux%20|%20windows-blue.svg)](https://github.com/wichue/nethello)
## 初衷
基于C++11实现的轻量级tcp/udp/raw/npcap网络测试工具，包含常用基础模块、事件循环、网络模块等，无其他三方库依赖，方便编译和部署。
## 项目特点
- tcp/udp客户端和服务端均可绑定IP和port。
- 文本聊天，可选tcp/udp/raw协议，测试网络是否连通。
- 压力测试，可选tcp/udp/raw协议，可设置发送速率，每包长度，测试时长，输出间隔等。
- 文件传输，支持tcp和raw+kcp实现，由客户端发送文件至服务端。
- 原始套接字测试，实现文本聊天、性能测试和文件传输，linux使用rawsocket，windows使用npcap，支持linux和windows相互发。

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
### 原始套接字raw和npcap
- 文本聊天

```shell
# 相互发
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d
```
![raw_text](https://github.com/wichue/nethello/blob/master/doc/raw_text.png)
- 性能测试

```shell
# 接收端
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d -P -l 0
# 发送端
./nethello -r -I eth0 -M 00:16:3e:3c:07:9d -P
```
![raw_perf](https://github.com/wichue/nethello/blob/master/doc/raw_perf.png)

- 文件传输
```shell
# 接收端
./nethello -r -s -F -I eth0
# 发送端
./nethello -r -c 123 -F -I eth0 -S /root/gitcode/nethello/build/500M.txt -D /root/gitcode/nethello/build/file
```

## windows原始套结字npcap配置说明
https://npcap.com/#download
下载：npcap-sdk-1.13.zip，解压缩后获取Lib和Include文件夹。

vs配置
```
VS-属性-C/C++-附加包含目录，把...\Include目录添加进去。
VS-属性-调试-环境，添加 PATH=...\Lib;%PATH%
VS-属性-连接器-常规-附加库目录，添加 ...\Lib
...用完整目录替代。
```
- windows的-I选项识别的是网卡描述，按字段模糊匹配即可，比如USB网卡包含'USB'字段，-I USB即可识别该网卡。
- 如果提示缺少Packet.dll和wpcap.dll，拷贝适合当前操作系统环境的库到可执行文件目录。
- 如果不需要windows的npcap测试，为了减少库依赖，可以修改config.h文件WIN_USE_NPCAP_LIB宏定义为0。

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
      -r, --raw                 run raw socket
      -I, --interface           local net card, only for -r
      -M, --dstmac              destination mac address, only for -r
```
