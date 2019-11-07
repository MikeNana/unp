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