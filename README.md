# my_sylar

基于协程的C++高性能服务器框架

## 当前进度

| 日期       | 进度       |
| ---------- | ---------- |
| 2023.9.21  | 项目开始 |
| 2023.9.25  | 实现日志模块 |
| 2023.9.28  | 完成配置模块
| 2023.10.7  | 完成线程模块 |
| 2023.10.9  | 完成协程模块 |
| 2023.10.15 | 完成协程调度模块 |
| 2023.10.19 | 完成IO协程调度模块 |
| 2023.10.22 | 完成定时器模块，Hook模块，Address类，fdmanager类|
| 2023.10.25 | 完成socket模块 |
| 2023.10.28 | 完成序列化ByteArray|
| 2023.10.31 | 完成http协议封装 |
| 2023.11.5 | 实现HttpSession, HttpServer, HttpConnection |
| 2023.11.7| 完成守护进程模块|
| 2023.11.9| 完成环境模块|
| 2023.11.11| 完成Application|

> todo  chat_room

## Building my_sylar

### Install Dependencies

``` bash
sudo apt install build-essential wget bison  texinfo openssl net-tools doxygen 
sudo apt install ragel
```

第三方库

``` cpp
 boost yaml-cpp protobuf Zlib redis json-cpp 
```

只需要运行下面的脚本即可

* Ubuntu 22.04  gcc 11.4

```cmake
sudo bash ./script/install_thirdparty.sh
cmake -G Ninja -S . -B build 
cmake --build build
```
