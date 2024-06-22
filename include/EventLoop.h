#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include<vector>
#include<atomic>
#include<functional>
#include<memory>
#include<mutex>

class Channel;
class Poller;

//事件循环类，主要包含了两个大模块  channel  poller(epoll的抽象)
class EventLoop : noncopyable
{
public:
    using Functor=std::function<void()>;

    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();

    Timestamp pollerReturnTime() const {return pollerReturTime_;}

    //在当前loop中执行cb
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    //用来唤醒loop所在的线程的
    void wakeup();

    //EventLoop的方法   =》 Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //判断EventLoop对象是否在自己的线程里面
    bool isInloopThread() const {return threadId_==CurrentThread::tid();}

private:
    using ChannelList=std::vector<Channel*>;

    void handleRead();    //wake up
    void doPendingFunctors();  //执行回调

    std::atomic_bool looping_;  //原子操作，底层通过CAS实现的
    std::atomic_bool quit_;  //标识退出loop循环

    const pid_t threadId_;  //记录当前loop所在线程的id

    Timestamp pollerReturTime_; //poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;//主要作用：当mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_;//标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; //存储loop需要执行的所有的回调操作
    std::mutex mutex_;//互斥锁，用来保护上面vector容器的线程安全操作
};