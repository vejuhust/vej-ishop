#include "ishop_cgis.h"

#define PART_MAX    20                          /*Matrix指令参数的最大个数*/
#define STR_MAX     102400                      /*临时字符串长度*/
#define MODEL       "model/top_five"            /*网页模板文件地址*/
#define PAGE        "baby/top_five.htm"         /*生成的网页的地址*/

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/top_five.htm\" />";

char            tmpstr  [STR_MAX];              /*临时字符串*/
char            year    [100];                  /*待查询年份字符串*/
long            month;                          /*待查询月份*/
struct top5_t   state   [10];                   /*全部Top节点*/


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

        /*提取year*/
        if (NULL != strcasestr(tmpstr, "year")) {
            strcat(ins, strstr(tmpstr, "=") + 1);

            /*还原转义符*/
            recovery_url(ins);

            head = strstr(ins, "#");
            if (NULL != head) {
                *(head) = '\n';
                strcat(ins, "\n");
                break;
            }

            if ('\0' == *(ins + 1) ) {
                error_handler("select the year please.",
                              "../cgi-bin/which_month");
            }

            strcpy(year, ins + 1);
            strcat(ins, "\n");
        }
        else {
            /*提取month*/
            if (NULL != strcasestr(tmpstr, "month")) {
                long  end = strlen(ins);
                strcat(ins, strstr(tmpstr, "=") + 1);

                /*还原转义符*/
                recovery_url(ins);

                if ('\0' == *(ins + end) ) {
                    error_handler("select the month please.",
                                  "../cgi-bin/which_month");
                }

                month = atoi(ins + end);
                strcat(ins,"\n");
                break;
            }
            else {
                error_handler("select a date please.",
                              "../cgi-bin/which_month");
            }
        }

        head = c_and + 1;
        c_and  = strstr(head, "&");
    }
}


/*分析并还原经FIFO输出管道传递的Matrix反馈信息*/
void transfer_input2(char * str, long len, char first, void * aim, long *cnt) {
    FILE  * file;
    char    type;

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
    if (first == type) {
        fread(cnt, sizeof(long), 1, file);

        if (0 == *cnt) {
            error_handler("no website records.", NULL);
        }

        fread(&len, sizeof(long), 1, file);
        fread(aim, len, *cnt, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);
}


/*将获得所有Top信息填入网页模板*/
void generate_state(long cnt) {
    FILE * file;
    char * mark0;
    char * mark1;
    char * word1 = (char *) calloc(STR_MAX, sizeof(char));
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));
    long   i;

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
    sprintf(word1, "%s, %s", month_list_abbr[month], year);
    strcat(new, word1);
    mark0 += strlen(m0);
    strcat(new, mark0);

    /*在标记一处填入信息*/
    if (0 == state[0].cnt) {
        sprintf(word1, "<p>No Orders in %s, %s.</p>",
                        month_list_abbr[month], year );
        strcat(new, word1);
    }
    else {
        strcat(new, "<table width=\"95%\"><caption></caption><tr><th>No.</th>"
                    "<th>Websites</th><th>Times</th></tr>" );
        for (i = 0; i < cnt; i++) {
            if (0 == state[i].cnt) {
                break;
            }

            sprintf(word1,
                    "<tr %s>"
                        "<td>%ld</td>"
                        "<td><a href=\"../cgi-bin/view_web?web_num=%ld&"
                        "submit=OK\">%s</a></td>"
                        "<td>%ld</td>"
                    "</tr>",
                    (i & 0x1)?(""):("class=\"odd\""), i + 1, state[i].web_num,
                    state[i].web_name, state[i].cnt );
            strcat(new, word1);
        }
        strcat(new, "</table>");
    }

    mark1 += strlen(m1);
    strcat(new, mark1);

    /*生成网页文件*/
    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
}


/*主函数*/
int main(int argc, char** argv) {
    char         * url = (char *) calloc(STR_MAX, sizeof(char));
    char         * ins = (char *) calloc(STR_MAX, sizeof(char) );
    long           len;
    long           cnt;
    int            output;
    int            input;

    /*从环境变量中获取CGI参数*/
    strcpy(url, getenv("QUERY_STRING"));

    /*从CGI参数中提取信息，生成Matrix指令*/
    dispense_url('S', url, ins);

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

    /*分析并还原Matrix反馈信息，获得全部Top节点数据*/
    transfer_input2(tmpstr, len, 'S', state, &cnt);

    /*将获得所有Top节点信息填入网页模板*/
    generate_state(cnt);

    /*向客户端发送meta refresh代码，引导浏览器打开新生成的网页*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}


