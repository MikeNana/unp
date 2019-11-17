#include "unp.h"

void dg_cli(FILE* fp, int sockfd, const SA* pservaddr, socklen_t servlen)
{
    char sendline[MAXLINE], recvline[MAXLINE+1];
    int n;
    while(Fgets(sendline, MAXLINE, fp) != NULL)
    {
        Sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
        n = Recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
        recvline[n] = 0;
        Fputs(recvline, stdout);
    }
}
int main(int argc, char** argv)
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    if(argc != 2)
        err_quit("usage: udpcli<IPaddress>\n");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    dg_cli(stdin, sockfd, (SA*)&servaddr, sizeof(servaddr));
    exit(0);
}
