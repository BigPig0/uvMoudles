# uvMoudles
基于libUV实现一些常用的模块封装
https://github.com/BigPig0/uvMoudles.git
https://gitee.com/ztwlla/uvMoudles.git

#### 功能模块
* uvIPC [c]
进程间通讯，使用pipe实现，server和client各有一个接口

* uvNetPlus [c++]
封装常用的网络接口，已实现tcpserver、tcpclient、tcpconnectpool、httpserver、httpclient。
计划增加websocket、tls、ftp、rtsp、rtmp等网络协议

#### 计划中的模块
* uvLog
仿照流行的log4j的api实现一个日志模块

* uvPM
进程管理模块

#### 第三方库
* libUV: https://github.com/libuv/libuv.git
* libcstl: https://github.com/activesys/libcstl.git
* 
