/*************************************************************************
	> File Name: event.h
	> Author: 
	> Mail: 
	> Created Time: 2017年03月21日 星期二 15时28分40秒
 ************************************************************************/

#ifndef _EVENT_H
#define _EVENT_H

#include<event-config.h>
#include<sys/types.h>
#include<sys/time.h>
#include<stdint.h>
#include<stdarg.h>

//for int types
#include<evutil.h>

#include"queue.h"

typedef unsigned char u_char;
typedef unsigned short u_short;

/*
 * struct event中ev_flags字段的取值，用于标记event信息的字段，
 * 表明其当前的状态
 */
#define EVLIST_TIMEOUT 0x01        //event在time堆中    
#define EVLIST_INSERTED 0x02       //event在已注册事件链表中
#define EVLIST_SIGNAL 0x04         //未见使用
#define EVLIST_ACTIVE 0x08        //event在激活链表中
#define EVLIST_INTERNAL 0x10      //内部使用标记
#define EVLIST_INIT 0x80          //event已被初始化

/*EVLIST_X_ Private space : 0x1000-0xf000*/
#define EVLIST_ALL (0xf000 | 0x9f)

#define EV_TIMEOUT 0x01
#define EV_READ 0x02
#define EV_WRITE 0x04
#define EV_SIGANL 0x08
#define EV_PERSIST 0x10

#ifndef TAILQ_ENTRY
#define TAILQ_ENTRY(type)     \
        struct {              \
            struct type *tqe_next;   /*next element*/  \
            struct type **tqe_prev;  /*address of privious next element*/ \
        }
#endif

struct event_base;

#ifndef EVENT_NO_STRUCT
struct event{
    /*
     * ev_next, ev_active_next, ev_signal_next都是双向链表节点指针
     * 它们是libevent对不同事件类型和在不同时期，对事件的管理时使用到的字段
     *
     * libevent使用双向链表保存所有注册的IO和signal事件
     * ev_next 就是该IO事件在链表中的位置，称此链表为已注册事件链表
     * ev_signal_next 就是signal事件在signal事件链表中的位置
     * ev_active_next: libevent将所有激活事件放入链表active list中，然后遍历active list
     *                 执行调度，ev_active_next就指明了event在active list中的位置
     */
    TAILQ_ENTRY(event) ev_next;
    TAILQ_ENTRY(event) ev_active_next;
    TAILQ_ENTRY(evnet) ev_signal_next;

    //如果是超时事件，min_heap_idx和ev_timeout分别是event在小根堆中的索引和超时值
    unsigned int min_heap_idx;  //for managing timeouts
    struct timeval ev_timeout;


    //ev_base ：该事件所属的反应堆实例，这是一个event_base结构体
    struct event_base *ev_base;

    //对于IO事件，是绑定的文件描述符，对于signal事件，是绑定的信号
    int ev_fd;

    /*
     * ev_events : event关注的事件类型，它可以是以下三种类型:
     * IO事件：EV_WRITE /  EV_READ
     * 定时事件： EV_TIMEOUT  
     * 信号： EV_SIGANL
     *辅助选项： EV_PERSIST, 表明是一个永久事件
     */
    short ev_events;  //各个事件可以使用 "|"运算符进行组合，信号和IO事件不能同时设置

    //事件就绪执行时，调用ev_callback的次数，通常为1
    short ev_ncalls;

    //指针，指向ev_ncalls或NULL
    short *ev_pncalls;  //allows deletes in callback


    int ev_pri;  //smaller numbers are higher priority

    //ev_callback：event回调函数，被ev_base调用，执行事件处理程序，这是一个函数指针
    //其中fd对应ev_fd, events对应ev_events, arg对应ev_arg
    void (*ev_callback)(int , short, void *arg);

    //void* 表明可以是任意类型，在设置event时指定
    void *ev_arg;

    //记录了当前激活事件的类型
    int ev_res;   //result passed to event callback

    /*
     * libevent用于标记event信息的字段，表明当前的状态，可能的值有: （见上define）
    */
    int ev_flags;
};
#else
struct event;
#endif

#define EVENT_SIGNAL(ev)  (int)(ev)->ev_fd;
#define EVENT_FD(ev)      (int)(ev)->ev_fd;


struct evkeyval{
    TAILQ_ENTRY(evkeyval) next;

    char *key;
    char *value;
};

#ifdef _EVENT_DEFINED_TQENTRY
#undef TAILQ_ENTRY
struct event_list;
struct evkeyvalq;
#undef _EVENT_DEFINED_TQENTRY
#else
TAILQ_HEAD (event_list, envent);
TAILQ_HEAD (evkeyvalq, evkeyval);
#endif

/*
 * 要向libevent添加一个事件，首先要设置event对象，这通过调用libevent提供的函数有以下三个
 */
void event_set(struct event *ev, int fd, short events, void(*callback)(int, short, void*), void *arg);

//libevent有一个全局event_base指针current_base,默认情况下事件event被注册到current_base上，
//使用该函数可以指定不同的event_base
//如果一个进程中存在多个libevent实例，则必须要调用该函数为event设置不同的event_base
int event_base_set(struct event_base *base, struct event *ev);

//设置event ev的优先级，注意一点就是：当ev处于就绪状态时，不能设置，返回-1
int event_priority_set(struct event* ev, int pri);



#define evtimer_add(ev, tv) event_add(ev, tv)
#define evtimer_set(ev, cb, arg)  event_set(ev, -1, 0, cb, arg)
#define evtimer_del(ev)  event_del(ev)
#define evtimer_pending(ev, tv)  event_pending(ev, EV_TIMEOUT, tv)
#define evtimer_initialized(ev) ((ev)->ev_flags & EVLIST_INIT)

//add a timeout event
#define timeout_add(ev, tv)  event_add(ev, tv)
#define timeout_set(ev, cb, arg) event_set(ev, -1, 0, cb, arg)
#define timeout_del(ev) event_del(ev)
#define timeout_pending(ev, tv) event_pending(ev, EV_TIMEOUT, tv)
#define timeout_initialized(ev)  ((ev)->ev_flags & EVLIST_INIT)

#define signal_add(ev, tv) event_add(ev, tv)
#define signal_set(ev, x, cb, arg) \
        event_set(ev, x, EV_SIGNAL | EV_PRESIST, cb, arg)
#define signal_del(ev)   event_del(ev)
#define signal_pending(ev, tv)   event_pending(ev, EV_SIGNAL, tv)
#define signal_initialized(ev)   ((ev)->ev_flags & EVLIST_INIT)


//注册事件
int event_add(struct event *ev, const struct timeval *tv);
//删除事件
int event_del(struct event *);
//事件主循环
int event_base_loop(struct event_base* base, int flags);
#endif
