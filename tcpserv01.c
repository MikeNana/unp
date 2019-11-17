//version.1
//开启多个子进程为不同客户服务，此时开启新进程的开销不可忽略
/*
#include "unp.h"
void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];
again:
    while((n = read(sockfd, buf, MAXLINE)) > 0)
        Writen(sockfd, buf, n);
    if(n < 0 && errno == EINTR)
        goto again;
    else if (n < 0)
    {
        err_sys("str_echo: read error");
    }
}
void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child %d terminated\n", pid);
    return ;
}
int main(int argc, char** argv)
{
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    pid_t children;
    void sig_chld(int);
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    Bind(listenfd, (SA*)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);
    //必须捕获SIGCHLD信号并作相应的处理
    signal(SIGCHLD, sig_chld);
    for(;;)
    {
        len = sizeof(cliaddr);
        //需要对被中断的系统调用作处理
        if((connfd = accept(listenfd, (SA*)&cliaddr, &len)) < 0)
        {
            if(errno == EINTR)
                continue;
            else
                err_sys("accept error");
        }
        if((children = Fork()) == 0)
        {
            Close(listenfd);
            str_echo(connfd);
            exit(0);
        }
        Close(connfd);
    }
}
*/

//version.2
//使用单进程和select的TCP服务器，可避免多进程的开销，但仍然有问题
/*
#include "unp.h"
int main(int argc, char ** argv)
{
    int i, maxi, sockfd, connfd, listenfd, maxfd;
    int nready, client[FD_SETSIZE];
    ssize_t n;
    fd_set rset, allset;
    char buf[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in servaddr, cliaddr;

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    Bind(listenfd, (SA*)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

    maxfd = listenfd;
    maxi = -1;
    for(i = 0; i < FD_SETSIZE; ++i)
        client[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for(;;)
    {
        rset = allset;
        nready = Select(maxfd+1, &rset, NULL, NULL, NULL);
        //如果有新连接到来
        if(FD_ISSET(listenfd, &rset))
        {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd, (SA*)&cliaddr, &clilen);

            for(i = 0; i < FD_SETSIZE; ++i)
            {
                if(client[i] < 0)
                {
                    client[i] = connfd;
                    break;
                }
            }
            if(i == FD_SETSIZE)
                err_quit("too many clients\n");
            FD_SET(connfd, &rset);
            if(connfd > maxfd)
                maxfd = connfd;
            if(i > maxi)
                maxi = i;
            if(--nready <= 0)
                continue;
        }
        for(i = 0; i <= maxi; ++i)
        {
            if((sockfd = client[i]) < 0)
                continue;
            if(FD_ISSET(sockfd, &rset))
            {
                if((n = Read(sockfd, buf, MAXLINE)) == 0)
                {
                    Close(sockfd);
                    FD_CLR(sockfd, &rset);
                    client[i] = -1;
                }
                else
                    Writen(sockfd, buf, n);
                if(--nready <= 0)
                    break;
            }
        }
    }
}
*/

//version.3 多线程版本的回射服务器
#include "unpthread.h"
static void* doit(void*);

int main(int argc, char** argv)
{
    int listenfd, *iptr;
    pthread_t tid;
    socklen_t addrlen, len;
    struct sockaddr *cliaddr;
    if(argc == 2)
        listenfd = Tcp_listen(NULL, argv[1], &addrlen);
    else if(argc == 3)
        listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
    else 
        err_quit("usage: tcpserv01 [<host>] <service or port>");
    cliaddr = Malloc(addrlen);
    for(;;)
    {
        len = addrlen;
        iptr = Malloc(sizeof(int));
        *iptr = Accept(listenfd, cliaddr, &len);
        Pthread_create(&tid, NULL, &doit, iptr);//注意:参数只能传指针，所以每次循环应该重新开辟一个指针的空间来存新的已连接描述符
    }
}

static void* doit(void* arg)
{
    int connfd;
    connfd = *((int*)arg);
    free(arg);
    
    Pthread_detach(pthread_self());
    str_echo(connfd);
    Close(connfd);
    return (NULL);
}