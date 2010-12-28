#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


extern char     tmpstr  [];             /*临时字符串*/

/*UNIX类错误处理*/
int unix_error(char * msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

/*execve()系统调用的封装*/
void Execve(const char * filename, char * const argv[], char * const envp[]) {
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}

/*将URL中的以%xx方式表示的字符还原为ASCII字符*/
void recovery_url(char * str) {
    char * sp;
    long   i = 0;
    long   t = 0;
    char   c;

    /*将所有的加号替换为空格*/
    sp = str;
    while ('\0' != *sp) {
        if ('+' == *sp) {
            *sp = ' ';
        }
        ++sp;
    }

    /*依次处理每一个字符*/
    sp = str;
    while ('\0' != *sp) {

        /*是否为%开头的转义符*/
        if ('%' != *sp) {
            /*复制非转义符*/
            tmpstr[i++] = *sp;
            ++sp;
        }
        else {
            /*提取转义符第一位*/
            ++sp;
            c = *sp;
            if (0 != isdigit(c)) {
                /*处理十六进制数字*/
                t = (c - '0') * 16;
            }
            else {
                /*处理十六进制字母*/
                t = (tolower(c) - 'a' + 10) * 16;
            }

            /*提取转义符第二位*/
            ++sp;
            c = *sp;
            if (0 != isdigit(c)) {
                /*处理十六进制数字*/
                t += (c - '0');
            }
            else {
                /*处理十六进制字母*/
                t += (tolower(c) - 'a' + 10);
            }

            /*保存转换后的结果*/
            tmpstr[i++] = (char) t;
            ++sp;
        }
    }
    tmpstr[i] = '\0';

    /*将转换结果写回原字符串*/
    strcpy(str, tmpstr);
}


