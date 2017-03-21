/*************************************************************************
	> File Name: queue.h
	> Author: 
	> Mail: 
	> Created Time: 2017年03月21日 星期二 17时03分28秒
 ************************************************************************/

#ifndef _QUEUE_H
#define _QUEUE_H

/*
 * Tail queue definitions
 */
#define TAILQ_HEAD(name, type)    \
        struct name {            \
                     struct type* tqh_first;  /*first element*/    \
                     struct type **tqh_last;  /* addr of last next element*/ \ 
        }

#define TAILQ_HEAD_INITIALIZER(head)  \
        { NULL, &(head).tqh_first }

#define TAILQ_ENTRY(type)   \
        struct {    \
                struct type *tqe_next;  /*next element*/   \
                struct type **tqe_prev;   /*address of previous next element*/ \

        }
#endif
