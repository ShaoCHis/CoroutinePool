#include "coroutine.hpp"

using namespace std::placeholders;

/**
 * 初始化ucp结构体，将当前的上下文保存到ucp中
 * int getcontext(ucontext_t *ucp);
 *
 *
 * 设置当前的上下文为ucp，setcontext的上下文ucp应该通过getcontext或者makecontext取得，如果调用成功则不返回。
 * 如果上下文是通过调用getcontext()取得,程序会继续执行这个调用。
 * 如果上下文是通过调用makecontext取得,程序会调用makecontext函数的第二个参数指向的函数，
 *          如果func函数返回,则恢复makecontext第一个参数指向的上下文第一个参数指向的上下文context_t中指向的uc_link.如果uc_link为NULL,则线程退出。
 * int setcontext(const ucontext_t *ucp);
 *
 * makecontext修改通过getcontext取得的上下文ucp(这意味着调用makecontext前必须先调用getcontext)。
 *      然后给该上下文指定一个栈空间ucp->stack，设置后继的上下文ucp->uc_link.
 * 上下文通过setcontext或者swapcontext激活后，执行func函数，argc为func的参数个数，后面是func的参数序列。
 *      当func执行返回后，后继的上下文被激活，如果继承上下文为NULL时，线程退出
 * void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);
 *
 *
 * 将当前上下文保存在 oucp中，切换到 ucp中
 * int swapcontext(ucontext_t *oucp, const ucontext_t *ucp);
 *
 */

// struct sockaddr_in remote_addr;
// socklen_t len = sizeof(remote_addr);
// int client_fd = accept(listenfd_, (struct sockaddr *)&remote_addr, &len);
// if (client_fd < 0)
// {
//     std::cerr << "accept error" << std::endl;
//     continue;
// }
// std::cout << "client connection from " << inet_ntoa(remote_addr.sin_addr) << ":" << ntohs(remote_addr.sin_port)
//             << "   client fd =" <s< client_fd << std::endl;
// struct epoll_event ev;
// ev.events = EPOLLIN;
// ev.data.fd = client_fd;
// // 加入epoll
// epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);
// // 设置非阻塞
// fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFD, 0) | O_NONBLOCK);
// send(client_fd, "Welcome to My server", 21, 0);

// char buffer[1024];
// int len;
// if ((len = recv(events_[i].data.fd, buffer, sizeof(buffer), 0)) > 0)
// {
//     send(events_[i].data.fd, "Welcome to My server\n", 21, 0);
//     printf("%s fd %d \n", buffer, events_[i].data.fd);
// }
// else
// {
//     printf("client offline with: "
//             "clientfd = %d \n",
//             events_[i].data.fd);
//     epoll_ctl(epoll_fd_,EPOLL_CTL_DEL,listenfd_,NULL);
//     close(events_[i].data.fd);
// }

//////////////Coroutine相关实现
// 初始化时，协程中没有任务信息
Coroutine::Coroutine(int number)
{
    this->number_ = number;
    // 第一个int 为 事件connectionfd ，第二个int为 监听listenfd
    ctx::execution_context<int, int> source([&](ctx::execution_context<int, int> sink, int clientfd, int listenfd) mutable
                                            {
        for(;;){
            std::cout << "处理连接 ：" << clientfd << " listenfd:" << listenfd << std::endl;
            this->task_(clientfd,listenfd);
            //返回main（在这儿即是pool线程）线程
            //这里需要接收sink 传入的clientfd和listenfd对象，否则clienfd和listenfd不会更新
            std::tie(sink,clientfd,listenfd)= sink(clientfd,listenfd);
        } 
        return sink; });
    this->coroutineCtx_ = std::move(source);
}

// 设置协程执行的任务---->接收可读和可写两种任务
void Coroutine::setTask(Task task)
{
    std::cout << "任务地址：：" << &task << std::endl;
    this->task_ = task;
}

// 协程继续执行
void Coroutine::resum(int clientfd, int listenfd)
{
    std::cout << "resum coroutine ctx" <<clientfd << listenfd<< std::endl;
    this->coroutineCtx_ = std::move(std::get<0>(this->coroutineCtx_(clientfd, listenfd)));
    std::cout << "yield coroutine ctx" <<clientfd << listenfd<< std::endl;
}

// 获取协程编号
int Coroutine::getNumber()
{
    return this->number_;
}

///////////////CoroutinePool相关实现

// 获取单例
CoroutinePool *CoroutinePool::getInstance()
{
    // 单例对象
    static CoroutinePool coroutinePool_;
    return &coroutinePool_;
}

// 析构函数
CoroutinePool::~CoroutinePool()
{
    while (!coroutines_.empty())
    {
        Coroutine *cor = coroutines_.front();
        coroutines_.pop();
        delete cor;
    }
    close(epoll_fd_);
    close(listenfd_);
};

