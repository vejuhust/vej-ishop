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


#define MAXLINE         8192            /*HTTP报头的缓存大小*/
#define MAXBUF          8192            /*HTTP响应的缓存大小*/
#define LISTENQ         1024            /*listen()的backlog参数值*/
#define RIO_BUFSIZE     8192            /*内建缓存大小*/


extern int     h_errno;                 /*sockets用错误变量，在netdb.h中定义*/
extern char ** environ;                 /*环境表指针*/


/*缓冲rio_t的结构体*/
typedef struct {
    int    rio_fd;                          /*内建缓冲区的描述符*/
    int    rio_cnt;                         /*内建缓冲区中未读字节的数量*/
    char * rio_bufptr;                      /*内建缓冲区去中下一个未读字节的地址*/
    char   rio_buf  [RIO_BUFSIZE];          /*内建缓冲区*/
} rio_t;


/*UNIX类错误处理*/
int unix_error(char * msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}


/*DNS类错误处理*/
int dns_error(char * msg) {
    fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
    exit(0);
}


/*应用类错误处理*/
int app_error(char * msg) {
    fprintf(stderr, "%s\n", msg);
    exit(0);
}


/*fork()系统调用的封装*/
pid_t Fork(void) {
    pid_t   pid;

    if ((pid = fork()) < 0) {
        unix_error("Fork error");
    }

    return (pid);
}


/*execve()系统调用的封装*/
void Execve(const char * filename, char * const argv[], char * const envp[]) {
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}


/*wait()系统调用的封装*/
pid_t Wait(int * status) {
    pid_t   pid;

    if ((pid  = wait(status)) < 0) {
        unix_error("Wait error");
    }

    return (pid);
}


/*open()系统调用的封装*/
int Open(const char * pathname, int flags, mode_t mode) {
    int     rc;

    if ((rc = open(pathname, flags, mode)) < 0) {
        unix_error("Open error");
    }

    return (rc);
}


/*close()系统调用的封装*/
void Close(int fd) {
    int     rc;

    if ((rc = close(fd)) < 0) {
        unix_error("Close error");
    }
}


/*dup2()系统调用的封装*/
int Dup2(int fd1, int fd2) {
    int     rc;

    if ((rc = dup2(fd1, fd2)) < 0) {
        unix_error("Dup2 error");
    }

    return (rc);
}


/*mmap()系统调用的封装*/
void *Mmap(void * addr, size_t len, int prot, int flags, int fd, off_t offset) {
    void    * ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *) -1)) {
        unix_error("mmap error");
    }

    return (ptr);
}


/*munmap()系统调用的封装*/
void Munmap(void * start, size_t length) {
    if (munmap(start, length) < 0) {
        unix_error("munmap error");
    }
}


/*sockets函数accept()的封装*/
int Accept(int s, struct sockaddr * addr, socklen_t * addrlen) {
    int     rc;

    if ((rc = accept(s, addr, addrlen)) < 0) {
        unix_error("Accept error");
    }

    return (rc);
}


/*从描述符为fd文件的当前位置处读取最多n个字节存入usrbuf(无缓冲)*/
ssize_t rio_readn(int fd, void * usrbuf, size_t n) {
    size_t      nleft = n;
    ssize_t     nread;
    char      * bufp  = usrbuf;

    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            /*若该系统调用被中断*/
            if (errno == EINTR) {
                /*准备再次调用*/
                nread = 0;
            }
            else {
                /*其他错误情况*/
                return (-1);
            }
        }
        else {
            /*若已读至文件尾*/
            if (nread == 0) {
                break;
            }
        }

        nleft -= nread;
        bufp  += nread;
    }

    /*返回已读入的字节数*/
    return (n - nleft);
}


/*从usrbuf中最多传送n个字节至描述符为fd文件的当前位置(无缓冲)*/
ssize_t rio_writen(int fd, void * usrbuf, size_t n) {
    size_t      nleft = n;
    ssize_t     nwritten;
    char      * bufp  = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            /*若该系统调用被中断*/
            if (errno == EINTR) {
                /*准备再次调用*/
                nwritten = 0;
            }
            else {
                /*其他错误情况*/
                return (-1);
            }
        }

        nleft -= nwritten;
        bufp  += nwritten;
    }

    /*返回已写入的字节数*/
    return (n);
}


