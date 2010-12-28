#include "ishop_cgis.h"

#define STR_MAX     102400                      /*Matrix指令参数的最大个数*/
#define MODEL       "model/edit_shop"           /*网页模板文件地址*/
#define PAGE        "baby/edit_shop.htm"        /*生成的网页的地址*/

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/edit_shop.htm\" />";

char            tmpstr  [STR_MAX];              /*临时字符串*/
char            clue    [7] = "[[0]]\0";        /*model文件中数据填入的标记*/
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
    if ('N' == type) {
        fread(&len, sizeof(long), 1, file);
        fread(&shop, len, 1, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);
}


/*获取所有Website记录的信息列表*/
void get_web_list(char * buf) {
    int     output;
    int     input;
    long    len;
    long    cnt;
    long    i;
    char    ins[] = "J\n\n";
    FILE  * file;
    char  * r0;
    char  * r1;
    char  * r2;

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
    if ('J' != tmpstr[0]) {
        error_handler("What's up?", NULL);
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
        error_handler("no website record, add one first.", NULL);
    }
    
    /*从临时文件中重新按格式读取*/
    fread(&len, sizeof(long), 1, file);
    fread(&tmpstr, sizeof(char), len, file);

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);

    /*根据信息生成网页代码串*/
    r0 = tmpstr;
    for (i = 0; i < cnt; i++) {
        r1  = strstr(r0, "\n");
        *r1 = '\0';
        r1++;

        r2  = strstr(r1, "\n");
        *r2 = '\0';
        r2++;

        if (shop.web_num == atoi(r0)) {
            sprintf(buf, "%s<option selected value=\"%s\" >%s</option>\r\n\0",
                            buf, r0, r1);
        }
        else {
            sprintf(buf, "%s<option value=\"%s\" >%s</option>\r\n\0",
                            buf, r0, r1);
        }

        r0 = r2;
    }
}


/*将待修改的Shop节点信息填入网页模板*/
void generate_edit_web(void) {
    FILE * file;
    char * mark[10];
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));
    char * buf = (char *) calloc(STR_MAX, sizeof(char));
    long i;

    /*载入网页模板*/
    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    /*寻找并处理网页模板中的标记*/
    for (i = 0; i < 8; i++) {
        clue[2] = i + '0';
        mark[i] = strstr(str, clue);
    }

    for (i = 0; i < 8; i++) {
        *(mark[i]) = '\0';
    }

    /*在标记零处填入信息*/
    strcpy(new, str);
    get_web_list(buf);
    strcat(new, buf);
    mark[0] += 5;

    /*在标记一处填入信息*/
    strcat(new, mark[0]);
    strcat(new, shop.shop_id);
    mark[1] += 5;

    /*在标记二处填入信息*/
    strcat(new, mark[1]);
    strcat(new, shop.shop_name);
    mark[2] += 5;

    /*在标记三处填入信息*/
    strcat(new, mark[2]);
    strcat(new, shop.owner);
    mark[3] += 5;

    /*在标记四处填入信息*/
    strcat(new, mark[3]);
    for (i = 1; i <= 34; i++) {
        if (i == shop.shop_area) {
            sprintf(buf,"<option selected value=\"%ld\" >%s</option>",
                        i, area_list[i]);
        }
        else {
            sprintf(buf,"<option value=\"%ld\" >%s</option>", i, area_list[i]);
        }
        strcat(new, buf);
    }
    mark[4] += 5;

    /*在标记五处填入信息*/
    strcat(new, mark[4]);
    for (i = 1; i <= 33; i++) {
        if (i == shop.bank) {
            sprintf(buf,"<option selected value=\"%ld\" >%s</option>",
                        i, bank_list[i]);
        }
        else {
            sprintf(buf,"<option value=\"%ld\" >%s</option>", i, bank_list[i]);
        }
        strcat(new, buf);
    }
    mark[5] += 5;

    /*在标记六处填入信息*/
    strcat(new, mark[5]);
    sprintf(buf, "%ld", shop.web_num);
    strcat(new, buf);
    mark[6] += 5;

    /*在标记七处填入信息*/
    strcat(new, mark[6]);
    strcat(new, shop.shop_id);
    mark[7] += 5;
    strcat(new, mark[7]);

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

    /*分析并还原Matrix反馈信息，获取待修改的Shop节点数据*/
    transfer_input(tmpstr, len);

    /*生成目标网页*/
    generate_edit_web();

    /*向客户端发送meta refresh代码，引导浏览器打开新生成的网页*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    /*程序正常退出*/
    exit(0);
}