// 协程池启动函数
void CoroutinePool::start(uint32_t coroutineCnt)
{
    coroutineCnt_ = coroutineCnt;
    for (int i = 0; i < coroutineCnt; i++)
    {
        coroutines_.push(new Coroutine(i));
    }

    // 监听
    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serveraddr;
    // 本地协议 IPv4
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port_);

    if (-1 == bind(listenfd_, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr)))
    {
        std::cerr << "bind fail.(绑定ip 和 端口失败)" << std::endl;
        return;
    }

    // 将listenfd设置为异步读写
    int flags = fcntl(listenfd_, F_GETFL, 0);
    if (fcntl(listenfd_, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        std::cerr << "set nonblocking fail!" << std::endl;
        return;
    }

    // 开启监听
    if (listen(listenfd_, 10) < 0)
    {
        std::cerr << "listen error!" << std::endl;
    }
    else
    {
        std::cout << "start the socket:" << inet_ntoa(serveraddr.sin_addr) << ":" << ntohs(serveraddr.sin_port);
    }

    // 创建epoll
    epoll_fd_ = epoll_create(1);
    if (epoll_fd_ == -1)
    {
        std::cerr << "create epoll failed!The CoroutinePool create failed!" << std::endl;
        return;
    }

    struct epoll_event ev;
    ev.data.fd = listenfd_;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listenfd_, &ev);
    std::cout << "listen fd = " << listenfd_ << std::endl;
    // 开始处理epoll事件
    while (1)
    {
        int ready = epoll_wait(epoll_fd_, events_, 1024, -1);
        // 拿到epoll事件就绪数量
        std::cout << "queue size:" << coroutines_.size() << std::endl;
        for (int i = 0; i < ready; i++)
        {
            int connfd = events_[i].data.fd;
            std::cout << "connfd:" << connfd << std::endl;
            // 从队列中拿取一个协程,
            // 提供析构函数为 将协程重新入队
            std::shared_ptr<Coroutine> sp(coroutines_.front(), [&](Coroutine *cor)
                                          { coroutines_.push(cor); });
            coroutines_.pop();
            std::cout << "corountine count:" << sp->getNumber() << ";shared_ptr count:" << sp.use_count() << std::endl
                      << std::endl;
            // 有事件可读
            if (events_[i].events & EPOLLIN)
            {
                if (connfd == listenfd_)
                {
                    // 设置协程任务为可读任务
                    std::cout << "创建连接";
                    sp->setTask(std::bind(&CoroutinePool::r_accept, this, _1, _2));
                }
                else
                {
                    std::cout << "读数据";
                    sp->setTask(std::bind(&CoroutinePool::r_recv, this, _1, _2));
                }
            }
            // 有事件可写
            else if (events_[i].events & EPOLLOUT)
            {
                std::cout << "协数据";
                // 设置协程任务为可写
                sp->setTask(std::bind(&CoroutinePool::r_send, this, _1, _2));
            }
            // 让出CPU给其他协程运行,这里是交给可读事件的协程
            resum(sp, connfd, listenfd_);
        }
    }
}

// 重启某个协程
void CoroutinePool::resum(std::shared_ptr<Coroutine> sp, int connfd, int listenfd)
{
    std::cout << "main 让出CPU 执行协程" << connfd << listenfd<< std::endl;
    sp->resum(connfd, listenfd);
    std::cout << "main 获取CPU 执行main" << connfd << listenfd<<std::endl;
}

// 创建连接
void CoroutinePool::r_accept(int clientfd, int listenfd)
{
    std::cout << "accept task " << clientfd << listenfd << std::endl;
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int client = accept(listenfd, (struct sockaddr *)&clientaddr, &len);
    if (client < 0)
    {
        std::cerr << "accept error" << std::endl;
        return;
    }
    std::cout << "client connection from " << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port)
              << "   client fd =" << client << std::endl;
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client;
    // 加入epoll
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client, &ev);
    // 设置非阻塞
    fcntl(client, F_SETFL, fcntl(client, F_GETFD, 0) | O_NONBLOCK);
}

void CoroutinePool::r_recv(int clientfd, int listenfd)
{
    std::cout << "read task " << clientfd << listenfd << std::endl;
    char buffer[1024];
    int len;
    if ((len = recv(clientfd, buffer, sizeof(buffer), 0)) > 0)
    {
        printf("%s fd %d \n", buffer, clientfd);
        struct epoll_event ev;
        ev.data.fd = clientfd;
        ev.events = EPOLLOUT | EPOLLET;
        // 修改状态给客户端连接返回
        epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, clientfd, &ev);
    }
    else
    {
        printf("client offline with: "
               "clientfd = %d \n",
               clientfd);
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, clientfd, NULL);
        // 应该析构shared_ptr
        close(clientfd);
    }
}

void CoroutinePool::r_send(int clientfd, int listenfd)
{
    std::cout << "send task " << clientfd << listenfd << std::endl;
    send(clientfd, "Welcome to My server\n", 21, 0);
    struct epoll_event ev;
    ev.data.fd = clientfd;
    ev.events = EPOLLIN | EPOLLET;
    // 修改状态给客户端连接返回
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, clientfd, &ev);
}
