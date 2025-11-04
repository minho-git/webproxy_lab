#include "csapp.h"


int main(void) {

    struct sockaddr_storage clientaddr;
    socklen_t size = sizeof(clientaddr);

    int listenfd = open_listenfd("9000");
    int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, (socklen_t*)&size);
    char buf[1024];

    ssize_t clientlen;

    while((clientlen = read(connfd, buf, sizeof(buf))) > 0) {

        printf("수신: %zd 바이트.\n", (ssize_t)clientlen);
    

        write(connfd, buf, clientlen);
    }

    close(connfd);
    close(listenfd);
    return 0;
}