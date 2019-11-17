//客户/服务器设计范式
//1.多进程:每个客户一个子进程
//优点:容易理解，代码书写方便
//缺点:无法处理高并发的情况，临时开辟子进程开销很大
/*
#include "unp.h"
int main(int argc, char ** argv)
{
    int connfd, listenfd;
    pid_t childpid;
    socklen_t addrlen, clilen;
    struct sockaddr *cliaddr;
    void sig_chld(int), sig_int(int), web_child(int);

    if(argc == 2)
        listenfd = Tcp_listen(NULL, argv[1], &addrlen);
    else if(argc == 3)
        listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
    else 
        err_quit("usage: server01 [<host>] <port#> ");
    cliaddr = Malloc(addrlen);

    signal(SIGCHLD, sig_chld);
    signal(SIGINT, sig_int);

    for(;;)
    {
        clilen = addrlen;
        if((connfd = accept(listenfd, cliaddr, &clilen)) < 0)
        {
            if(errno == EINTR)
                continue;
            else
                err_sys("accept error");
        }
        if((childpid = Fork()) == 0)
        {
            Close(listenfd);
            web_child(connfd);
            exit(0);
        }
        Close(connfd);
    }
}
*/
//2.预先派生子进程池且accept无上锁保护，以此来减少临时开辟子进程带来的开销
//优点:可部分减小服务器压力
//缺点:由于需要预先派生子进程，而需要派生的个数不确定，如果提前生成的子进程太多也会影响服务器性能，太少又没有太好的效果
#include "unp.h"
static int nchildren;
static pid_t *pids;
pid_t child_make(int i, int listenfd, int addrlen)
{
    pid_t pid;
    void child_main(int, int, int);
    if((pid = Fork()) > 0)
        return (pid);
    child_main(i, listenfd, addrlen);
}
void child_main(int i, int listenfd, int addrlen)
{
    int connfd;
    void web_child(int);
    socklen_t clilen;
    struct sockaddr* cliaddr;
    cliaddr = Malloc(addrlen);
    printf("child %ld starting\n", (long)getpid());
    for(;;)
    {
        clilen = addrlen;
        connfd = Accept(listenfd, cliaddr, &clilen);
        web_child(connfd);
        Close(connfd);
    }
}
int main(int argc, char** argv)
{
    int listenfd, i;
    socklen_t addrlen;
    void sig_int(int);
    pid_t child_make(int, int, int);
    if(argc == 3)
        listenfd = Tcp_listen(NULL, argv[1], &addrlen);
    else if(argc == 4)
        listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
    else
        err_quit("usage: server02 [<host>] <port#> <#childre>");
    nchildren = atoi(argv[argc-1]);
    pids = Calloc(nchildren, sizeof(pid_t));
    for(i = 0; i < nchildren; ++i)
        pids[i] = child_make(i, listenfd, addrlen);
    signal(SIGINT, sig_int);
    for(;;)
        pause();
    
}