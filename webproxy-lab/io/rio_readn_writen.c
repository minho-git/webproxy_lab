#include "../csapp.h"
#include <cerrno>
#include <stddef.h>

ssize_t rrio_readn(int fd, void *userbuf, size_t n) {
    
    size_t nleft = n;
    ssize_t nread;
    char *bufp = userbuf;
    
    while(nleft > 0) {
        
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            } else {
                return -1;
            }
        } else if(nread == 0) {
            break; // EOF
        }
        
        nleft -= nread;
        bufp += nread;
        
    }
    
    return n - nleft;
}

ssize_t rrio_writen(int fd, void *usrbuf, size_t n) {
    
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
        
        if ((nwritten = write(fd, usrbuf, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0;
            } else {
                return -1;
            }  
        }
        
        nleft -= nwritten;
        bufp += nwritten;

    }
        
        
    
    
    
}