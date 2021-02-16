一、inux系统产生了下面五种网络模式的方案。
- 阻塞 I/O（blocking IO）：在IO执行的两个阶段都被block了
- 非阻塞 I/O（nonblocking IO）：用户进程需要不断的主动询问kernel数据好了没有
- I/O 多路复用（ IO multiplexing）：单个进程可以同时处理多个网络连接的I/O。
- 信号驱动 I/O（ signal driven IO）
- 异步 I/O（asynchronous IO）：一切完成后，kernel会给用户进程发送一个signal，告诉它read操作完成了。

二、select
int select (int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

##select 函数监视的文件描述符分3类，分别是writefds、readfds、和exceptfds。
##调用后select函数会阻塞，直到有描述符就绪（有数据可读、可写、或者有except），或者超时（timeout指定等待时间，如果立即返回设为null即可），函数返回。
##当select函数返回后，可以通过遍历fdset，来找到就绪的描述符。
##select目前几乎在所有的平台上支持，其良好跨平台支持也是它的一个优点。
##select的一个缺点在于单个进程能够监视的文件描述符的数量存在最大限制，在Linux上一般为1024，可以通过修改宏定义甚至重新编译内核的方式提升这一限制，但是这样也会造成效率的降低。

二、poll
int poll (struct pollfd *fds, unsigned int nfds, int timeout);
##不同与select使用三个位图来表示三个fdset的方式，poll使用一个pollfd的指针实现。
struct pollfd {
    int fd; /* file descriptor */
    short events; /* requested events to watch */
    short revents; /* returned events witnessed */
};
##pollfd结构包含了要监视的event和发生的event，不再使用select“参数-值”传递的方式。
##同时，pollfd并没有最大数量限制（但是数量过大后性能也是会下降）。
##和select函数一样，poll返回后，需要轮询pollfd来获取就绪的描述符。(select和poll都需要在返回后，通过遍历文件描述符来获取已经就绪的socket)
##事实上，同时连接的大量客户端在一时刻可能只有很少的处于就绪状态，因此随着监视的描述符数量的增长，其效率也会线性下降。

三、epoll
1.创建epoll句柄：
int epoll_create(int size)；
##size用来告诉内核这个监听的数目一共有多大，这个参数不同于select()中的第一个参数，给出最大监听的fd+1的值，
##参数size并不是限制了epoll所能监听的描述符最大个数，只是对内核初始分配内部数据结构的一个建议。
##当创建好epoll句柄后，它就会占用一个fd值，在linux下如果查看/proc/进程id/fd/，是能够看到这个fd的，所以在使用完epoll后，必须调用close()关闭，否则可能导致fd被耗尽。

2.对指定描述符fd执行op操作：
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)；
##epfd——epoll_create的返回值，epoll文件句柄
##op——表示op操作，用三个宏来表示：添加EPOLL_CTL_ADD，删除EPOLL_CTL_DEL，修改EPOLL_CTL_MOD。分别添加、删除和修改对fd的监听事件。
##fd——是需要监听的fd（文件描述符）
##epoll_event：是告诉内核需要监听什么事，struct epoll_event结构如下：
struct epoll_event {
  __uint32_t events;  /* Epoll events */
  epoll_data_t data;  /* User data variable */
};
##events可以是以下几个宏的集合：
  EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
  EPOLLOUT：表示对应的文件描述符可以写；
  EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
  EPOLLERR：表示对应的文件描述符发生错误；
  EPOLLHUP：表示对应的文件描述符被挂断；
  EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
  EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里

3.等待I/O事件
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
##最多返回maxevents个事件。
##参数events用来从内核得到事件的集合，maxevents告之内核这个events有多大，这个maxevents的值不能大于创建epoll_create()时的size。
##参数timeout是超时时间（毫秒，0会立即返回，-1将不确定，也有说法说是永久阻塞）。
##该函数返回需要处理的事件数目，如返回0表示已超时。

4.工作模式
epoll对文件描述符的操作有两种模式：LT（level trigger）和ET（edge trigger）。LT模式是默认模式，LT模式与ET模式的区别如下：
（1）LT模式：当epoll_wait检测到描述符事件发生并将此事件通知应用程序，应用程序可以不立即处理该事件。下次调用epoll_wait时，会再次响应应用程序并通知此事件。
（2）ET模式：当epoll_wait检测到描述符事件发生并将此事件通知应用程序，应用程序必须立即处理该事件。如果不处理，下次调用epoll_wait时，不会再次响应应用程序并通知此事件。
##在这种模式下，当描述符从未就绪变为就绪时，内核通过epoll告诉你。
##ET模式在很大程度上减少了epoll事件被重复触发的次数，因此效率要比LT模式高。
##epoll工作在ET模式的时候，必须使用非阻塞socket，以避免由于一个文件句柄的阻塞读/阻塞写操作把处理多个文件描述符的任务饿死。
