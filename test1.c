/*************************************************************************
	> File Name: test1.c
	> Author: 
	> Mail: 
	> Created Time: 2017年03月21日 星期二 15时01分25秒
 ************************************************************************/

#include<stdio.h>
#include<event.h>

struct event ev;
struct timeval tv;

void time_cb(int fd, short event, void *argc){
    printf("timer wakeup.\n");
    event_add(&ev, &tv);
}

int main()
{
    struct event_base *base = event_init();

    tv.tv_sec = 10;
    tv.tv_usec = 0;
    evtimer_set(&ev, time_cb, NULL);
    event_add(&ev, &tv);
    event_base_dispatch(base);

    return 0;
}
