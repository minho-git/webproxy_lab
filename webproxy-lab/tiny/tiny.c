/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <cstdio>
#include <string.h>

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, int head);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, int head);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd; //test
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]); // 지정된 파일 스트림에 지정된 형식으로 출력한다.
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // socket() + bind() + lisnten()
  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // clientaddr에 주소 채워 넣기!
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // clientaddr로 호스트, 포트 채워넣기
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 클라이언트 포트 출력
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

void doit(int fd) {

    int is_static;
    struct stat sbuf;
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    int is_head = 0;

    Rio_readinitb(&rio, fd); // rio 구조체 초기화. 파일 디스크립터 연결, 버퍼는 내부 구조체에 존재
    Rio_readlineb(&rio, buf, MAXLINE); // rio 구조체를 이용해서 한줄 읽어서 buf에 담기.

    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version); // buf에서 method, uri, version 채워 넣기.
    
    if (strcasecmp(method, "GET") == 0) {
        //pass
    } else if (strcasecmp(method, "HEAD") == 0) { // method가 GET 방식이 아닌 경우 에러 출력하고 종료.
       is_head = 1; 
    } else {
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }

    read_requesthdrs(&rio); // Tiny 서버가 사용하지 않는 부가 정보(헤더)들을 '빈 줄'이 나올 때까지 읽어서 버퍼에서 깨끗이 치워버리는 역할

    // Parse URI
    is_static = parse_uri(uri, filename, cgiargs); // 여기서 filename이랑 cgiargs 할당 완료.
    if (stat(filename, &sbuf) < 0) { // stat 작동 과정 공부하기!
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if (is_static) { // 정적 콘텐츠


        serve_static(fd, filename, sbuf.st_size, is_head);

    } else { // 동적 콘텐츠


        serve_dynamic(fd, filename, cgiargs, is_head);
    }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {

    char body[MAXLINE], buf[MAXLINE];

    // HTTP 응답 body 만들기
    sprintf(body, "<html><title>Tiny Errorr</title>");
    sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web serever</em>\r\n", body);

    // HTTP 응답 출력
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf)); //널 종결 문자(\0)를 만나는 '직전까지'의 문자 개수
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {

    char buf[MAXLINE]; // 임시 작업대, rio_t에서 버퍼랑 위치 기록한다.

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }

    return;
}

int parse_uri(char *uri, char *filename, char *cgiargs) {

    char *ptr;

    if (!strstr(uri, "cgi-bin")) { // 정적 콘텐츠: uri = "/" 또는 uri = "/page.html"
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/') { // 만약 uri가 /로 끝난다면
            strcat(filename, "home.html");
        }

        return 1;
    } else { // 동적 콘텐츠
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1); // ? 뒤에 인자들을 cgiargs에 대입
            *ptr = '\0'; // ? 문자를 \0(문자열 끝)로 변경
        } else {
            strcpy(cgiargs, "");
        }

        strcpy(filename, ".");
        strcat(filename, uri); // "." + "? 앞에 남아있는 부분"
        return 0;
    }
}

void serve_static(int fd, char *filename, int filesize, int head) {

    char filetype[MAXLINE], buf[MAXLINE], *srcp, *start;
    int srcfd;

    // 응답 헤더 보내기
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers:\n");
    printf("%s", buf);

    //응답 바디 보내기
    if (!head) {
        srcfd = Open(filename, O_RDONLY, 0);
        // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
        // Close(srcfd);
        // Rio_writen(fd, srcp, filesize);
        // Munmap(srcp, filesize);
    
        // 11.9
        // mmap, Rio_writen -> malloc, Rio_readn, Rio_writen
        // 파일 사이즈만큼 말록 할당해서 얻은 포인터를 read 인자로 넘기고 쓰자
    
        start = (char *)Malloc(filesize);
        Rio_readn(srcfd, start, filesize);
        Rio_writen(fd, start, filesize);
    
        Close(srcfd);
        free(start);
    }
}

void get_filetype(char *filename, char *filetype) {

    if (strstr(filename, ".html")) {
        strcpy(filetype, "text/html");
    } else if (strstr(filename, ".gif")) {
        strcpy(filetype, "image/gif");
    } else if(strstr(filename, ".png")) {
        strcpy(filetype, "image/png");
    } else if(strstr(filename, ".jpg")) {
        strcpy(filetype, "image/jpeg");
    } else if(strstr(filename, ".mp4")) {
        strcpy(filetype, "video/mp4");
    } else {
        strcpy(filetype, "text/plain");
    }
}

void serve_dynamic(int fd, char *filename, char *cgiargs, int head) {

    char buf[MAXLINE], *emptylist[] = {NULL};

    // 서버가 아는 최소한의 헤더만 보내기
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (!head) {
        if (Fork() == 0) { // 자식 프로세스
            setenv("QUERY_STRING", cgiargs, 1);
            Dup2(fd, STDOUT_FILENO);
            Execve(filename, emptylist, environ);
        }
    }
    Wait(NULL);

}
