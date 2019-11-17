#include "unp.h"
void dg_echo(int sockfd, SA* cliaddr, socklen_t clilen)
{   
    int n;
    socklen_t len;
    char msg[MAXLINE];
    for(;;)
    {
        len = clilen;
        n = Recvfrom(sockfd, msg, MAXLINE, 0, cliaddr, &len);
        Sendto(sockfd, msg, n, 0, cliaddr, len);
    }
}
int main(int argc, char** argv)
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    Bind(sockfd, (SA*)&servaddr, sizeof(servaddr));
    dg_echo(sockfd, (SA*)&cliaddr, sizeof(cliaddr));
    return 0;
}