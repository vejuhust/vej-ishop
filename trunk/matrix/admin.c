#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) /*FIFO文件访问权限*/
#define FIFO_IN     "/tmp/iShopFIFOinput"                   /*FIFO输入管道*/
#define FIFO_OUT    "/tmp/iShopFIFOoutput"                  /*FIFO输出管道*/
#define MAX_MSG     1024000                                 /*指令最大长度*/
#define MAX_LINE    18                                      /*单行显示最多字节数*/

char    msg [MAX_MSG];                                      /*指令字符串*/

/*返回带操作序列号的时间字符串，精确到微秒*/
char * timestr(void) {
    static int      cnt = 0;
    char          * str;
    struct timeval  t1;
    struct tm     * t2;

    /*获取当前时间*/
    gettimeofday(&t1, NULL);
    t2 = localtime(&t1.tv_sec);
    str = (char *) calloc(100, sizeof(char));

    /*将当前时间整理后写入字符串str*/
    sprintf(str, "%.4d_%4d-%.2d-%.2d %.2d:%.2d:%.2d.%.6ld", ++cnt,
                t2->tm_year + 1900, t2->tm_mon + 1, t2->tm_mday,
                t2->tm_hour, t2->tm_min, t2->tm_sec, (long) t1.tv_usec);

    /*返回字符串首地址*/
    return (str);
}

/*从FIFO输出管道读入反馈信息，并以十六进制方式显示*/
void read_display(int file) {
    long    len;
    long    i;
    long    j;
    char    buf [100];

    /*从FIFO输出管道读入反馈信息*/
    len = read(file, msg, MAX_MSG);
    msg[len] = '\0';

    /*显示信息长度*/
    printf("length = %ld\n", len);

    for (i = 0; i < len; i++) {

        /*以十六进制方式显示*/
        sprintf(buf, "%2x\0", msg[i]);
        printf("%c%c ", buf[strlen(buf) - 2], buf[strlen(buf) - 1]);

        /*若单行排满*/
        if (0 == (i+1) % MAX_LINE) {
            putchar(' ');
            putchar('|');

            /*在屏幕上显示可打印字符*/
            for (j = i + 1 - MAX_LINE; j <= i; j++) {
                if (msg[j] > 31) {
                    /*显示可打印字符*/
                    putchar(msg[j]);
                }
                else {
                    /*用.代替不可打印字符*/
                    putchar('.');
                }
            }

            putchar('|');
            putchar('\n');
        }
    }

    /*若结束时单行并未排满*/
    i = MAX_LINE - (len % MAX_LINE);
    if (0 != i) {
        for (j = 0; j < i; j++) {
            putchar(' ');
            putchar(' ');
            putchar(' ');
        }

        putchar(' ');
        putchar('|');

        /*显示剩下的字符*/
        for (j = len - (len % MAX_LINE); j < len; j++) {
            if (msg[j] > 31) {
                /*显示可打印字符*/
                putchar(msg[j]);
            }
            else {
                /*用.代替不可打印字符*/
                putchar('.');
            }
        }

        putchar('|');
        putchar('\n');
    }
}

/*主函数*/
int main(int argc, char** argv) {
    int     input;
    int     output;
    long    len;
    int     i;

    /*关闭Standard I/O库的缓冲，避免提示符显示不同步*/
    setbuf(stdout, NULL);

    /*按条处理指令，直到收到quit要求退出*/
    printf("%s: ", timestr());
    while ( 0 != strcmp(fgets(msg, MAX_MSG, stdin), "quit\n") ) {

        /*读入用户指令*/
        msg[strlen(msg) - 1] = '\0';
        len = strlen(msg);

        /*将指令中的#替换成Matrix要求的换行符*/
        for (i = 0; i < len; i++) {
            if ('#' == msg[i]) {
                msg[i] = '\n';
            }
        }

        /*打开FIFO输入管道*/
        output = open(FIFO_IN, O_WRONLY | O_TRUNC);
        printf("%s: Opened FIFO_IN\n", timestr());

        /*写入并关闭用户输入的指令*/
        write(output, msg,  len);
        close(output);

        /*提示FIFO管道输入完成，等待Matrix进行处理*/
        printf("%s: Wrote FIFO_IN\n", timestr());

        /*等待并打开FIFO输出管道*/
        input  = open(FIFO_OUT, O_RDONLY);
        printf("%s: Opened FIFO_OUT\n", timestr());
        printf("%s: Read FIFO_OUT\n", timestr());

        /*读取并显示Matrix反馈信息*/
        read_display(input);
        close(input);

        /*显示输入信息提示符*/
        printf("%s: ", timestr());
    }

    /*程序正常结束*/
    return (EXIT_SUCCESS);
}


