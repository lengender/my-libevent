# my-libevent
### 目的<br>
深入剖析libevent源码，在学习过程中，自己造轮子<br>
在学习libevent框架的同时，了解底层基础数据结构和设计技巧，夯实基础，提高网络编程能力<br>
同时，也作为自己的开源项目学习，希望在找工作时候，可以用来吹牛逼，哈哈。。就是这么不要脸<br>
GO !!! <br>

### 开始<br>
* 3.21    完成事件event数据结构和相关接口函数,开始学习事件处理框架 event_base<br>

* 3.22    完成libevent关于事件主循环的分析，主要是event_base_loop函数的解析过程<br>

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

	以上为事件主循环的主要流程，libevent将Timer和Signal事件都统一到了系统I/O的demultiplex机制中<br>
	下一步，具体分析信号事件，定时器事件和I/O多路复用事件的处理<br>

* 3.24  完成libevent关于信号处理部分，它将signal事件统一到系统的I/O多路复用中。Siganl事件的出现对进程来说<br>
	是完全随机的，进程不能只是测试一个变量来判别是否发生了一个信号，而是告诉内核“在信号发生时，请执行如下的操作”。<br>
 	统一事件源的工作原理如下：假如用户要监听SIGINT信号，那么在实现的内部就对SIGINT这个函数设置捕捉函数。此外，<br>
	在实现的内部还要建立一条管道，并把这个管道加入到多路IO复用函数中。当SIGINT这个信号发生时，抓捕函数就会被调用。<br>
	而这个抓捕函数的工作就是往管道写入一个字符。此时，这个管道就变成是可读的，多路IO复用函数就检测到这个管道变成可读了。
	换句话说，就是多路IO复用函数检测到这个SIGINT信号发生了，这也就完成了对信号的监听工作。<br>
	Libevent
		
