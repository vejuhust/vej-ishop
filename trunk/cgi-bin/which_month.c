#include "ishop_cgis.h"

#define STR_MAX     102400                          /*Matrix指令参数的最大个数*/
#define MODEL       "model/which_month"             /*网页模板文件地址*/
#define PAGE        "baby/which_month.htm"          /*生成的网页的地址*/

const char * ins  = "V\n\n\0";                      /*Matrix指令V*/

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/which_month.htm\" />";

char        tmpstr  [STR_MAX];                      /*临时字符串*/


/*错误处理函数*/
void error_handler(char * msg) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    setenv("QUERY_STRING2", "../index.htm", 1);

    Execve("cgi-bin/error", emptylist, environ);
}


/*主函数*/
int main(int argc, char** argv) {
    int     output;
    int     input;
    long    len;
    long    min;
    long    max;
    long    i;
    FILE  * file;
    char  * mark;
    char  * str = (char *) calloc(STR_MAX, sizeof(char));
    char  * new = (char *) calloc(STR_MAX, sizeof(char));
    char  * buf = (char *) calloc(STR_MAX, sizeof(char));

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
    if ('Z' == tmpstr[0]) {
        char * msg;

        file = fopen(TMPFILE, "wb");
        fwrite(&tmpstr[1], sizeof(char), len, file);
        fclose(file);

        file = fopen(TMPFILE, "rb");

        fread(&len, sizeof(long), 1, file);
        msg = (char *) calloc(len + 5, sizeof(char));

        fread(msg, sizeof(char), len, file);

        fclose(file);
        remove(TMPFILE);

        error_handler(msg);
    }

    /*若为反馈信息发生错误*/
    if ('V' != tmpstr[0]) {
        error_handler("What's up?");
    }

    /*将反馈信息存入临时文件*/
    file = fopen(TMPFILE, "wb");
    fwrite(&tmpstr[1], sizeof(char), len, file);
    fclose(file);

    /*从临时文件中重新按格式读取*/
    file = fopen(TMPFILE, "rb");
    fread(&min, sizeof(long), 1, file);
    fread(&max, sizeof(long), 1 ,file);

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);

    /*载入网页模板*/
    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    /*寻找并处理网页模板中的标记*/
    *new = '\0';
    mark  = strstr(str, m0);
    *mark = '\0';
    strcat(new, str);

    /*在标记零处填入信息*/
    for (i = min; i <= max; i++) {
        sprintf(buf, "<option value=\"%ld\" >%ld</option>\r\n\0", i, i);
        strcat(new, buf);
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


