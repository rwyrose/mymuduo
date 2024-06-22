#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include<sys/epoll.h>

/*
epoll的使用
epoll_create
epoll_ctl
epoll_wait
*/

class Channel;

class EPollPoller:public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;   //override表示该函数是覆盖函数，该函数在基类是虚函数

    //重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs,ChannelList *aactiveChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;
private:
    static const int kInitEventListSize=16;

    //填写活跃的连接
    void fillActiveChannels(int numEvents,ChannelList *activeChannels) const;
    //更新channel通道
    void update(int operation,Channel *channel);

    using EventList=std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};