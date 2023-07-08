//finished
//由于非活跃连接占用了连接资源，严重影响服务器的性能，通过实现一个服务器定时器，处理这种非活跃连接，释放连接资源。利用alarm函数周期性地触发SIGALRM信号,该信号的信号处理函数利用管道通知主循环执行定时器链表上的定时任务.
#include"lst_timer.h"
#include"../http/http_conn.h"

sort_timer_lst::sort_timer_lst()
{
    head = NULL;
    tail = NULL;
}

sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;
    while(tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (!head)
    {
        head = tail = timer;
        return;
    }
    if (timer->expire < head->expire)
    {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

//定时器链表中插入一个新的定时器节点，并保证链表中所有节点按照超时时间从小到大排列。
void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head)
{
    util_timer *prev = lst_head;
    util_timer *tmp = prev->next;
    while (tmp)
    {
        if (timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

//实现了调整定时器链表中一个定时器节点的位置，以确保链表中所有节点按照超时时间从小到大排列
void sort_timer_lst::adjust_timer(util_timer* timer)
{
    if(!timer)
    {
        return;
    }
    util_timer* tmp = timer->next;
    if(!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    if(timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->prev = NULL;
        add_timer(timer, head);
    }
    else{
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void sort_timer_lst::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

//处理超时的定时器节点，并执行相应的回调函数
void sort_timer_lst::tick()
{
    if (!head)
    {
        return;
    }
    
    time_t cur = time(NULL);
    util_timer *tmp = head;
    while (tmp)
    {
        if (cur < tmp->expire)
        {
            break;
        }
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void Utils::init(int timeslot)//初始化定时器时间间隔
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数,该函数用于处理 SIG 信号，将信号值写入管道中，以触发 epoll_wait 函数的返回
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;//创建一个 sigaction 结构体 sa
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;//将结构体 sa 中的 sa_handler 成员设为参数 handler，表示当接收到信号时会调用该处理函数
    if (restart)//如果参数 restart 为 true，将结构体 sa 中的 sa_flags 成员的 SA_RESTART 标志位置 1，表示在信号处理函数执行期间，被该信号中断的系统调用会自动重启
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);//将结构体 sa 中的 sa_mask 成员设为信号集的全集，即表示在信号处理函数执行期间，屏蔽所有信号
    assert(sigaction(sig, &sa, NULL) != -1);//调用 sigaction 函数向系统注册信号处理函数，并检查函数的返回值是否为 -1，如果是则说明注册失败
}

//定时处理任务，重新定时以不断触发SIGALRM信号调用定时器链表对象的 tick 函数处理超时的定时器，并设置下一次定时器事件的超时时间
void Utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
//用于关闭客户端连接并删除其对应的 epollfd，以便服务器端不再监听该客户端套接字的事件
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}