/*从缓冲区rp中读取最多n个字节存入usrbuf*/
static ssize_t rio_read(rio_t * rp, char * usrbuf, size_t n) {
    int     cnt;

    while (rp->rio_cnt <= 0) {
        /*当缓存去为空时，将其填满*/
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            /*若该系统调用未被中断*/
            if (errno != EINTR) {
                return (-1);
            }
        }
        else {
            /*若已读至缓冲区尾部*/
            if (rp->rio_cnt == 0) {
                return (0);
            }
            else {
                /*重置缓冲区指针*/
                rp->rio_bufptr = rp->rio_buf;
            }
        }
    }

    /*cnt = min(n, rp->rio_cnt) 取最小值*/
    cnt = n;
    if (rp->rio_cnt < n) {
        cnt = rp->rio_cnt;
    }

    /*从缓冲区中复制cnt个字节存入usrbuf*/
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt    -= cnt;

    /*返回已读入的字节数*/
    return (cnt);
}


/*缓冲区初始化，使文件描述符与缓冲区相对应*/
void rio_readinitb(rio_t * rp, int fd) {
    rp->rio_fd     = fd;
    rp->rio_cnt    = 0;
    rp->rio_bufptr = rp->rio_buf;
}


/*从缓冲区rp中读取最多n个字节存入usrbuf(有缓冲)*/
ssize_t rio_readnb(rio_t * rp, void *usrbuf, size_t n) {
    size_t      nleft = n;
    ssize_t     nread;
    char      * bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = rio_read(rp, bufp, nleft)) < 0) {
            /*若其中的系统调用被中断*/
            if (errno == EINTR) {
                /*准备再次调用*/
                nread = 0;
            }
            else {
                /*其他错误情况*/
                return (-1);
            }
        }
        else {
            /*若已读至文件尾*/
            if (nread == 0) {
                break;
            }
        }
        nleft -= nread;
        bufp  += nread;
    }

    /*返回已读入的字节数*/
    return (n - nleft);
}


/*从缓冲区rp中读取最多maxlen个字节的文本行存入usrbuf(有缓冲)*/
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
            /*若已读至文件尾*/
            if (rc == 0) {
                if (n == 1) {
                    /*无缓存数据*/
                    return (0);
                }
                else {
                    /*存在被缓存的数据*/
                    break;
                }
            }
            else {
                /*其他错误情况*/
                return (-1);
            }
        }
    }
    *bufp = 0;

    /*返回已读入的字节数*/
    return (n);
}


/*函数rio_writen()的封装*/
void Rio_writen(int fd, void *usrbuf, size_t n) {
    if (rio_writen(fd, usrbuf, n) != n) {
        unix_error("Rio_writen error");
    }
}


/*函数rio_readlineb()的封装*/
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t     rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        unix_error("Rio_readlineb error");
    }

    return (rc);
}


/*将socket(),bind(),listen()函数合用，创建并返回在port端口上的监听描述符*/
int open_listenfd(int port) {
    struct sockaddr_in  serveraddr;
    int                 listenfd;
    int                 optval   = 1;

    /*创建socket描述符*/
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return (-1);
    }

    /*针对bind()错误的设置*/
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval, sizeof(int)) < 0) {
        return (-1);
    }

    /*让所有连接port端口的请求指向listenfd*/
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family      = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port        = htons((unsigned short) port);

    if (bind(listenfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) < 0) {
        return (-1);
    }

    /*将listenfd转为用于监听的socket*/
    if (listen(listenfd, LISTENQ) < 0) {
        return (-1);
    }

    /*返回描述符*/
    return (listenfd);
}


/*函数open_listenfd()的封装*/
int Open_listenfd(int port) {
    int     rc;

    if ((rc = open_listenfd(port)) < 0) {
        unix_error("Open_listenfd error");
    }

    return (rc);
}


