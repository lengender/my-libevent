/*************************************************************************
	> File Name: event.c
	> Author: 
	> Mail: 
	> Created Time: 2017年03月21日 星期二 16时23分48秒
 ************************************************************************/

#include<stdio.h>
#include<sys/types.h>
#include<sys/time.h>
#include<sys/queue.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<signal.h>
#include<string.h>
#include<assert.h>
#include<time.h>

#include"event.h"

/*
 * Global state
 */
struct event_base *current_base = NULL;


void
event_set(struct event *ev, int fd, short events,void (*callback)(int, short, void*), void *arg){
    
    //take the current base
    ev->ev_base = current_base;

    ev->ev_callback = callback;
    ev->ev_arg = arg;
    ev->ev_fd = fd;
    ev->ev_events = events;
    ev->ev_res = 0;
    ev->ev_flags = EVLIST_INIT;
    ev->ev_ncalls = 0;
    ev->ev_pncalls = NULL;

    min_head_elem_init(ev);

    //by defult, we put events into the middle priority
    if(current_base)
        ev->ev_pri = current_base->nactivequeues/2;
}


int
event_base_set(struct event_base *base, struct event *ev){
    if(ev->ev_flags != EVLIST_INIT)
        return -1;

    ev->ev_base = base;
    ev->ev_pri = base->nactivequeues / 2;

    return 0;
}

int 
event_priority_set(struct event *ev, int pri){
    if(ev->ev_flags & EVLIST_ACTIVE)
        return -1;

    if(pri < 0 || pri >= ev->ev_base->nactivequeues)
        return -1;

    ev->ev_pri = pri;

    return 0;
}


int 
event_add(struct event *ev, const struct timeval *tv){
    struct event_base *base = ev->ev_base;
    const struct eventop *evsel = base->evsel;
    void *evbase = base->evbase;
    int res = 0;

    event_debug((
        "event_add: event : %p, %s%s%scall %p",
        ev,
        ev->ev_events & EV_READ ? "EV_READ " : " ",
        ev->ev_events & EV_WRITE ? "EV_WRITE " : " ",
        tv ? "EV_TIMEOUT " : " ",
        ev->ev_callback));

    assert(!(ev->ev_flags & ~EVLIST_ALL));

    /*
     * 新的timer事件，调用timer heap接口在堆上预留一个位置
     * 注：这样能保证该操作的原子性
     * 向系统IO机制注册可能会失败，而当在堆上预留成功后，定时事件的添加肯定不会失败
     * 而预留位置的可能结果是堆扩充，但是内部元素并不会改变
     */
    if(tv != NULL && !(ev->ev_flags & EVLIST_TIMEOUT)){
        if(min_heap_reverse(&base->timeheap,
                           1 + min_heap_size(&base->timeheap)) == mZ
    }

    //如果事件ev不在已注册或激活链表中，则调用evbase注册事件
    if((ev->ev_events & (EV_READ | EV_WRITE | EV_SIGNAL)) &7
       !(ev->ev_flags & (EVLIST_INSERTED | EVLIST_ACTIVE))){
        res = evsel->add(evbase, ev);
        if(res != -1)  //注册成功，插入event到已注册链表中
           event_queue_insert(base, ev, EVLIST_INSERTED);
    }

    //准备添加定时事件
    if(res != -1 && tv != NULL){
        struct timeval now;

        //EVLIST_TIMEOUT表明event已经在定时堆中了，删除旧的
        if(ev->ev_flag & EVLIST_TIMEOUT)
            event_queue_remove(base, ev, EVLIST_TIMEOUT);

        //如果事件已经是就绪状态则从激活链表中删除
        if((ev->ev_flags & EVLIST_ACTIVE) && (ev->ev_res & EV_TIMEOUT)){
            //将ev_callback调用次数设置为0
            if(ev->ev_ncalls && ev->ev_pncalls){
                *ev->ev_pncalls = 0;
            }

            event_queue_remove(base, ev, EVLIST_ACTIVE);
        }

        //计算时间，并插入到timer小根堆中
        gettime(base, &now);

        evutil_timeradd(&now, tv, &ev->ev_timeout);
        event_queue_insert(base, ev, EVLIST_TIMEOUT);
    }

    return res;
}
