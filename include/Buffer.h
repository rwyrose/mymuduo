#pragma once

#include<vector>
#include<string>
#include<algorithm>

//网络库底层的缓冲区类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend=8;
    static const size_t kInitialSize=1024;

    //explicit不允许默认生成对象
    explicit Buffer(size_t initialSize=kInitialSize)
        :buffer_(kCheapPrepend+initialSize)
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend)
        {}

    size_t readableBytes() const
    {
        return writerIndex_-readerIndex_;
    }

    size_t writeableBytes() const
    {
        buffer_.size()-writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    //返回缓存区中可读数据的起始地址
    const char* peek() const
    {
        return begin()+readerIndex_;
    }

    //onMessage string  <-  Buffer
    void retrieve(size_t len)
    {
        if(len<readableBytes())
        {
            readerIndex_+=len;  //用户只读取了可读缓存区数据一部分，就是len，还剩下readerIndex_+=len  ->  writerIndex_
        }
        else   //len == readableBytes()
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_=writerIndex_=kCheapPrepend;
    }

    //把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllsString()
    {
        return retrieveAsString(readableBytes()); //应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);
        retrieve(len);  //上面一句把缓存区中可读数据，已经读取出来，这里肯定要把缓存区进行复位操作
        return result;
    }

    //buffer_.size  -  writerIndex_
    void ensureWriteableBytes(size_t len)
    {
        if(writeableBytes()<len)
        {
            makeSpace(len);  //扩容操作
        }
    }

    //把[data,data+len]内存上的数据，添加到writeable缓存区当中
    void append(const char *data,size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd,int* saveErrno);
    //通过fd发送数据
    ssize_t writeFd(int fd,int* saveErrno);

private:
    char* begin()
    {
        //it.operator*()
        return &*buffer_.begin();  //vector底层数组首元素地址，也就是数组的起始地址
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    void makeSpace(size_t len)
    {
        if(writeableBytes()+prependableBytes()<len+kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);
        }
        else
        {
            size_t readable=readableBytes(); //未读数据大小
            std::copy(begin()+readerIndex_,
                    begin()+writerIndex_,
                    begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readerIndex_+readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};