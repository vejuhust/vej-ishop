#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>


#define MAXLINE         8192
#define MAXBUF          8192
#define LISTENQ         1024
#define RIO_BUFSIZE     8192


extern int     h_errno;
extern char ** environ;

typedef struct {
    int    rio_fd;
    int    rio_cnt;
    char * rio_bufptr;
    char   rio_buf  [RIO_BUFSIZE];
} rio_t;


int unix_error(char * msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}


int dns_error(char * msg) {
    fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
    exit(0);
}


int app_error(char * msg) {
    fprintf(stderr, "%s\n", msg);
    exit(0);
}


pid_t Fork(void) {
    pid_t   pid;

    if ((pid = fork()) < 0) {
        unix_error("Fork error");
    }
    
    return (pid);
}


void Execve(const char * filename, char * const argv[], char * const envp[]) {
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}


pid_t Wait(int * status) {
    pid_t   pid;

    if ((pid  = wait(status)) < 0) {
        unix_error("Wait error");
    }
    
    return (pid);
}


int Open(const char * pathname, int flags, mode_t mode) {
    int     rc;

    if ((rc = open(pathname, flags, mode)) < 0) {
        unix_error("Open error");
    }

    return (rc);
}


void Close(int fd) {
    int     rc;

    if ((rc = close(fd)) < 0) {
        unix_error("Close error");
    }
}


int Dup2(int fd1, int fd2) {
    int     rc;

    if ((rc = dup2(fd1, fd2)) < 0) {
        unix_error("Dup2 error");
    }
    
    return (rc);
}


void *Mmap(void * addr, size_t len, int prot, int flags, int fd, off_t offset) {
    void    * ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1)) {
        unix_error("mmap error");
    }
    
    return (ptr);
}


void Munmap(void * start, size_t length) {
    if (munmap(start, length) < 0) {
        unix_error("munmap error");
    }
}


int Accept(int s, struct sockaddr * addr, socklen_t * addrlen) {
    int     rc;

    if ((rc = accept(s, addr, addrlen)) < 0) {
        unix_error("Accept error");
    }

    return (rc);
}


ssize_t rio_readn(int fd, void * usrbuf, size_t n) {
    size_t      nleft = n;
    ssize_t     nread;
    char      * bufp  = usrbuf;

    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            }
            else {
                return (-1);
            }
        }
        else {
            if (nread == 0) {
                break;
            }
        }
        nleft -= nread;
        bufp  += nread;
    }
    
    return (n - nleft);
}


ssize_t rio_writen(int fd, void * usrbuf, size_t n) {
    size_t      nleft = n;
    ssize_t     nwritten;
    char      * bufp  = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR) {
                nwritten = 0;
            }
            else {
                return (-1);
            }
        }
        nleft -= nwritten;
        bufp  += nwritten;
    }
    
    return (n);
}


static ssize_t rio_read(rio_t * rp, char * usrbuf, size_t n) {
    int     cnt;

    while (rp->rio_cnt <= 0) {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) {
                return (-1);
            }
        }
        else {
            if (rp->rio_cnt == 0) {
                return (0);
            }
            else {
                rp->rio_bufptr = rp->rio_buf;
            }
        }
    }

    cnt = n;
    if (rp->rio_cnt < n) {
        cnt = rp->rio_cnt;
    }

    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt    -= cnt;

    return (cnt);
}


void rio_readinitb(rio_t * rp, int fd) {
    rp->rio_fd     = fd;
    rp->rio_cnt    = 0;
    rp->rio_bufptr = rp->rio_buf;
}


ssize_t rio_readnb(rio_t * rp, void *usrbuf, size_t n) {
    size_t      nleft = n;
    ssize_t     nread;
    char      * bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = rio_read(rp, bufp, nleft)) < 0) {
            if (errno == EINTR) {
                nread = 0;
            }
            else {
                return (-1);
            }
        }
        else {
            if (nread == 0) {
                break;
            }
        }
        nleft -= nread;
        bufp  += nread;
    }

    return (n - nleft);
}


ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    int        n;
    int        rc;
    char       c;
    char     * bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                break;
            }
        }
        else {
            if (rc == 0) {
                if (n == 1) {
                    return (0);
                }
                else {
                    break;
                }
            }
            else {
                return (-1);
            }
        }
    }
    *bufp = 0;
    
    return (n);
}


void Rio_writen(int fd, void *usrbuf, size_t n) {
    if (rio_writen(fd, usrbuf, n) != n) {
        unix_error("Rio_writen error");
    }
}


ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t     rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        unix_error("Rio_readlineb error");
    }
    
    return (rc);
}


