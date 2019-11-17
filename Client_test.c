//用于测试九种服务器的设计范式
#include "unp.h"
#define MAXN 16384
int main(int argc, char** argv)
{
    int fd, nchildren, nloops, nbytes;
    pid_t pid;
    ssize_t n;
    char request[MAXLINE], reply[MAXLINE];
    if(argc != 6)
        err_quit("usage: client <hostname or IPaddr> <port> <#children> <#loops/child> <#bytes/request>");
    nchildren = atoi(argv[3]);
    nloops = atoi(argv[4]);
    nbytes = atoi(argv[5]);
    snprintf(request, sizeof(request), "%d\n", nbytes);

    for(int i = 0; i < nchildren; ++i)
    {
        if((pid == Fork()) == 0)
        {
            for(int j = 0; j < nloops; ++j)
            {
                fd = Tcp_connect(argv[1], argv[2]);
                Write(fd, request, strlen(request));
                if((n = Readn(fd, reply, nbytes)) != nbytes)
                    err_quit("server returned %d bytes.", n);
                Close(fd);
            }
            printf("child %d done.\n",i);
            exit(0);   
        }
    }
    while(wait(NULL) > 0);
    if(errno != ECHILD)
        err_sys("wait error");
    exit(0);
}