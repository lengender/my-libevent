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
extern struct event_base* evsignal_base;

/* Prototypes*/
static void event_queue_insert(struct event_base *, struct event*, int);
static void event_queue_remove(struct event_base *, struct event*, int);

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

    event_debug(
        "event_add: event : %p, %s%s%scall %p",
        ev,
        ev->ev_events & EV_READ ? "EV_READ " : " ",
        ev->ev_events & EV_WRITE ? "EV_WRITE " : " ",
        tv ? "EV_TIMEOUT " : " ",
        ev->ev_callback);

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

int 
event_del(struct event *ev){
    struct event_base *base;
    const struct eventop *evsel;
    void *evbase;

    event_debug("event_del: %p, callback %p", ev, ev->ev_callback);

    //ev_base为NULL, 表明ev没有被注册
    if(ev->ev_base == NULL)
        return -1;

    //取得ev注册的ev_base和eventop指针
    base = ev->ev_base;
    evsel = base->evsel;
    evbase = base->evbase;

    assert(!(ev->ev_flags & ~EVLIST_ALL));

    //将ev_callback调用次数设置为0， 
    //see if we are just active execting this event in a loop
    if(ev->ev_ncalls && ev->ev_pncalls){
        *ev->ev_pncalls = 0;
    }

    //从对应的链表中删除
    if(ev->ev_flags & EVLIST_TIMEOUT)
        event_queue_remove(base, ev, EVLIST_TIMEOUT);
    if(ev->ev_flags & EVLIST_ACTIVE)
        event_queue_remove(base, ev, EVLIST_ACTIVE);
    if(ev->ev_flags & EVLIST_INSERTED){
        event_queue_remove(base, ev, EVLIST_INSERTED);
        //EVLIST_INSTRTED表明是IO或signal事件，需要调用IO demultiplexer注销事件
        return (evsel->del(evbase, ev));
    }

    return 0;
}

int 
event_base_loop(struct event_base *base, int flags){
    const struct eventop *evsel = base->evsel;
    void *evbase = base->evbase;
    struct timeval tv;
    struct timeval *tv_p;
    int res, done;

    //清空时间缓存
    base->tv_cache.tv_sec = 0;

    //evsignal_base是全局变量，在处理signal时，用于指明signal所属的event_base实例
    if(base->sig.ev_signal_added)
        evsignal_base = base;

    done = 0;

    //事件主循环
    while(!done){
        //查看是否需要跳出循环，程序可以调用event_loopexit_cb()设置event_gotterm标记
        //调用event_base_loopbreak设置event_break标志
        if(base->event_gotterm){
            base->event_gotterm = 0;
            break;
        }

        if(base->event_break){
            base->event_break = 0;
            break;
        }

        //校正系统时间，如果系统使用的是非MONOTONIC时间，用户可能会向后调整了系统时间
        //在timeout_correct函数中，比较last wait time和当前事件，如果
        //当前时间 < last wait time
        //表明时间有问题，这需要更新timer_heap中所有定时事件的超时时间
        timeout_correct(base, &tv);
        
    }

}

void 
event_queue_insert(struct event_base *base, struct event* ev, int queue){
    //ev可能已经在激活列表中了，避免重复插入
    if(ev->ev_flags & queue){
        if(queue & EVLIST_ACTIVE)
            return;

        event_errx(1, "%s: %p(fd %d) already on queue %x", __func__,
                  ev, ev->ev_fd, queue);
    }

    if(~ev->ev_flags & EVLIST_INTERNAL)
        base->event_count++;

    ev->ev_flags |= queue;
    switch(queue){
        case EVLIST_INSERTRED:  //IO或signal事件，加入已注册事件链表
            TAILQ_INSERT_TAIL(&base->eventqueue, ev, ev_next);
            break;
        case EVLIST_ACTIVE:  //就绪事件，加入激活链表
            base->event_count_active++;
            TAILQ_INSERT_TAIL(base->activequeues[ev->ev_pri], ev, ev_active_next);
            break;
        case EVLIST_TIMEOUT:  //定时事件，加入堆
            min_heap_push(&base->timeheap, ev);
            break;
        default:
        event_errx(1, "%s: unknown queue %x", __func__, queue);
    }
}

void
event_queue_remove(struct event_base* base, struct event *ev, int queue)
{
    if(!(ev->ev_flags & queue))
        event_errx(1, "%s: %p (fd %d) not on queue %x", __func__, ev, ev->ev_fd, queue);

    if(~ev->ev_flags & EVLIST_INTERNAL)
        bae->event_count--;

    ev->ev_flags &= ~queue;
    switch(queue){
        case EVLIST_INSERTED:
            TAILQ_REMOVE(&base->eventqueue, ev, ev_next);
            break;
        case EVLIST_ACTIVE:
            base->event_count_active--;
            TAILQ_REMOVE(base->activequeues[ev->ev_pri], ev, ev_active_next);
            break;
        case EVLIST_TIMEOUT:
            min_heap_erase(&base->timheap, ev);
            break;
        default:
            event_errx(1, "%s: unknown queue %x", __func__, queue);
    }
}
