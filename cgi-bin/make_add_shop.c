#include "ishop_cgis.h"

#define STR_MAX     102400                          /*临时字符串长度*/
#define MODEL       "model/add_shop"                /*网页模板文件地址*/
#define PAGE        "baby/add_shop.htm"             /*生成的网页的地址*/

const char * ins  = "J\n\n\0";                      /*Matrix指令J*/

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/add_shop.htm\" />";

char            tmpstr  [STR_MAX];                  /*临时字符串*/

/*错误处理函数*/
void error_handler(char * msg) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    setenv("QUERY_STRING2", "../cgi-bin/make_add_shop", 1);

    Execve("cgi-bin/error", emptylist, environ);
}

/*主函数*/
int main(int argc, char** argv) {
    int     output;
    int     input;
    long    len;
    long    cnt;
    long    i;
    FILE  * file;
    char  * mark;
    char  * str = (char *) calloc(STR_MAX, sizeof(char));
    char  * new = (char *) calloc(STR_MAX, sizeof(char));
    char  * buf = (char *) calloc(STR_MAX, sizeof(char));
    char  * r0;
    char  * r1;
    char  * r2;

    /*通过FIFO输入管道将指令提交给Matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*在FIFO输出管道上读取Matrix的反馈信息*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*若为错误反馈信息*/
    if ('J' != tmpstr[0]) {
        error_handler("What's up?");
    }

    /*将反馈信息存入临时文件*/
    file = fopen(TMPFILE, "wb");
    fwrite(&tmpstr[1], sizeof(char), len, file);
    fclose(file);

    /*从临时文件中重新按格式读取*/
    file = fopen(TMPFILE, "rb");
    fread(&cnt, sizeof(long), 1, file);

    /*若为反馈数据为空*/
    if (0 == cnt) {
        fclose(file);
        remove(TMPFILE);
        error_handler("no website record, add one first.");
    }
    
    /*从临时文件中重新按格式读取*/
    fread(&len, sizeof(long), 1, file);
    fread(&tmpstr, sizeof(char), len, file);

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);

    /*载入网页模板*/
    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    /*寻找并处理网页模板中的标记*/
    mark  = strstr(str, m0);
    *mark = '\0';

    /*在标记处填入信息*/
    strcpy(new, str);
    r0 = tmpstr;
    for (i = 0; i < cnt; i++) {
        r1  = strstr(r0, "\n");
        *r1 = '\0';
        r1++;

        r2  = strstr(r1, "\n");
        *r2 = '\0';
        r2++;

        sprintf(buf, "<option value=\"%s\" >%s</option>\r\n\0", r0, r1);
        strcat(new, buf);

        r0 = r2;
    }

    mark += strlen(m0);
    strcat(new, mark);

    /*生成网页文件*/
    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);

    /*向客户端发送meta refresh代码，引导浏览器打开新生成的网页*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}


