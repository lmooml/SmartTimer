##1．SmartTimer能干什么？##


简单说来，**SmartTimer**是一个轻量级的基于STM32的定时器调度器，在单片机”裸跑”的情况下，可以很方便的实现异步编程。

它可以应用在对实时性要求没那么高的场合，比如说一个空气检测装置，每200ms收集一次甲醛数据，这个任务显然对实时性要求没那么高，如果时间上相差几毫秒，甚至几十毫秒也没关系，那么使用SmartTimer非常适合；而如果开发一个四轴飞行器，无论是对陀螺仪数据的采集、计算，以及对4个电机的控制，在时间的控制上都需要非常精确。那么这种场合下SmartTimer无法胜任，你需要一个带有抢占优先级机制的实时系统。

不同的场景，选择不同的工具和架构才是最合理的，**SmartTimer**只能做它力所能及的事情。

虽然**SmartTimer**是基于STM32开发的，但是它可以很方便的移植到其他的单片机上。


##2. SmartTimer的一般用法##

###2.1 Runlater。###

在单片机编程中，想实现在”xxx毫秒后调用xxx函数”的功能，一般有3种方法：

1. 用阻塞的，非精确的方式，就是用for(i=0;i<0xffff;i++);这种循环等待的方式，来非精确的延迟一段时间，然后再顺序执行下面的程序；
2. 利用硬件定时器实现异步的精确延时，把XXX函数在定时器中断里执行；
3. 同样是利用硬件定时器，但是只在定时器中断里设置标志位，在系统的主While循环中检测这个标志位，当检测到标志置位后，去运行XXX函数。

从理论上来说，以上3种方式中，第3种采用定时器设定标志位的方法最好。因为首先主程序不用阻塞，在等待的时间里，MCU完全可以去做其他的事情，其次在定时器中断里不用占用太多的时间，节约中断资源。但这种方式有个缺点，就是实现起来相对麻烦一些。因为如果你要有N个runlater的需求，那么就得设置N个标志位，还要考虑定时器的分配、设定。在程序主While循环里也会遍布N个查询标志位的if语句。如果N足够多，其实大于5个，就会比较头疼。这样会使主While循环看起来很乱。这样的实现不够简洁、优雅。

**SmartTimer**首先解决的就是这个问题，它可以优雅地延迟调用某函数。

###2.2        Runloop###

在定时器编程方面还有另一个典型需求，就是“每隔xxx毫秒运行一次XXX函数,一共运行XXX次”。这个实现起来和runlater差不多，就是加一个运行次数的技术标志。我就不再赘述了。还是那句话：

**SmartTimer**可以优雅的实现Runloop功能。

###2.3        Delay###
并不是说非阻塞就一定比阻塞好，因为在某些场景下，必须得用到阻塞，使单片机停下来等待某个事件。那么SmartTimer也可以提供这个功能。

##3. SmartTimer的高级用法##
所谓的高级用法，并不是说SmartTimer有隐藏模式，能开启黑科技。而是说，如果你能转变思路，举一反三地话，可以利用SmartTimer提供的简单功能实现更加优化、合理的系统结构。

传统的单片机裸跑一般采用状态机模式，就是在主While循环里设定一些标志位或是设定好程序进行的步骤，根据事件的进程来跳转程序。简单的说来，这是一种顺序执行的程序结构。其灵活性和实时性并不高，尤其是当需要处理的业务越来越多，越来越复杂时，状态机会臃肿不堪，一不留神（其实是一定以及肯定）就会深埋bug于其中，调试解决BUG时也会异常痛苦。


如果你能转换一下思路，不再把业务逻辑中各个模块的关系看成基于因果（顺序），而是基于时间，模块间如果需要确定次序可以采用标志位进行同步。那么恭喜你，你已经有了采用实时系统的思想，可以尝试使用RT-thread等操作系统来完成你的项目了。但是，使用操作系统有几个问题，第一是当单片机资源有限的时候，使用操作系统恐怕不太合适；第二是学习操作系统本身有一定的难度，至少你需要花费一定的时间；第三如果你的项目复杂度没有那么高，使用操作系统有点大材小用。