int open_listenfd(int port) {
    struct sockaddr_in  serveraddr;
    int                 listenfd;
    int                 optval   = 1;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return (-1);
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval, sizeof(int)) < 0) {
        return (-1);
    }

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family      = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port        = htons((unsigned short) port);
    
    if (bind(listenfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0) {
        return (-1);
    }

    if (listen(listenfd, LISTENQ) < 0) {
        return (-1);
    }
    
    return (listenfd);
}


int Open_listenfd(int port) {
    int     rc;

    if ((rc = open_listenfd(port)) < 0) {
        unix_error("Open_listenfd error");
    }
    
    return (rc);
}


void client_error(int fd, char * cause, char * errnum,
                         char * shortmsg, char * longmsg) {
    char    buf   [MAXLINE];
    char    body  [MAXBUF];

    sprintf(body, "<html><title>iShop Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The iShop Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


void print_time(void) {
    struct timeval        tv;
    static struct timeval first;
    static long int       cnt = 0;
    
    ++cnt;

    if (1 == cnt) {
        if (0 != gettimeofday(&first, NULL)) {
            app_error("gettimeofday error");
        }
    }

    if (0 != gettimeofday(&tv, NULL)) {
        app_error("gettimeofday error");
    }
    
    if (tv.tv_usec < first.tv_usec) {
        printf("[num:%-10ld time:%8ldsec %7ldusec]\n", cnt,
                (long)tv.tv_sec  - (long)first.tv_sec  - 1,
                (long)tv.tv_usec - (long)first.tv_usec + 1000000);
    }
    else {
        printf("[num:%-10ld time:%8ldsec %7ldusec]\n", cnt,
                (long)tv.tv_sec  - (long)first.tv_sec,
                (long)tv.tv_usec - (long)first.tv_usec);
    }
}


void read_request_headers(rio_t * rp) {
    char    buf  [MAXLINE];

    print_time();

    Rio_readlineb(rp, buf, MAXLINE);    
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }

    return;
}


int parse_uri(char * uri, char * filename, char * cgiargs) {
    char   * ptr;

    if (!strstr(uri, "cgi-bin")) {
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/') {
            strcat(filename, "index.htm");
        }
        
        return (1);
    }
    else {
        ptr = index(uri, '?');

        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        }
        else {
            strcpy(cgiargs, "");
        }
        
        strcpy(filename, ".");
        strcat(filename, uri);
        
        return (0);
    }
}


void get_filetype(char * filename, char * filetype) {
    if (strstr(filename, ".htm"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".css"))
        strcpy(filetype, "text/css");
    else if (strstr(filename, ".js"))
        strcpy(filetype, "text/javascript");
    else
        strcpy(filetype, "text/plain");
}


void serve_static(int fd, char * filename, int filesize) {
    int     srcfd;
    char  * srcp;
    char    filetype    [MAXLINE];
    char    buf         [MAXBUF];
 
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: iShop Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    srcfd = Open(filename, O_RDONLY, 0);
    srcp  = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}


void serve_dynamic(int fd, char * filename, char * cgiargs) {
    char    buf         [MAXLINE];
    char  * emptylist   []  = { NULL };

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: iShop Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
  
    if (Fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
    
    Wait(NULL);
}

void play_with_me(int fd) {
    int         is_static;
    char        buf         [MAXLINE];
    char        method      [MAXLINE];
    char        uri         [MAXLINE];
    char        version     [MAXLINE];
    char        filename    [MAXLINE];
    char        cgiargs     [MAXLINE];
    rio_t       rio;
    struct stat sbuf;

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")) {
        client_error(fd, method, "501", "Not Implemented",
                                 "iShop does not implement this method");
        return;
    }
    read_request_headers(&rio);

    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        client_error(fd, filename, "404", "Not found",
                                   "iShop couldn't find this file");
        return;
    }

    if (is_static) {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            client_error(fd, filename, "403", "Forbidden",
                                       "iShop couldn't read this file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    }
    else {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            client_error(fd, filename, "403", "Forbidden",
                                       "iShop couldn't run this CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}

int main(int argc, char ** argv) {
    struct sockaddr_in  clientaddr;
    int                 listenfd;
    int                 connfd;
    int                 port;
    int                 clientlen;
    

    if (argc != 2) {
        port = 8080;
    }
    else {
        port = atoi(argv[1]);
    }

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        play_with_me(connfd);
        Close(connfd);
    }
}
