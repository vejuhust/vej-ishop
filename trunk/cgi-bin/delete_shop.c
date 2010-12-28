#include "ishop_cgis.h"

#define STR_MAX     102400          /*临时字符串长度*/

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../cgi-bin/which_shop?delete\" />";

char    tmpstr  [STR_MAX];          /*临时字符串*/


/*错误处理函数*/
void error_handler(char * msg, char * msg2) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    if (NULL == msg2) {
        setenv("QUERY_STRING2", "../index.htm", 1);
    }
    else {
        setenv("QUERY_STRING2", msg2, 1);
    }

    Execve("cgi-bin/error", emptylist, environ);
}


/*将URL拆分、还原出CGI参数，按格式填入相应的Matrix指令*/
void dispense_url(char type, char * str, char * ins) {
    char  * head = str;
    char  * c_and;
    long    len;

    strcat(str, "&");
    *ins = type;
    c_and = strstr(str, "&");
    while (NULL != c_and) {

        /*以&为分隔符提取表单参数*/
        len = c_and - head;
        strncpy(tmpstr, head, len);
        tmpstr[len] = '\0';

        /*提取web_num*/
        if (NULL != strcasestr(tmpstr, "web_num")) {
            strcat(ins, strstr(tmpstr, "=") + 1);
            recovery_url(ins);

            head = strstr(ins, "#");
            if (NULL != head) {
                *(head) = '\n';
                strcat(ins, "\n");
                break;
            }

            strcat(ins, "\n");
        }
        else {
            /*提取shop_id*/
            if (NULL != strcasestr(tmpstr, "shop_id")) {
                strcat(ins, strstr(tmpstr, "=") + 1);
                recovery_url(ins);
                strcat(ins,"\n");
                break;
            }
            else {
                error_handler("select a shop record please.",
                              "../cgi-bin/which_shop?delete");
            }
        }

        head = c_and + 1;
        c_and  = strstr(head, "&");
    }
}


/*分析并还原经FIFO输出管道传递的Matrix反馈信息*/
void check_input(char * str, long len) {
    FILE * file;
    char   type;

    /*将反馈信息存入临时文件*/
    file = fopen(TMPFILE, "wb");
    fwrite(str, sizeof(char), len, file);
    fclose(file);

    /*从临时文件中重新按格式读取*/
    file = fopen(TMPFILE, "rb");
    fread(&type, sizeof(char), 1, file);

    /*若为错误反馈信息*/
    if ('Z' == type) {
        char * msg;

        fread(&len, sizeof(long), 1, file);
        msg = (char *) calloc(len + 5, sizeof(char));

        fread(msg, sizeof(char), len, file);

        fclose(file);
        remove(TMPFILE);

        error_handler(msg, NULL);
    }

    /*若为操作成功的反馈*/
    if ('H' != type) {
        error_handler("What the fuck?", NULL);
    }

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);
}


/*主函数*/
int main(int argc, char** argv) {
    char         * url = (char *) calloc(STR_MAX, sizeof(char));
    char         * ins = (char *) calloc(STR_MAX, sizeof(char) );
    long           len;
    int            output;
    int            input;

    /*从环境变量中获取CGI参数*/
    strcpy(url, getenv("QUERY_STRING"));
    
    /*从CGI参数中提取信息，生成Matrix指令*/
    dispense_url('H', url, ins);

    /*通过FIFO输入管道将指令提交给Matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*释放ins指令字符串所占用的堆空间*/
    free(ins);

    /*在FIFO输出管道上读取Matrix的反馈信息*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*分析并确认Matrix反馈信息*/
    check_input(tmpstr, len);

    /*向客户端发送meta refresh代码，引导浏览器返回调用网页*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}


