#include "csapp.h"


int main(int argc, char **argv) {

    int listenfd, connfd;
    struct sockaddr_storage clientaddr;
    socklen_t clientlen;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "오류 발생: %s <port>\n", argv[0]);
        exit(0);
    }


    // 소켓 생성, 연결, 대기
    listenfd = Open_listenfd(argv[1]);

    while(1) {

        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        
        Getnameinfo(&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);


        // echo(connfd);
        Close(connfd);        
    }

    exit(0);
}