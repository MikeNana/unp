//version.1
/*
#include "unp.h"
void str_cli(FILE* fp, int sockfd)
{
    char sendline[MAXLINE], recvline[MAXLINE];
    while(Fgets(sendline, MAXLINE, fp) != NULL) //此时只是单纯地阻塞在某一个特定的输入源，如果此时有来自服务器端的RST，
                                                //则无法正常处理
    {
        Writen(sockfd, sendline, strlen(sendline));
        if(Readline(sockfd, recvline, MAXLINE) == 0)
            err_quit("str_cli: server terminated prematurely");
        Fputs(recvline, stdout);
    }
}
*/
//用select更改str_cli的版本1，采用select可以同时监控多个文件描述符，并且对每个满足条件的文件描述符做出相应的处理
//但此时对于批量处理输入仍然有问题，如果在输入完EOF之后就关闭socket,可能导致还在管道中的数据无法到达客户端，从而显示不完全
//因此此时关闭只能关闭socket的写的一半，所以后续会用shutdown函数进行修改
/*
void str_cli(FILE* fp, int sockfd)
{
    int maxfdp1;
    char sendline[MAXLINE], recvline[MAXLINE];
    fd_set rset;
    FD_ZERO(&rset);
    for(;;)
    {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd);
        Select(maxfdp1, &rset, NULL, NULL, NULL);
        //如果已连接套接字可用
        if(FD_ISSET(sockfd, &rset))
        {
            if(Readline(sockfd, recvline, MAXLINE) == 0)
                err_quit("str_cli: server terminated prematurely");
            Fputs(recvline, stdout);
        }
        //如果标准输入可用
        if(FD_ISSET(fileno(fp), &rset))
        {
            if(Fgets(sendline, MAXLINE, fp) == NULL)//对EOF的处理不够严谨，如果是批量输入，此时应该保持半关闭状态
                return;                             //以此等待剩余数据的传输，可由shutdown函数完成
            Writen(sockfd, sendline, strlen(sendline));
        }
    }
    return;
}
*/
//version.2
//用shutdown和select修改，并且用的不是文本行处理，而是缓冲区处理的str_cli版本
/*
#include "unp.h"
void str_cli(FILE* fp, int sockfd)
{
    int maxfdp1, stdineof;
    stdineof = 0;
    fd_set rset;
    FD_ZERO(&rset);
    char buf[MAXLINE];
    int n;
    for(;;)
    {
        if(stdineof == 0)
            FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd)+1;
        Select(maxfdp1, &rset, NULL, NULL, NULL);
        //如果标准输入可读
        if(FD_ISSET(fileno(fp), &rset))
        {
            if((n = Read(fileno(fp), buf, MAXLINE)) == 0)
            {
                stdineof = 1;
                Shutdown(sockfd, SHUT_WR);//如果读到EOF，则关闭socket的写一半，此时还可以将socket中剩余的数据读出来，防止显示不完全的问题      
                FD_CLR(fileno(fp), &rset);
                continue;
            }
            Write(sockfd, buf, n);
        }
        //如果socket可读
        if(FD_ISSET(sockfd, &rset))
        {
            if((n = Read(fileno(fp), buf, MAXLINE)) == 0)
            {
                if(stdineof == 1)
                    return;
                else
                    err_quit("str_cli: server terminated prematurely.");
            }
        }
        Write(fileno(fp), buf, n);
    }
    
}
int main(int argc, char** argv)
{
    int sockfd[5];
    struct sockaddr_in servaddr, cliaddr;
    if(argc != 2)
        err_quit("usage: tcpcli <IPaddress>");
    for(int i = 0; i < 1; ++i)
    {
        sockfd[i] = Socket(AF_INET, SOCK_STREAM, 0);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(SERV_PORT);
        Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
        Connect(sockfd[i], (SA*)&servaddr, sizeof(servaddr));
    }
    str_cli(stdin, sockfd[0]);
    exit(0);
}
*/

//version.3
//采用poll来替代select重写该服务器程序
#include "unp.h"
#include <limits.h>
int main(int argc, char **argv)
{
    int i, maxi, sockfd, listenfd, connfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;
    struct pollfd client[OPEN_MAX];
    socklen_t clilen;
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    Bind(listenfd, (SA*)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    for(i = 0; i < OPEN_MAX; ++i)
        client[i].fd = -1;
    maxi = 0;

    for(;;)
    {
        nready = Poll(client, maxi+1, INFTIM);
        if(client[0].revents & POLLRDNORM)
        {
            clilen = sizeof(cliaddr);
            connfd = Accept(listenfd, (SA*)&cliaddr, &clilen);
            for(i = 1; i < OPEN_MAX; ++i)
            {
                if(client[i].fd < 0)
                {
                    client[i].fd = connfd;
                    break;
                }
            }
            if(i == OPEN_MAX)
                err_quit("too many clients.");
            if(i > maxi)
                maxi = i;
            if(--nready <= 0)
                continue;
        }
        for(i = 1; i < maxi; ++i)
        {
            if((sockfd = client[i].fd) < 0)
                continue;
            if(client[i].revents & (POLLRDNORM | POLLERR))
            {
                if((n = read(sockfd, buf, MAXLINE)) < 0)
                {
                    if(errno == ECONNRESET)
                    {
                        Close(sockfd);
                        client[i].fd = -1;
                    }
                    else
                        err_sys("read error");
                }
                else if(n == 0)
                {
                    Close(sockfd);
                    client[i].fd = -1;
                }
                else
                    Writen(sockfd, buf, n);
                if(--nready <= 0)
                    break;
            }
        }
    }
}
