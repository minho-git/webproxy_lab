#include <stdio.h>
#include <string.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);



/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    // 명령어 검사
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 소켓 연결 대기
    listenfd = Open_listenfd(argv[1]);

    while(1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        doit(connfd);
        Close(connfd);
    }
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {

    char body[MAXLINE], buf[MAXLINE];

    // HTTP 응답 body 만들기
    sprintf(body, "<html><title>Tiny Errorr</title>");
    strcat(body, "<body bgcolor=\"ffffff\">\r\n");
    strcat(body, errnum);
    strcat(body, ": ");
    strcat(body, shortmsg);
    strcat(body, "\r\n");
    strcat(body, "<p>");
    strcat(body, longmsg);
    strcat(body, ": ");
    strcat(body, cause);
    strcat(body, "\r\n");
    strcat(body, "<hr><em>The Proxy Server</em>\r\n"); // (Tiny -> Proxy로 수정)

    // HTTP 응답 출력
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf)); //널 종결 문자(\0)를 만나는 '직전까지'의 문자 개수
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void doit(int fd) {

    rio_t rio, server_rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], server_buf[MAXLINE];
    char hostname[MAXLINE], path[MAXLINE];
    char hostname_with_port[MAXLINE], port[MAXLINE];
    char *port_ptr;
    int host_header_found = 0;
    int serverfd;
    size_t n;

    // 요청 버퍼
    char request_buf[MAXLINE];

    Rio_readinitb(&rio, fd);
    if (Rio_readlineb(&rio, buf, MAXLINE) <= 0) {
        return; // 클라이언트가 아무것도 안 보냈으면 종료
    }

    if (sscanf(buf, "%s %s %s", method, uri, version) != 3) {
        // 3개가 아니면 "400 Bad Request" 에러 전송
        clienterror(fd, buf, "400", "Bad Request",
                    "Proxy received a malformed request line");
        return;
    }

    if (strcasecmp(method, "GET") != 0) {
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }

    // uri -> [http://] + [hostname_with_port] + [path]로 분리하기
    // "http://"를 제외한 나머지("www.cmu.edu:8080/hub/index.html")에서
    // 첫 "/"를 기준으로 [호스트+포트] 부분과 [경로] 부분을 분리합니다.
    if (sscanf(uri, "http://%[^/]%s",hostname_with_port, path) == 2) {
        // 성공
    } else if (sscanf(uri, "http://%s", hostname_with_port) == 1){ // "/"가 없는 경우 (예: "http://www.cmu.edu")
        strcpy(path, "/");
    } else {
        // URI가 "http://"로 시작하지 않는 등 형식이 잘못됨
        clienterror(fd, uri, "400", "Bad Request",
                    "Proxy received a malformed URI");
        return;
    }

    // hostname_with_port -> [hostname] + [port]
    // www.cmu.edu:8080
    port_ptr = strchr(hostname_with_port, ':');

    if (port_ptr != NULL) {
        *port_ptr = '\0';
        strcpy(hostname, hostname_with_port);
        strcpy(port, port_ptr + 1);

    } else {
        strcpy(hostname, hostname_with_port);
        strcpy(port, "80");
    }

    // 요청 헤더 조립하기
    // GET 라인 만들기
    sprintf(request_buf, "GET %s HTTP/1.0\r\n", path);

    // 나머지 헤더들을 돌면서 빈라인 만나기 전까지
    while (Rio_readlineb(&rio, buf, MAXLINE) > 0) {

        if (strcmp(buf, "\r\n") == 0) {
            break;
        }

        // 이미 추가한 헤더는 continue하고
        if ((strncasecmp(buf, "User-Agent:", 11) == 0) ||
            (strncasecmp(buf, "Connection:", 11) == 0) ||
            (strncasecmp(buf, "Proxy-Connection:", 17) == 0)) {

            continue;
        }

        // Host 헤더를 보냈다면
        if ((strncasecmp(buf, "Host:", 5)) == 0) {
            host_header_found = 1;
            strcat(request_buf, buf); // 🚨 FIX: sprintf -> strcat
            continue;
        }

        // 나머지 헤더는 그대로 추가하기
        strcat(request_buf, buf); // 🚨 FIX: sprintf -> strcat
    }

    // Host 헤더를 안 보냈다면
    if (!host_header_found) {
        // 🚨 FIX: sprintf -> strcat (문자열을 나눠서 덧붙임)
        strcat(request_buf, "Host: ");
        strcat(request_buf, hostname);
        strcat(request_buf, "\r\n");
    }

    // 필수 헤더 추가하기
    strcat(request_buf, user_agent_hdr); // 🚨 FIX: sprintf -> strcat
    strcat(request_buf, "Connection: close\r\n"); // 🚨 FIX: sprintf -> strcat
    strcat(request_buf, "Proxy-Connection: close\r\n"); // 🚨 FIX: sprintf -> strcat

    // 마지막에 빈라인 추가하기
    strcat(request_buf, "\r\n");

    // 서버에게 요청 보내기
    serverfd = Open_clientfd(hostname, port); //Open_clientfd (클라이언트용): getaddrinfo() + socket() + connect()

    Rio_readinitb(&server_rio, serverfd);
    Rio_writen(serverfd, request_buf, strlen(request_buf));

    // 서버에게 응답 받아서 클라이언트에게 보내주기
    while ((n = Rio_readnb(&server_rio, server_buf, MAXLINE)) > 0) {

        Rio_writen(fd, server_buf, n);
    }


    Close(serverfd);



}