那么，请允许我没羞没臊的说一句，其实利用**SmartTimer**中的Runloop功能可以简单的实现基于时间的主程序框架。

##4.关于Demo##
与源码一起提供的，还有一个Demo程序。这个Demo比较简单，主要是为了测试SmartTimer的功能。Demo程序基本可以体现Runlater，Runloop，Delay功能。同时也能基本体现基于时间的编程思想（单片机裸跑程序框架）。

##5.SmartTimer的使用##
SmartTimer.h中声明的公开函数并不多，总共有8个：

```C
void stim_init ( void );

void stim_tick (void);

void stim_mainloop ( void );

int8_t stim_loop ( uint16_t delayms, void (*callback)(void), uint16_t times);

int8_t stim_runlater ( uint16_t delayms, void (*callback)(void));

void stim_delay ( uint16_t delayms);

void stim_kill_event(int8_t id);

void stim_remove_event(int8_t id);
```

下面我将逐一介绍
###5.1 必要的前提###
SmartTimer能够工作的必要条件是:
 
- **A.** 设置Systick的定时中断（也可以是其他的硬件定时器TIMx,我选择的是比较简单的Systick），我默认设置为1ms中断一次，使用者可以根据自己的情况来更改。Systick时钟的设置在stim_init函数中，该函数必须在主程序初始化阶段调用一次。
- **B.** 在定时器中断函数中调用stim_tick()；可以说，这个函数是**SmartTimer**的引擎，如A步骤所述，默认情况下，每1ms，定时器中断会调用一次stim_tick();
- **C.** 在主While循环中执行stim_mainloop()，这个函数主要有两个作用，一是执行定时结束后的回调函数；二是回收使用完毕的timer事件的资源。

###5.2 开始使用SmartTimer###
做好以上的搭建工作后，就可以开始使用**SmartTimer**了。



>  int8_t stim_runlater ( uint16_t delayms, void (*callback)(void))；

该函数接受两个参数，返回定时事件的id。参数delayms传入延迟多长时间，注意这里的单位是根据之前A步骤里，你设置的时间滴答来确定的(默认单位是1ms)；第二个参数是回调函数的函数指针，目前只支持没有参数，且无返回值的回调函数，未来会考虑加入带参数和返回值的回调。
举例：

``timer_runlater(100,ledflash); //100豪秒(100*1ms=100ms)后,执行void ledflash(void)函数``

如果在stim_init（）中，设置的时钟滴答为10ms执行一次，那么传入同样的参数，意义就会改变：

``timer_runlater(100,ledflash); //1秒(100*10ms=1000ms=1S)后,执行void ledflash(void)函数``



>  int8_t stim_loop ( uint16_t delayms, void (*callback)(void), uint16_t times);

这个函数的参数意义同runlater差不多，我就不详细说明了。 该函数接收3个参数，delayms为延迟时间，callback为回调函数指针，times是循环次数。 举例(以1ms滴答为例)：

``timer_runloop(50,ledflash,5); // 每50ms，执行一次ledflash()，总共执行5次``
``timer_runloop(80,ledflash, TIMER_LOOP_FOREVER); // 每80ms，执行一次ledflash(),无限循环。``




> void timer_delay ( uint16_t delayms);   //延迟xx ms

这个函数会阻塞主程序，并延迟一段时间。

> void stim_kill_event(int8_t id);
> 
> void stim_remove_event(int8_t id);

这两个函数，可以将之前设定的定时事件取消。比如之前用stim_loop无限循环了一个事件，当获取某个指令后，需要取消这个任务，则可以用这两个函数取消事件调度。这两个函数的区别是：

``void stim_kill_event(int8_t id); //直接取消事件，忽略未处理完成的调度任务。``
``void stim_remove_event(int8_t id);//将已经完成计时的调度任务处理完毕之后，再取消事件``

###5.3 注意事项###
SmartTimer可接受的Timer event数量是有上限的，这个上限由smarttimer.h中的宏定义

``#define        TIMEREVENT_MAX_SIZE      20``

来决定的。默认为20个，你可以根据实际情况增加或减少。但不可多于128个

