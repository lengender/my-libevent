# my-libevent
### 目的<br>
    深入剖析libevent源码，在学习过程中，自己造轮子
    在学习libevent框架的同时，了解底层基础数据结构和设计技巧，夯实基础，提高网络编程能力
    同时，也作为自己的开源项目学习，希望在找工作时候，可以用来吹牛逼，哈哈。。就是这么不要脸
    
    GO !!! 
    


### 开始<br>
####03.21  完成事件event数据结构和相关接口函数<br>
        开始学习事件处理框架-event_base

03.22  完成libevent关于事件主循环的分析，主要是event_base_loop函数的解析过程。
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
       下一步，具体分析信号事件，定时器事件和I/O多路复用事件的处理
 
