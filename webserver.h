#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpoll/threadpoll.h"
#include "./http/http_conn.h"

const int MAX_FD = 65536;           //最大文件描述符
const int MAX_EVENT_NUMBER = 10000; //最大事件数
const int TIMESLOT = 5;             //最小超时单位

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void init(int port , string user, string passWord, string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);

    void thread_pool();//创建线程池
    void sql_pool();//创建数据库连接池
    void log_write();//写日志
    void trig_mode();//设置 IO 多路复用模式
    void eventListen();//创建监听 socket 并将其加入 epoll 监听队列中
    void eventLoop();//进入事件循环，不断调用 epoll_wait() 函数等待事件的发生，并调用相应的处理函数进行处理
    void timer(int connfd, struct sockaddr_in client_address);//添加定时器
    void adjust_timer(util_timer *timer);//调整定时器
    void deal_timer(util_timer *timer, int sockfd);//处理定时器到期的事件
    bool dealclinetdata();//处理客户端发来的请求
    bool dealwithsignal(bool& timeout, bool& stop_server);//处理信号
    void dealwithread(int sockfd);//处理可读事件
    void dealwithwrite(int sockfd);//处理可写事件

public:
    //基础
    int m_port;
    char *m_root;
    int m_log_write;
    int m_close_log;
    int m_actormodel;

    int m_pipefd[2];
    int m_epollfd;
    http_conn *users;

    //数据库相关
    connection_pool *m_connPool;
    string m_user;         //登陆数据库用户名
    string m_passWord;     //登陆数据库密码
    string m_databaseName; //使用数据库名
    int m_sql_num;

    //线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;

    //epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER;
    int m_TRIGMode;
    int m_LISTENTrigmode;
    int m_CONNTrigmode;

    //定时器相关
    client_data *users_timer;
    Utils utils;
};
#endif