#include "ishop_cgis.h"

#define PART_MAX    20                          /*Matrix指令参数的最大个数*/
#define STR_MAX     102400                      /*临时字符串长度*/
#define MODEL       "model/view_done"           /*网页模板文件地址*/
#define PAGE        "baby/view_done.htm"        /*生成的网页的地址*/

const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/view_done.htm\" />";

char            tmpstr  [STR_MAX];              /*临时字符串*/
struct web_t    web;                            /*Website链节点*/
struct shop_t   shop;                           /*Shop链节点*/


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

            /*还原转义符*/
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

                /*还原转义符*/
                recovery_url(ins);
                strcat(ins,"\n");
                break;
            }
            else {
                error_handler("select a shop record please.",
                              "../cgi-bin/which_shop?v");
            }
        }

        head = c_and + 1;
        c_and  = strstr(head, "&");
    }
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


/*根据Shop节点的信息查询Website记录*/
void get_web_name(void) {
    char  * str = (char *) calloc(STR_MAX, sizeof(char) );
    int     output;
    int     input;
    long    len;

    /*生成查询指令*/
    sprintf(str, "M%ld\n\0", shop.web_num);

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
    transfer_input(str, len, 'M', &web);

    /*释放str指令字符串所占用的堆空间*/
    free(str);
}


/*将待查看的Shop节点信息填入网页模板*/
void generate_add_done(void) {
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
    strcat(new, "Shop");
    mark0 += strlen(m0);
    strcat(new, mark0);

    /*在标记一处填入信息*/
    sprintf(word1,
        "<tr class=\"odd\">"
            "<td width=\"170\">Website</td>"
            "<td width=\"340\"><a href=\"../cgi-bin/view_web?web_num=%ld&"
            "submit=OK\">%s</a></td>"
        "</tr>"
        "<tr>"
            "<td width=\"170\">Shop ID</td>"
            "<td width=\"340\">%s</td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td width=\"170\">Shop Name</td>"
            "<td width=\"340\">%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"170\">Owner</td>"
            "<td width=\"340\">%s</td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td width=\"170\">Location</td>"
            "<td width=\"340\">%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"170\">Bank</td>"
            "<td width=\"340\">%s</td>"
        "</tr>"
            ,
        web.web_num, web.web_name, shop.shop_id, shop.shop_name,
        shop.owner, area_list[shop.shop_area], bank_list[shop.bank]);

    strcat(new, word1);
    mark1 += strlen(m1);
    strcat(new, mark1);

    /*在标记二处填入信息*/
    strcat(new, "../cgi-bin/which_shop?v");
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
    dispense_url('N', url, ins);

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

    /*分析并还原Matrix反馈信息，将回馈的Shop节点数据存入shop*/
    transfer_input(tmpstr, len, 'N', &shop);

    /*根据新Shop节点的信息查询Website记录*/
    get_web_name();

    /*生成目标网页*/
    generate_add_done();

    /*向客户端发送meta refresh代码，引导浏览器打开新生成的网页*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}


