#include "ishop_cgis.h"

#define STR_MAX     102400                      /*Matrix指令参数的最大个数*/
#define MODEL       "model/which_shop"          /*网页模板文件地址*/
#define PAGE        "baby/which_shop.htm"       /*生成的网页的地址*/

/*Edit模块页面参数*/
char * edit_list [] = {
    "Edit",
    "Edit",
    "make_edit_shop",
    "Edit",
    "Modify",
    NULL
};

/*Delete模块页面参数*/
char * delete_list [] = {
    "Delete",
    "Delete",
    "delete_shop",
    "Delete",
    "Remove",
    NULL
};

/*Check模块页面参数*/
char * view_list [] = {
    "View",
    "View",
    "view_shop",
    "View",
    "Check",
    NULL
};

const char * ins  = "K\n\n\0";                  /*Matrix指令K*/

/*最终向客户端发送的meta refresh代码*/
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/which_shop.htm\" />";

char        tmpstr  [STR_MAX];                  /*临时字符串*/


/*错误处理函数*/
void error_handler(char * msg) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    setenv("QUERY_STRING2", "../index.htm", 1);

    Execve("cgi-bin/error", emptylist, environ);
}


/*主函数*/
int main(int argc, char** argv) {
    char    clue  [7] = "[[0]]\0";
    char ** word;
    int     output;
    int     input;
    long    len;
    long    cnt;
    long    num[WEB_MAX];
    long    i;
    long    j;
    FILE  * file;
    char  * mark;
    char  * str = (char *) calloc(STR_MAX, sizeof(char));
    char  * new = (char *) calloc(STR_MAX, sizeof(char));
    char  * buf = (char *) calloc(STR_MAX, sizeof(char));
    char  * web = (char *) calloc(STR_MAX, sizeof(char));
    char  * r0;
    char  * r1;
    char  * r2;

    /*根据环境变量决定生成的版本*/
    switch ( *(getenv("QUERY_STRING")) ) {
        case 'e' :
            word = &edit_list;
            break;
        case 'd' :
            word = &delete_list;
            break;
        case 'v' :
            word = &view_list;
            break;
        default :
            error_handler("Oops...what did you input!?");
            break;
    }

    /*通过FIFO输入管道将指令提交给Matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*在FIFO输出管道上读取Matrix的反馈信息*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*若为反馈信息发生错误*/
    if ('K' != tmpstr[0]) {
        error_handler("What's happened?");
    }

    /*将反馈信息存入临时文件*/
    file = fopen(TMPFILE, "wb");
    fwrite(&tmpstr[1], sizeof(char), len, file);
    fclose(file);

    /*从临时文件中重新按格式读取*/
    file = fopen(TMPFILE, "rb");
    fread(&cnt, sizeof(long), 1,   file);
    fread(&num, sizeof(long), cnt, file);

    /*若反馈信息为空*/
    if (0 == cnt) {
        fclose(file);
        remove(TMPFILE);
        error_handler("no website record, add one first!");
    }

    /*从临时文件中重新按格式读取*/
    fread(&len, sizeof(long), 1,   file);
    fread(&tmpstr, sizeof(char), len, file);

    /*关闭并删除临时文件*/
    fclose(file);
    remove(TMPFILE);

    /*判断反馈信息是否为空*/
    j = 0;
    for (i = 0; i < cnt; i++) {
        if (0 != num[i]) {
            j = 1;
            break;
        }
    }

    if (0 == j) {
        error_handler("no shop record, add one first!");
    }

    /*载入网页模板*/
    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    /*寻找并处理网页模板中的标记*/
    *new = '\0';
    for (i = 0; i < 5; i++) {
        clue[2] = i + '0';
        mark  = strstr(str, clue);
        *mark = '\0';

        strcat(new, str);
        strcat(new, word[i]);
        str = mark + 5;
    }

    clue[2] = '5';
    mark  = strstr(str, clue);
    *mark = '\0';
    strcat(new, str);
    str = mark + 5;

    /*在相应标记处填入信息*/
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

                sprintf(buf, "<option value=\"%s#%s\" >%s</option>\r\n\0",
                             web, r0, r1);
                strcat(new, buf);

                r0 = r2;
            }
            strcat(new, "</optgroup>\r\n\0");
        }
        else {
            r0 = r2;
        }
    }

    strcat(new, str);

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


