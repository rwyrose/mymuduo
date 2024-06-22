#include "Buffer.h"

#include <errno.h>
#include<sys/uio.h>
#include<unistd.h>


/*
 从fd上读取数据   Poller工作在LT模式
 Buffer缓存区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
*/
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65526]={0};  //栈上的内存空间  64k
    struct iovec vec[2];
    const size_t writable = writeableBytes();  //这是Buffer底层缓存区剩余的可写空间大小
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base=extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt=(writable<sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n<0)
    {
        *saveErrno=errno;
    }
    else if (n<=writable)   //buffer的可写缓存区已经够存储读出来的数据了
    {
        writerIndex_+=n;
    }
    else   //extrabuf里面也写入了数据
    {
        writerIndex_=buffer_.size();
        append(extrabuf,n-writable);  //writeIndex_开始写 n-writable 大小的数据
    }

    return n;
}

// 通过fd发送数据
ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n=::write(fd,peek(),readableBytes());
    if(n<0)
    {
        *saveErrno=errno;
    }
    return n;
}