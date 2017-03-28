/*************************************************************************
	> File Name: select.c
	> Author: 
	> Mail: 
	> Created Time: 2017年03月28日 星期二 10时07分32秒
 ************************************************************************/

#include<stdio.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/select.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<assert.h>

#include"event.h"
#include"evutil.h"
#include"event-internal.h"
#include"evsignal.h"
#include"log.h"


#ifndef howmany
#define howmany(x, y) (((x) + (y) - 1) / (y))
#endif

#ifndef _EVENT_HAVE_FD_MASK
#undef NFDBITS
#define NFDBITS (sizeof(long) * 8)
typedef unsigned long fd_mask;
#endif


struct selectop{
    int event_fds;
    int event_fdsz;
    fd_set *event_readset_in;
    fd_set *event_writeset_in;
    fd_set *event_readset_out;
    fd_set *event_writeset_out;
    struct event **event_r_by_fd;
    struct event **event_w_by_fd;
};

static void *select_init(struct event_base *);
static int select_add(void *, struct event *);
static int select_del(void *, struct event *);
static int select_dispatch(struct event_base *, void *, struct timeval *);
static void select_dealloc(struct event_base*, void *);


const struct eventop selectops = {
    "select",
    select_init,
    select_add,
    select_del,
    select_dispatch,
    select_dealloc,
    0
};

static int select_resize(struct selectop *sop, int fdsz);

static void *
select_init(struct event_base* base){
    struct selectop *sop;

    //disable select when this environment variable is set
    if(evutil_getenv("EVENT_NOSELECT"))
        return NULL;

    if(!(sop = calloc(1, sizeof(struct selectop))))
        return NULL;

    select_resize(sop, howmany(32 + 1, NFDBITS) * sizeof(fd_mask));

    evsignal_init(base);

    return sop;
}

#ifdef CHECK_INVARIANTS
static void 
check_selectop(struct selectop *sop){
    int i;
    for(i = 0; i <= sop->event_fds; ++i){
        if(FD_ISSET(i, sop->event_readset_in)){
            assert(sop->event_r_by_fd[i]);
            assert(sop->event_r_by_fd[i]->ev_events & EV_READ);
            assert(sop->event_r_by_fd[i]->ev_fd == i);
        }
        else{
            assert(!sop->event_r_by_fd[i]);
        }

        if(FD_ISSET(i, sop->event_writeset_in)){
            assert(sop->event_w_by_fd[i]);
            assert(sop->event_w_by_fd[i]->ev_events & EV_WTIRE);
            assert(sop->event_w_by_fd[i]->ev_fd == i);
        }
        else{
            assert(!sop->event_w_by_fd[i]);
        }
    }
}
#else
#define check_selectop(sopo) do{ (void)sop; }while(0)
#endif

static int 
select_dispatch(struct event_base *base, void *arg, struct timeval *tv){
    int res, i, j;
    struct selectop *sop = arg;

    check_selectop(sop);

    memcpy(sop->event_readset_out, sop->event_readset_in, sop->event_fdsz);
    memcpy(sop->event_writeset_out, sop->event_writeset_in, sop->event_fdsz);

    res = select(sop->event_fds + 1, sop->event_readset_out, sop->event_writeset_out,
                NULL, tv);

    check_selectop(sop);

    if(res == -1){
        if(errno != EINTR){
            event_warn("select");
            return -1;
        }

        evsignal_process(base);
        return 0;
    }
    else if(base->sig.evsignal_caught){
        evsignal_process(base);
    }e

    event_debug("%s: select reports %d", __func__, res);

    check_selectop(sop);
    i = random() % (sop->event_fds + 1);
    for(j = 0; j <= sop->event_fds; ++j){
        struct event *r_ev = NULL, *w_ev = NULL;
        if(++i >= sop->event_fds + 1)
            i = 0;

        res = 0;
        if(FD_ISSET(i, sop->event_readset_out)){
            r_ev = sop->event_r_by_fd[i];
            res |= EV_READ;
        }

        if(FD_ISSET(i, sop->event_writeset_out)){
            w_ev = sop->event_w_by_fd[i];
            res |= EV_WTIRE;
        }

        if(r_ev && (res & r_ev->ev_events)){
            event_active(r_ev, res & r_ev->ev_events, 1);
        }

        if(w_ev && w_ev != r_ev && (res & w_ev->ev_events))
           event_active(w_ev, res & w_ev->ev_events, 1);
    }

    check_selectop(sop);
    return 0;
}


