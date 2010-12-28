#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>


#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FIFO_IN     "/tmp/iShopFIFOinput"
#define FIFO_OUT    "/tmp/iShopFIFOoutput"
#define MAX_MSG     1024000
#define MAX_LINE    18


char    msg [MAX_MSG];


char * timestr(void) {
    static int      cnt = 0;
    char          * str;
    struct timeval  t1;
    struct tm     * t2;

    gettimeofday(&t1, NULL);
    t2 = localtime(&t1.tv_sec);
    str = (char *) calloc(100, sizeof(char));

    sprintf(str, "%.4d_%4d-%.2d-%.2d %.2d:%.2d:%.2d.%.6ld", ++cnt,
                t2->tm_year + 1900, t2->tm_mon + 1, t2->tm_mday,
                t2->tm_hour, t2->tm_min, t2->tm_sec, (long) t1.tv_usec);

    return (str);
}


void read_display(int file) {
    long    len;
    long    i;
    long    j;
    char    buf [100];
    
    len = read(file, msg, MAX_MSG);
    msg[len] = '\0';

    printf("length = %ld\n", len);

    for (i = 0; i < len; i++) {
        sprintf(buf, "%2x\0", msg[i]);
        printf("%c%c ", buf[strlen(buf) - 2], buf[strlen(buf) - 1]);

        if (0 == (i+1) % MAX_LINE) {
            putchar(' ');
            putchar('|');
            for (j = i + 1 - MAX_LINE; j <= i; j++) {
                if (msg[j] > 31) {
                    putchar(msg[j]);
                }
                else {
                    putchar('.');
                }
            }
            putchar('|');
            putchar('\n');
        }
    }

    i = MAX_LINE - (len % MAX_LINE);
    if (0 != i) {
        for (j = 0; j < i; j++) {
            putchar(' ');
            putchar(' ');
            putchar(' ');
        }

        putchar(' ');
        putchar('|');
        for (j = len - (len % MAX_LINE); j < len; j++) {
            if (msg[j] > 31) {
                putchar(msg[j]);
            }
            else {
                putchar('.');
            }
        }
        putchar('|');
        putchar('\n');
    }
}


int main(int argc, char** argv) {
    int     input;
    int     output;
    long    len;
    int     i;

    setbuf(stdout, NULL);

    printf("%s: ", timestr());
    while ( 0 != strcmp(fgets(msg, MAX_MSG, stdin), "quit\n") ) {
        msg[strlen(msg) - 1] = '\0';
        len = strlen(msg);
        for (i = 0; i < len; i++) {
            if ('#' == msg[i]) {
                msg[i] = '\n';
            }
        }

        output = open(FIFO_IN, O_WRONLY | O_TRUNC);
        printf("%s: Opened FIFO_IN\n", timestr());
        
        write(output, msg,  len);
        close(output);

        printf("%s: Wrote FIFO_IN\n", timestr());

        input  = open(FIFO_OUT, O_RDONLY);
        printf("%s: Opened FIFO_OUT\n", timestr());
        
        printf("%s: Read FIFO_OUT\n", timestr());
        read_display(input);
        close(input);

        printf("%s: ", timestr());
    }

    return (EXIT_SUCCESS);
}

