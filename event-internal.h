/*************************************************************************
	> File Name: event-internal.h
	> Author: 
	> Mail: 
	> Created Time: 2017年03月21日 星期二 16时37分00秒
 ************************************************************************/

#ifndef _EVENT-INTERNAL_H
#define _EVENT-INTERNAL_H
#include"event.h"

/*
 * 因为，libevent将系统提供的IO demultiplex机制统一封装成了一个eventop结构，
 * 因此，eventops[]包含了select,poll,kequeue和epoll等等其中的若干个全局实例对象
 * 也就是说，在libevent中，每个IO demultiplex机制的实现都必须提供这五个函数接口，
 * 来完成自身的初始化，销毁释放；对事件的注册/注销/分发
 *
 * 比如对于epoll, libevent实现了这5个对应的接口函数，并在初始化时将eventop的5个函数
 * 指针指向这5个函数，那么程序就可以使用epoll作为IO demultiplex机制了
 */
struct eventop{
    const char *name;
    void *(*init)(struct event_base *);    //初始化
    int (*add)(void *, struct event *);    //注册事件
    int (*del)(void *, struct event *);    //删除事件
    int (*dispatch)(struct event_base*, void *, struct timeval *); //事件分发
    void (*dealloc)(struct event_base *, void *);   //注销，释放资源

    //set if we need to reinitialize the event base
    int need_reinit;
};


struct event_base{
    /*
     * evsel 和evbase这两个字段可看做是类和静态函数的关系，比如添加事件时
     * 的调用行为evsel->add(evbase, ev),实际执行操作的是evbase;这相当于class::add(instance, ev),
     * instance就是class的一个对象实例。
     * evsel指向了全局变量static const struct eventop *eventops[]中的一个
     *
     * evbase实际上是一个eventop实例对象
     */
    const struct eventop *evsel;
    void *evbase;

    //总事件的个数
    int event_count;

    //活跃事件的个数
    int event_count_active;

    //处理完当前所有事件之后退出
    int event_getterm;

    //立即退出
    int event_break;

    //activequeues是一个二级指针，可看做是一个数组，其中的元素ativequeues[priority]是一个链表，
    //链表的每个节点指向一个优先级为priority的就绪事件event
    struct event_list **activequeues;
    int nactivequeues;

    //用来管理信号的结构体
    struct evsignal_info sig;

    //链表，保存了所有的注册事件event的指针
    struct event_list eventqueue;

    //管理定时事件的小根堆
    struct min_heap timeheap;
    
    //以下两个为libevent用于时间管理的变量
    struct time tv_cache;
    struct timeval event_tv;

};
#endif
