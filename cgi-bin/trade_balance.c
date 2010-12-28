#include "ishop_cgis.h"

#define PART_MAX    20                          /*Matrix指令参数的最大个数*/
#define STR_MAX     102400                      /*临时字符串长度*/
#define MODEL       "model/trade_balance"       /*网页模板文件地址*/
#define PAGE        "baby/trade_balance.htm"    /*生成的网页的地址*/

/*Matrix指令T格式*/
char * item[] = {
    "year",
    "month",
    "area_a",
    "area_b",
    NULL
};

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/trade_balance.htm\" />";

char        tmpstr   [STR_MAX];                 /*临时字符串*/
char        year     [100];                     /*待查询年份字符串*/
long        month;                              /*待查询月份*/
long        area_a;                             /*待查询地区A*/
long        area_b;                             /*待查询地区B*/


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
void dispense_url(char type, char * str, char * item[], long cnt, char * ins) {
    char  * head = str;
    char  * c_and;
    long    i;
    long    len;
    char  * part[PART_MAX];

    /*清空part数组*/
    memset(part, 0, PART_MAX * sizeof(char *));
    strcat(str, "&");

    c_and = strstr(str, "&");
    while (NULL != c_and) {

        /*以&为分隔符提取表单参数*/
        len = c_and - head;
        strncpy(tmpstr, head, len);
        tmpstr[len] = '\0';

        for (i = 0; i < cnt; i++) {

            /*按参数名称依次提取*/
            if (NULL != strcasestr(tmpstr, item[i])) {
                part[i] = (char *) calloc(len + 2, sizeof(char));
                strcpy(part[i], strstr(tmpstr, "=") + 1);

                /*还原转义符*/
                recovery_url(part[i]);

                switch (i) {
                    case 0 :
                        strcpy(year, part[0]);
                        break;
                    case 1 :
                        month = atoi(part[1]);
                        break;
                    case 2 :
                        area_a = atoi(part[2]);
                        break;
                    case 3 :
                        area_b = atoi(part[3]);
                        break;
                }

                break;
            }
        }
        head = c_and + 1;
        c_and  = strstr(head, "&");
    }

    /*将结果按格式写入ins字符串*/
    *ins = type;
    for (i = 0; i < cnt; i++) {
        if (NULL != part[i]) {
            strcat(ins, part[i]);
            free(part[i]);
        }
        strcat(ins, "\n");
    }
}


/*分析并还原经FIFO输出管道传递的Matrix反馈信息*/
void transfer_input2(char * str, long len, char first, void * aim) {
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
        fread(&len, sizeof(long), 1, file);
        fread(aim, len, 1, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);
}


/*根据所获得的信息结合网页模板生成结果页面*/
void generate_result(long double sum) {
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
    sprintf(word1, "%s and %s", area_list[area_a], area_list[area_b]);
    strcat(new, word1);
    mark0 += strlen(m0);
    strcat(new, mark0);

    /*在标记一处填入信息*/
    if (fabs(sum) < 1e-6) {
        /*贸易平衡*/
        sprintf(word1, "In %s %s, %s and %s made the trade balance.",
          month_list_abbr[month], year, area_list[area_a], area_list[area_b]);
    }
    else {
        if (sum > 0) {
            /*贸易顺差*/
            sprintf(word1, "In %s %s, %s had trade surplus to %s.",
            month_list_abbr[month], year, area_list[area_a], area_list[area_b]);
        }
        else {
            /*贸易逆差*/
            sprintf(word1, "In %s %s, %s had trade deficit to %s.",
            month_list_abbr[month], year, area_list[area_a], area_list[area_b]);
        }
    }

    strcat(new, word1);
    mark1 += strlen(m1);
    strcat(new, mark1);

    /*生成网页文件*/
    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
}


/*判断输入数据是否齐全*/
int empty_exist(char * str) {
    char * p = str + 1;

    while ('\0' != *p) {
        if ( ('\n' == *(p-1)) && ('\n' == *(p)) ) {
            return (1);
        }
        p++;
    }

    return (0);
}


/*主函数*/
int main(int argc, char** argv) {
    char         * url = (char *) calloc(STR_MAX, sizeof(char));
    char         * ins = (char *) calloc(STR_MAX, sizeof(char) );
    long           len;
    long           cnt;
    int            output;
    int            input;
    long double    sum;

    /*从环境变量中获取CGI参数*/
    strcpy(url, getenv("QUERY_STRING"));

    /*从CGI参数中提取信息，生成Matrix指令*/
    dispense_url('T', url, item, 4, ins);

    /*判断输入数据是否齐全*/
    if (1 == empty_exist(ins)) {
        error_handler("input missing.", NULL);
    }

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
    transfer_input2(tmpstr, len, 'T', &sum);

    /*根据所获得的信息结合网页模板生成结果页面*/
    generate_result(sum);

    /*向客户端发送meta refresh代码，引导浏览器打开新生成的结果页面*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}



