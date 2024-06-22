# mymuduo

基于C++11的muduo网络库核心功能源码(无需依赖boost库)
开发环境：Centos7 VS Code
编译器：g++
编程语言：C++

# 项目说明

项目编译问题

sudo ./autobuild.sh，直接运行编译脚本即可

# 项目概述

## muduo网络库的reactor模型

在muduo网络库中，采用的是**reactor**模型，那么，什么是reactor模型呢？

```
Reactor： 即非阻塞同步网络模型，可以理解为，向内核去注册一个感兴趣的事件，事件来了去通知你，你去处理 Proactor： 即异步网络模型，可以理解为，向内核去注册一个感兴趣的事件及其处理handler，事件来了，内核去处理，完成之后告诉你
```

## muduo的设计

reactor模型在实际设计中大致是有以下几个部分：

- Event：事件
- Reactor： 反应堆
- Demultiplex：多路事件分发器
- EventHandler：事件处理器

在muduo中，其**调用关系**大致如下

- 将事件及其处理方法注册到reactor，reactor中存储了连接套接字connfd以及其感兴趣的事件event
- reactor向其所对应的demultiplex去注册相应的connfd+事件，启动反应堆
- 当demultiplex检测到connfd上有事件发生，就会返回相应事件
- reactor根据事件去调用eventhandler处理程序
