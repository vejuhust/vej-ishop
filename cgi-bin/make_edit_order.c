#include "ishop_cgis.h"

#define PART_MAX    20                            /*Matrix指令参数的最大个数*/
#define STR_MAX     102400                        /*临时字符串长度*/
#define MODEL       "model/edit_order"            /*网页模板文件地址*/
#define PAGE        "baby/edit_order.htm"         /*生成的网页的地址*/

/*Matrix指令O格式*/
char * item[] = {
    "web_num",
    "shop_id",
    "order_num",
    NULL
};

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/edit_order.htm\" />";

char            tmpstr  [STR_MAX];                /*临时字符串*/
char            clue    [7]   = "[[0]]\0";        /*model文件中数据填入的标记*/
struct shop_t   shop;                             /*Shop链节点*/
struct order_t  order;                            /*Order链节点*/


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
void transfer_input(char * str, long len) {
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
    if ('O' == type) {
        fread(&len, sizeof(long), 1, file);
        fread(&order, len, 1, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);
}


/*获取所有Shop记录的信息列表*/
void get_shop_list(char * aim) {
    int     output;
    int     input;
    long    len;
    long    cnt;
    long    num[WEB_MAX];
    long    i;
    long    j;
    char    ins[] = "K\n\n";
    FILE  * file;
    char  * r0;
    char  * r1;
    char  * r2;
    char  * new = (char *) calloc(STR_MAX, sizeof(char));
    char  * buf = (char *) calloc(STR_MAX, sizeof(char));
    char  * web = (char *) calloc(STR_MAX, sizeof(char));

    /*将指令通过FIFO输入管道传递给Matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*等待并打开FIFO输出管道，读取Matrix反馈信息*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*若为错误反馈信息*/
    if ('K' != tmpstr[0]) {
        error_handler("What's happened?", NULL);
    }

    /*将反馈信息存入临时文件*/
    file = fopen(TMPFILE, "wb");
    fwrite(&tmpstr[1], sizeof(char), len, file);
    fclose(file);

    /*从临时文件中重新按格式读取*/
    file = fopen(TMPFILE, "rb");
    fread(&cnt, sizeof(long), 1,   file);
    fread(&num, sizeof(long), cnt, file);

    /*若为反馈数据为空*/
    if (0 == cnt) {
        fclose(file);
        remove(TMPFILE);
        error_handler("no website record, add one first!", NULL);
    }

    /*从临时文件中重新按格式读取*/
    fread(&len, sizeof(long), 1,   file);
    fread(&tmpstr, sizeof(char), len, file);

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);

    /*判断反馈数据是否为空*/
    j = 0;
    for (i = 0; i < cnt; i++) {
        if (0 != num[i]) {
            j = 1;
            break;
        }
    }

    if (0 == j) {
        error_handler("no shop record, add one first!", NULL);
    }

    /*根据信息生成网页代码串*/
    r0 = tmpstr;
    for (i = 0; i < cnt; i++) {
        r1  = strstr(r0, "\n");
        *r1 = '\0';
        r1++;

        r2  = strstr(r1, "\n");
        *r2 = '\0';
        r2++;

        if (0 != num[i]) {
            sprintf(buf, "<optgroup label = \"%s\">\r\n\0", r1);
            strcpy(web, r0);
            strcat(new, buf);

            r0 = r2;
            for (j = 0; j < num[i]; j++) {
                r1  = strstr(r0, "\n");
                *r1 = '\0';
                r1++;

                r2  = strstr(r1, "\n");
                *r2 = '\0';
                r2++;

                if ( (order.web_num == atoi(web)) &&
                     (0 ==strcasecmp(order.shop_id, r0)) ) {
                    sprintf(buf, "<option selected value=\"%s#%s\">%s"
                                 "</option>\r\n\0", web, r0, r1);
                }
                else {
                    sprintf(buf, "<option value=\"%s#%s\" >%s</option>\r\n\0",
                            web, r0, r1);
                }
                strcat(new, buf);

                r0 = r2;
            }
            strcat(new, "</optgroup>\r\n\0");
        }
        else {
            r0 = r2;
        }
    }

    strcpy(aim, new);
}


