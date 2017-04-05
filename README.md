# my-libevent
### 目的<br>
深入剖析libevent源码，在学习过程中，自己造轮子。
在学习libevent框架的同时，了解底层基础数据结构和设计技巧，夯实基础，提高网络编程能力。
同时，也作为自己的开源项目学习。
GO !!!

### 开始<br>
* 3.21    完成事件event数据结构和相关接口函数,开始学习事件处理框架 event_base。

* 3.22    完成libevent关于事件主循环的分析，主要是event_base_loop函数的解析过程。

        1) 开始
        2) 调整系统时间与否
        3) 根据timer heap中的event的最小超时时间计算系统IO demultiplexer的最大等待时间
        4) 更新last wait time, 并清空time cache
        5) 调用系统I/O demultiplexer等待就绪I/O events
        6) 检查signal的激活标记，如果被设置，则检查激活signal event,并将event插入到激活链表中
        7) 将就绪的I/O event插入到激活链表中
        8) 检查heap中的timer events,将就绪的timer event从heap上删除，并插入到激活链表中
        9) 根据优先级处理激活链表中的就绪event,调用其回调函数执行事件处理(优先级越小越高)
        10) 结束

	以上为事件主循环的主要流程，libevent将Timer和Signal事件都统一到了系统I/O的demultiplex机制中
	下一步，具体分析信号事件，定时器事件和I/O多路复用事件的处理。

* 3.24   完成libevent关于信号处理部分。
    + 它将signal事件统一到系统的I/O多路复用中。Siganl事件的出现对进程来说是完全随机的，进程不能
        只是测试一个变量来判别是否发生了一个信号，而是告诉内核“在信号发生时，请执行如下的操作”。
    + 统一事件源的工作原理如下：
    假如用户要监听SIGINT信号，那么在实现的内部就对SIGINT这个函数设置捕捉函数。此外，在实现的内部
    还要建立一条管道，并把这个管道加入到多路IO复用函数中。当SIGINT这个信号发生时，抓捕函数就会被
    调用。而这个抓捕函数的工作就是往管道写入一个字符。此时，这个管道就变成是可读的，多路IO复用函
    数就检测到这个管道变成可读了。换句话说，就是多路IO复用函数检测到这个SIGINT信号发生了，这也就
    完成了对信号的监听工作。
    + 具体实现如下:

            1) 创建一个管道(Libevent实际上使用的是socketpair)
            2) 为这个socketpair的一个读端创建一个event，并将之加入到多路IO复用函数的监听之中
            3) 设置信号捕抓函数
            4) 有信号发生，就往socketpair写入一个字节 
    信号处理过程已完成，下一步，完成定时器处理部分。
    
* 3.26 完成集成定时器部分。
    + Libevent 将Timer时间融合到系统I/O多路复用的机制中，因为系统的IO机制像select()和epoll_wait()
        都有运行程序指定一个最大等待时间（也称为最大超时时间）timeout,即使没有IO事件发生，它们也
        能够保证在timeout时间内返回。
    + 根据所有Timer事件的最小超时时间来设置I/O的timeout时间；当系统I/O返回时，再激活所有就绪的Timer
        事件就可以了。这样就将Timer事件完美融合到了IO机制中了。
    + 使用最小堆管理Timer事件，其key值就是事件的超时时间。向堆中插入、删除元素的复杂度都是O(lgN),N为
        堆中元素的个数，而获取最小key值的复杂度为O(1)。堆是一个完全二叉树，基本存储方式是一个数组。

* 3.28 I/O demultiple机制
    + Libevent支持多种 I/O多路复用技术的关键在于结构体eventop.
        ```cpp
        struct eventop{
            const char* name;
            void *(*init)(struct event_base *);  //初始化
            int (*add)(void *, struct event *);  //注册事件
            int (*del)(void *, struct event *);  //删除事件
            int (*dispatch)(struct event_base *, void *, struct timeval *); /事件分发
            void (*dealloc)(struct event_base *, void *);  //注销，释放资源
            //set if we need to reinitialize the event base
            int need_reinit;
        };
        ```
        在libevent中，每种I/O demultiplex机制的实现都必须提供这五个函数接口。
    + epoll 实现
        * epoll.c中，eventop对象epollops定义如下：
            ```cpp
            const struct eventop epollops = {
                "epoll",
                epoll_init,
                epoll_add,
                epoll_del,
                epoll_dispatch,
                epoll_dealloc,
                1  //need reinit
            };
            ```
        * 具体实现见epoll.c中
        * epoll 函数相当重要！！！
    


        
       
    


		