/*将错误相关信息反馈给客户端*/
void client_error(int fd, char * cause, char * errnum,
                         char * shortmsg, char * longmsg) {
    char    buf   [MAXLINE];
    char    body  [MAXBUF];

    /*创建HTTP响应头*/
    sprintf(body, "<html><title>iShop Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The iShop Web server</em>\r\n", body);

    /*输出HTTP响应*/
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


/*在终端上显示调用此函数的次数和与第一次调用的时间间隔*/
void print_time(void) {
    struct timeval        tv;
    static struct timeval first;
    static long int       cnt = 0;

    ++cnt;

    /*第一次调用，设置初始时间first*/
    if (1 == cnt) {
        if (0 != gettimeofday(&first, NULL)) {
            app_error("gettimeofday error");
        }
    }

    /*获取当前时间*/
    if (0 != gettimeofday(&tv, NULL)) {
        app_error("gettimeofday error");
    }

    /*输出时间间隔*/
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


/*读取并显示HTTP报头*/
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


/*URI中提取文件名和CGI参数*/
int parse_uri(char * uri, char * filename, char * cgiargs) {
    char   * ptr;

    /*判断URI中是否含有cgi-bin字段*/
    if (NULL == strstr(uri, "cgi-bin")) {
        /*静态内容*/
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/') {
            strcat(filename, "index.htm");
        }

        return (1);
    }
    else {
        /*动态内容*/
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


/*从文件名中分析出MIME文件类型。(应避免使用love.gif.css这样的文件名)*/
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


/*提供静态内容服务*/
void serve_static(int fd, char * filename, int filesize) {
    int     srcfd;
    char  * srcp;
    char    filetype    [MAXLINE];
    char    buf         [MAXBUF];

    /*发送HTTP响应头至客户端*/
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: iShop Web Server\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    /*将被请求文件作为HTTP响应主体发送至客户端*/
    srcfd = Open(filename, O_RDONLY, 0);
    srcp  = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}


/*提供动态内容服务*/
void serve_dynamic(int fd, char * filename, char * cgiargs) {
    char    buf         [MAXLINE];
    char  * emptylist   []  = { NULL };

    /*发送HTTP响应头至客户端*/
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: iShop Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /*创建子进程*/
    if (Fork() == 0) {
        /*设置环境变量*/
        setenv("QUERY_STRING", cgiargs, 1);

        /*将标准输出重定向至客户端*/
        Dup2(fd, STDOUT_FILENO);

        /*在进程上下文中执行CGI程序*/
        Execve(filename, emptylist, environ);
    }

    /*父母进程等待子进程终止*/
    Wait(NULL);
}


/*HTTP事务处理函数*/
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

    /*读取HTTP请求行*/
    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);

    /*仅支持GET，若使用其他方法则反馈错误信息并返回main()*/
    if (strcasecmp(method, "GET")) {
        client_error(fd, method, "501", "Not Implemented",
                                 "iShop does not implement this method");
        return;
    }

    /*读取并处理HTTP报头*/
    read_request_headers(&rio);

    /*分析需要的是静态还是动态服务*/
    is_static = parse_uri(uri, filename, cgiargs);

    /*被请求的文件是否存在*/
    if (stat(filename, &sbuf) < 0) {
        client_error(fd, filename, "404", "Not found",
                                   "iShop couldn't find this file");
        return;
    }

    if (is_static) {
        /*静态内容*/
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            /*若文件无法访问*/
            client_error(fd, filename, "403", "Forbidden",
                                       "iShop couldn't read this file");
            return;
        }

        /*提供静态内容服务*/
        serve_static(fd, filename, sbuf.st_size);
    }
    else {
        /*动态内容*/
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            /*若文件无法访问*/
            client_error(fd, filename, "403", "Forbidden",
                                       "iShop couldn't run this CGI program");
            return;
        }

        /*提供动态内容服务*/
        serve_dynamic(fd, filename, cgiargs);
    }
}

int main(int argc, char ** argv) {
    struct sockaddr_in  clientaddr;
    int                 listenfd;
    int                 connfd;
    int                 port;
    int                 clientlen;

    /*默认使用8080端口*/
    if (argc != 2) {
        port = 8080;
    }
    else {
        port = atoi(argv[1]);
    }

    /*创建在port端口上的监听描述符*/
    listenfd = Open_listenfd(port);

    /*迭代式处理*/
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        play_with_me(connfd);
        Close(connfd);
    }

    return (0);
}