/*将待修改的Order节点信息填入网页模板*/
void generate_edit_order(void) {
    FILE * file;
    char * mark[15];
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));
    char * buf = (char *) calloc(STR_MAX, sizeof(char));
    long i;

    /*载入网页模板*/
    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    /*寻找并处理网页模板中的标记*/
    for (i = 0; i < 11; i++) {
        clue[2] = i + '0';
        mark[i] = strstr(str, clue);
    }

    for (i = 0; i < 11; i++) {
        *(mark[i]) = '\0';
    }

    /*在标记零处填入信息*/
    strcpy(new, str);
    get_shop_list(buf);
    strcat(new, buf);
    mark[0] += 5;

    /*在标记一处填入信息*/
    strcat(new, mark[0]);
    for (i = 1; i <= 4; i++) {
        if (i == order.pay) {
            sprintf(buf,"<option selected value=\"%ld\" >%s</option>",
                        i, pay_list[i]);
        }
        else {
            sprintf(buf,"<option value=\"%ld\" >%s</option>", i, pay_list[i]);
        }
        strcat(new, buf);
    }
    mark[1] += 5;

    /*在标记二处填入信息*/
    strcat(new, mark[1]);
    sprintf(buf, "%.2Lf", order.money);
    strcat(new, buf);
    mark[2] += 5;

    /*在标记三处填入信息*/
    strcat(new, mark[2]);
    sprintf(buf, "%.2ld", order.date.month);
    strcat(new, buf);
    mark[3] += 5;

    /*在标记四处填入信息*/
    strcat(new, mark[3]);
    sprintf(buf, "%.2ld", order.date.day);
    strcat(new, buf);
    mark[4] += 5;

    /*在标记五处填入信息*/
    strcat(new, mark[4]);
    sprintf(buf, "%.4ld", order.date.year);
    strcat(new, buf);
    mark[5] += 5;

    /*在标记六处填入信息*/
    strcat(new, mark[5]);
    for (i = 1; i <= 34; i++) {
        if (i == order.order_area) {
            sprintf(buf,"<option selected value=\"%ld\" >%s</option>",
                        i, area_list[i]);
        }
        else {
            sprintf(buf,"<option value=\"%ld\" >%s</option>", i, area_list[i]);
        }
        strcat(new, buf);
    }
    mark[6] += 5;

    /*在标记七处填入信息*/
    strcat(new, mark[6]);
    sprintf(buf, "%ld", order.web_num);
    strcat(new, buf);
    mark[7] += 5;

    /*在标记八处填入信息*/
    strcat(new, mark[7]);
    strcat(new, order.shop_id);
    mark[8] += 5;

    /*在标记九处填入信息*/
    strcat(new, mark[8]);
    sprintf(buf, "%ld", order.order_num);
    strcat(new, buf);
    mark[9] += 5;

    /*在标记十处填入信息*/
    strcat(new, mark[9]);
    sprintf(buf, "../cgi-bin/handle_order?web_num=%ld&shop_id=%s&submit=OK",
                  order.web_num, order.shop_id);
    strcat(new, buf);
    mark[10] += 5;
    strcat(new, mark[10]);

    /*生成网页文件*/
    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
}


/*主函数*/
int main(int argc, char** argv) {
    int     output;
    int     input;
    long    len;
    char  * ins = (char *) calloc(STR_MAX, sizeof(char) );
    char  * url = (char *) calloc(STR_MAX, sizeof(char));

    /*从环境变量中获取CGI参数*/
    strcpy(url, getenv("QUERY_STRING"));

    /*从CGI参数中提取信息，生成Matrix指令*/
    dispense_url('O', url, item, 3, ins);

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

    /*分析并还原Matrix反馈信息，获取待修改的Order节点数据*/
    transfer_input(tmpstr, len);

    /*生成目标网页*/
    generate_edit_order();

    /*向客户端发送meta refresh代码，引导浏览器打开新生成的网页*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}


