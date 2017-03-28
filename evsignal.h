/*************************************************************************
	> File Name: evsignal.h
	> Author: 
	> Mail: 
	> Created Time: 2017年03月24日 星期五 15时16分44秒
 ************************************************************************/

#ifndef _EVSIGNAL_H
#define _EVSIGNAL_H

typedef void (*ev_sighandler_t)(int);

//evsignal_info的初始化包括，创建socket pair，设置ev_signal事件(但并没有注册，
//而是等到信号注册时才检查并注册)，并将所有标记设置为零，初始化信号的注册事件
//链表指针等
struct evsignal_info{
    //为socket pair的读socket向event_base注册读事件时使用的event结构体
    struct event ev_signal;  

    //socket pair对，消息机制，fd[0]为写，fd[1]为读
    int ev_signal_pair[2];

    //记录ev_signal事件是否已经注册过了
    int ev_signal_added;

    //标记是否有信号发生过
    volatile sig_atomic_t evsignal_caught;

    //数组，evsigevents[signo]表示注册到信号signo的事件链表
    struct event_list evsigevents[NSIG];

    //具体记录每个信号的触发次数，evsigcaught[signo]是记录信号signo被触发的次数
    sig_atomic_t evsigcaught[NSIG];
#ifdef HAVE_SIGACTION
    //sh_old记录了原来的signal处理函数指针，当信号signo注册的event被清空时，需要重新设置
    //其处理函数
    struct sigaction **sh_old;
#else
    ev_sighandler_t **sh_old;
#endif
    int sh_old_max;
};


int evsignal_init(struct event_base*);
void evsignal_process(struct event_base *);
int evsignal_add(struct event *);
int evsignal_del(struct event*);
void evsignal_dealloc(struct event_base *);
#endif
