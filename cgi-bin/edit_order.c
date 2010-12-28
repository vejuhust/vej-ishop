#include "ishop_cgis.h"

#define PART_MAX    20                          /*Matrix指令参数的最大个数*/
#define STR_MAX     102400                      /*临时字符串长度*/
#define MODEL       "model/edit_done"           /*网页模板文件地址*/
#define PAGE        "baby/edit_done.htm"        /*生成的网页的地址*/

/*Matrix指令F格式*/
char * item[] = {
    "old1",
    "old2",
    "old3",
    "web_num",
    "pay",
    "money",
    "day",
    "month",
    "year",
    "order_area",
    NULL
};

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/edit_done.htm\" />";

char            tmpstr      [STR_MAX];          /*临时字符串*/
struct shop_t   old_shop;                       /*修改前的Shop链节点*/
struct shop_t   shop;                           /*修改后的Shop链节点*/
struct order_t  old_order;                      /*修改前的Order链节点*/
struct order_t  order;                          /*修改后的Order链节点*/


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
                if (3 == i) {
                    *(strstr(part[i], "#")) = '\n';
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


/*错误处理函数*/
void error_handler(char * msg) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    setenv("QUERY_STRING2", "../cgi-bin/which_order?edit", 1);

    Execve("cgi-bin/error", emptylist, environ);
}


/*分析并还原经FIFO输出管道传递的Matrix反馈信息*/
void transfer_input(char * str, long len, char first, void * aim) {
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

        error_handler(msg);
    }

    /*若为操作成功的反馈*/
    if (first == type) {
        fread(&len, sizeof(long), 1, file);
        fread(aim, len, 1, file);
    }
    else {
        error_handler("What the fuck?");
    }

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);
}


/*根据Order节点的信息查询Shop记录*/
void get_shop_name(struct order_t * order, struct shop_t * shop) {
    char  * str = (char *) calloc(STR_MAX, sizeof(char) );
    int     output;
    int     input;
    long    len;

    /*生成查询指令*/
    sprintf(str, "N%ld\n%s\n\0", order->web_num, order->shop_id);

    /*将指令通过FIFO输入管道传递给Matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, str, strlen(str));
    close(output);

    /*等待并打开FIFO输出管道，读取Matrix反馈信息*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, str, STR_MAX);
    close(input);
    str[len] = '\0';

    /*对反馈信息进行分析处理*/
    transfer_input(str, len, 'N', shop);

    /*释放str指令字符串所占用的堆空间*/
    free(str);
}


/*将修改前后的Order节点信息填入网页模板*/
void generate_edit_done(void) {
    FILE * file;
    char * mark0;
    char * mark1;
    char * mark2;
    char * word1 = (char *) calloc(STR_MAX, sizeof(char));
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));

    /*载入网页模板*/
    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    /*寻找并处理网页模板中的标记*/
    mark0 = strstr(str, m0);
    mark1 = strstr(str, m1);
    mark2 = strstr(str, m2);

    *mark0 = '\0';
    *mark1 = '\0';
    *mark2 = '\0';

    /*在标记零处填入信息*/
    strcpy(new, str);
    strcat(new, "shop");
    mark0 += strlen(m0);
    strcat(new, mark0);

    /*在标记一处填入信息*/
    sprintf(word1,
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Shop</td>"
            "<td width=\"5%%\">old</td>"
            "<td><a href=\"../cgi-bin/view_shop?web_num=%ld&shop_id=%s&"
            "submit=OK\">%s</a></td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td><a href=\"../cgi-bin/view_shop?web_num=%ld&shop_id=%s&"
            "submit=OK\">%s</a></td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Payment Method</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Sum</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%.2Lf</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%.2Lf</td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Date</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%s %ld, %ld</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%s %ld, %ld</td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Location</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Last Modified</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%s</td>"
        "</tr>",
         old_shop.web_num, old_shop.shop_id, old_shop.shop_name,
         shop.web_num, shop.shop_id, shop.shop_name,
         pay_list[old_order.pay], pay_list[order.pay], old_order.money,
         order.money, month_list[old_order.date.month], old_order.date.day,
         old_order.date.year, month_list[order.date.month], order.date.day,
         order.date.year, area_list[old_order.order_area],
         area_list[order.order_area], strstr(old_order.recent, "_") + 1,
         strstr(order.recent, "_") + 1 );

    strcat(new, word1);
    mark1 += strlen(m1);
    strcat(new, mark1);

    /*在标记二处填入信息*/
    sprintf(word1, "../cgi-bin/handle_order?web_num=%ld&shop_id=%s&submit=OK",
                  old_order.web_num, old_order.shop_id);
    strcat(new, word1);
    mark2 += strlen(m2);
    strcat(new, mark2);

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
    int            output;
    int            input;

    /*从环境变量中获取CGI参数*/
    strcpy(url, getenv("QUERY_STRING"));

    /*从CGI参数中提取信息，生成Matrix指令*/
    dispense_url('O', url, item, 10, ins);

    /*通过FIFO输入管道将指令提交给Matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*在FIFO输出管道上读取Matrix的反馈信息*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*分析并还原Matrix反馈信息，获取修改前的Order节点数据*/
    transfer_input(tmpstr, len, 'O', &old_order);

    /*根据修改前的Order节点的信息查询Shop记录*/
    get_shop_name(&old_order, &old_shop);

    /*将指令修改为F*/
    *ins = 'F';

    /*通过FIFO输入管道将指令提交给Matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*在FIFO输出管道上读取Matrix的反馈信息*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*分析并还原Matrix反馈信息，获取修改后的Order节点数据*/
    transfer_input(tmpstr, len, 'F', &order);

    /*根据修改后的Order节点的信息查询Shop记录*/
    get_shop_name(&order, &shop);

    /*生成目标网页*/
    generate_edit_done();

    /*向客户端发送meta refresh代码，引导浏览器打开新生成的网页*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}


