#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<memory>

//防止一个线程创建多个EventLoop对象   thread_local
__thread EventLoop *t_loopInThisThread=nullptr;

//定义默认的poller  IO复用接口的超时时间
const int kPollTimeMs=10000;

//创建wakeupfd，用来notify唤醒subreactor处理新来的Channel
int createEventfd()
{
    int evtfd=::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n",errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this,wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d",this,threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another Eventloop %p exists in this thread %d \n",t_loopInThisThread,threadId_);

    }
    else
    {
        t_loopInThisThread=this;
    }
    
    //设置wakeupFd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    //每一个EventLoop都将监听wakeupChannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

// 开启事件循环
void EventLoop::loop()
{
    looping_=true;
    quit_=false;
    LOG_INFO("EventLoop %p start looping \n",this);
    while(!quit_)
    {
        activeChannels_.clear();
        pollerReturTime_=poller_->poll(kPollTimeMs,&activeChannels_);
        for(Channel *channel:activeChannels_)
        {
            //Poller监听哪些channel发生事件了，然后上报EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollerReturTime_);
        }
        //执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping \n",this);
}

// 退出事件循环   1、loop在自己的线程中调用quit  2、在非loop的线程中，调用loop的quit
void EventLoop::quit()
{
    quit_=true;

    if(!isInloopThread()) //如果是在其他线程中，调用的quit   在一个subloop（worker）中，调用了mainloop（I/O）的quit
    {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if(isInloopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}
// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock_(mutex_);
        pendingFunctors_.emplace_back(cb);  //push_back是拷贝构造；emplace_back是直接构造（也就是默认构造）
    }
    if(!isInloopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

//每一个subReactor监听一个wakeupChannel，mainReactor通过给subReactor  write消息

void EventLoop::handleRead()
{
    uint64_t one=1;
    ssize_t n= read(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8",n);
    }
}

// 用来唤醒loop所在的线程的   向wakeupfd_写一个数据
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one,sizeof one);
    if(n!=sizeof one)
    {
        LOG_ERROR("Eventloop::wakeup() write %lu bytes instead of 8 \n",n);
    }
}

// EventLoop的方法   =》 Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    poller_->hasChannel(channel);
}

 // 执行回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_=true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor &Functor:functors)
    {
        Functor();   //执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_=false;
}