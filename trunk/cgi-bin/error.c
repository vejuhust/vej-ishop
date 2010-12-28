#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_MAX     102400                      /*临时字符串长度*/
#define MODEL       "model/error"               /*网页模板文件地址*/
#define PAGE        "baby/error.htm"            /*生成的网页的地址*/

/*最终向客户端发送的meta refresh代码*/
const char * s  = "<meta http-equiv=\"refresh\" content=\"0;"
                  "url=../baby/error.htm\" />";

/*model文件中数据填入的标记*/
const char * m0 = "[[0]]";
const char * m1 = "[[1]]";
const char * de = "../index.htm";

/*主函数*/
int main() {
    FILE * file;
    char * mark0;
    char * mark1;
    char * word0;
    char * word1;
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));

    /*从环境变量中获取CGI参数*/
    word0 = getenv("QUERY_STRING");
    word1 = getenv("QUERY_STRING2");

    /*若参数一不存在，则填入默认值*/
    if (NULL == word0) {
        word0 = (char *) calloc(100, sizeof(char));
        strcpy(word0, "we don't know what happened either :(");
    }

    /*若参数二不存在，则填入默认值*/
    if (NULL == word1) {
        word1 = de;
    }

    /*载入网页模板*/
    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    /*寻找并处理网页模板中的标记*/
    mark0 = strstr(str, m0);
    mark1 = strstr(str, m1);

    *mark0 = '\0';
    *mark1 = '\0';

    /*在标记零处填入信息*/
    strcpy(new, str);
    strcat(new, word0);
    mark0 += strlen(m0);
    strcat(new, mark0);

    /*在标记一处填入信息*/
    strcat(new, word1);
    mark1 += strlen(m1);
    strcat(new, mark1);

    /*生成网页文件*/
    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);

    /*向客户端发送meta refresh代码，引导浏览器打开新生成的网页*/
    printf("Content-length: %d\r\n", strlen(s));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", s);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}


