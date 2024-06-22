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

# muduo的设计

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

# muduo网络库核心代码模块

## channel：fd、events、revents、callbacks    

## 两种channel:  listenfd-->acceptorChannel      connfd-->connectionChannel

```c++
    EventLoop *loop_;  //事件循环
    const int fd_;  //fd,poller监听的对象
    int events_;  //注册fd感兴趣的事件
    int revents_; //poller返回的具体发生的事件

    //因为channel通道里面能够获知fd最终发生的具体的事件revents，所以他负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback  errorCallback_;
```

## Poller和EPollPoller     --Demultiplex(事件分发器)

```c++
	std::unordered_map<int,Channel*>channels_;
```

## EventLoop  --Reactor

​	wakeupFd_被封装成wakeupChannel_，主要作用：一个wakeupFd_是隶属于一个loop的，loo执行时驱动底层的时间是分发器（也即epoll_wait），没有事件发生时，loop线程阻塞于epoll_wait。 如果想唤醒某个loop线程，直接通过loop对象获取wakeup_fd,往wakeup_fd上写东西，loop就会被唤醒，因为每一个loop的wakeup_fd都被封装成wakeup_channel，注册在自己loop的事件分发器上

```c++
	ChannelList activeChannels_;

    std::unique_ptr<Poller> poller_;

    int wakeupFd_;//主要作用：当mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理
    std::unique_ptr<Channel> wakeupChannel_;
```

EventLoop管理者多个channel和一个poller，还有一个wakeup_fd，channel想把自己注册到poller上，或者在poller上修改自己感兴趣的事件，都是通过EventLoop获取poller对象，来向poller上设置的，同样，poller监测到socket上有相应的事件发生，通过EventLoop调用channel相应的fd所发生事件的回调函数

## thread和EventLoopThread

## EventLoopThreadPoll

getNextLoop()：通过轮询算法获取下一个subLoop      如果没有设置线程数量，返回的就是baseLoop

一个thread对应一个loop   one loop per thread

## Socket

## Acceptpor

主要封装了listenfd相关的操作，socket创建，bind绑定，listen，listen成功后就把listenfd打包成listenChannel，扔给baseLoop，监听相应事件

## Buffer

缓冲区：non-blocking的I/O都需要设置缓冲区。应用写数据-->缓冲区-->Tcp发送缓冲区-->send

  刚开始prependable是8个字节， readeridx     writeridx都是在8字节处                   prependable   readeridx     writeridx

## TcpConnection

一个连接成功的客户端对应一个TcpConnection    socket   channel   各种回调   发送和接收缓冲区

## TcpServer

Acceptor   EventLoopThreadPoll     ConnectionMap  connections_;

Acceptor 得到新用户，把新用户封装成TcpConnection，设置回调函数，通过EventLoopThreadPoll的getNextLoop()方法，把TcpConnection扔给相应的loop

