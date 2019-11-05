#include "error.h"
#include "error.c"
#include <sys/errno.h>
int main(int argc, char** argv)
{
    int sockfd, n;
    struct sockaddr_in servaddr;
    char recvline[MAXLINE+1];
    //首先判断参数个数是否正确
    if(argc != 2)
        err_quit("usage: a.out < <IPaddress>");
    //判断socket的创建是否成功
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_sys("socket error");
    //向服务器地址结构体中添加信息
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13);
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0)
        err_quit("inet_pton error for %s.", argv[1]);
    //连接
    if(connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) < 0)
        err_sys("connect error");
    while((n = read(sockfd, recvline, MAXLINE)) > 0)
    {
        recvline[n] = 0;
        if(fputs(recvline, stdout) == EOF)
            err_sys("fputs error");
    }
    return 0;
}