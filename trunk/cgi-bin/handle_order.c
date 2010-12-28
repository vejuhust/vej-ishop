#include "ishop_cgis.h"

#define MAX_IN      1000
#define PART_MAX    20
#define STR_MAX     102400
#define MODEL       "model/handle_order"
#define PAGE        "baby/handle_order.htm"

const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/handle_order.htm\" />";

char            tmpstr  [STR_MAX];
struct shop_t   shop;
struct order_t  order   [ORDER_MAX];

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

void dispense_url(char type, char * str, char * ins) {
    char  * head = str;
    char  * c_and;
    long    len;

    strcat(str, "&");

    *ins = type;
    c_and = strstr(str, "&");
    while (NULL != c_and) {
        len = c_and - head;
        strncpy(tmpstr, head, len);
        tmpstr[len] = '\0';

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
            if (NULL != strcasestr(tmpstr, "shop_id")) {
                strcat(ins, strstr(tmpstr, "=") + 1);
                recovery_url(ins);
                strcat(ins,"\n");
                break;
            }
            else {
                error_handler("select a shop record please.",
                              "../cgi-bin/which_order?v");
            }
        }
        
        head = c_and + 1;
        c_and  = strstr(head, "&");
    }
}


void transfer_input(char * str, long len, char first, void * aim) {
    FILE * file;
    char   type;

    file = fopen(TMPFILE, "wb");
    fwrite(str, sizeof(char), len, file);
    fclose(file);

    file = fopen(TMPFILE, "rb");
    fread(&type, sizeof(char), 1, file);

    if ('Z' == type) {
        char * msg;

        fread(&len, sizeof(long), 1, file);
        msg = (char *) calloc(len + 5, sizeof(char));

        fread(msg, sizeof(char), len, file);

        fclose(file);
        remove(TMPFILE);

        error_handler(msg, NULL);
    }

    if (first == type) {
        fread(&len, sizeof(long), 1, file);
        fread(aim, len, 1, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    fclose(file);
    remove(TMPFILE);
}


void transfer_input2(char * str, long len, char first, void * aim, long *cnt) {
    FILE  * file;
    char    type;

    file = fopen(TMPFILE, "wb");
    fwrite(str, sizeof(char), len, file);
    fclose(file);

    file = fopen(TMPFILE, "rb");
    fread(&type, sizeof(char), 1, file);

    if ('Z' == type) {
        char * msg;

        fread(&len, sizeof(long), 1, file);
        msg = (char *) calloc(len + 5, sizeof(char));

        fread(msg, sizeof(char), len, file);

        fclose(file);
        remove(TMPFILE);

        error_handler(msg, NULL);
    }

    if (first == type) {
        fread(cnt, sizeof(long), 1, file);

        if (0 == *cnt) {
            error_handler("no order records here.","../cgi-bin/which_order?v");
        }

        fread(&len, sizeof(long), 1, file);
        fread(aim, len, *cnt, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    fclose(file);
    remove(TMPFILE);
}


void get_shop_name(void) {
    char  * str = (char *) calloc(STR_MAX, sizeof(char) );
    int     output;
    int     input;
    long    len;

    sprintf(str, "N%ld\n%s\n\0", order[0].web_num, order[0].shop_id);

    /*send it to matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, str, strlen(str));
    close(output);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, str, STR_MAX);
    close(input);
    str[len] = '\0';

    transfer_input(str, len, 'N', &shop);

    free(str);
}


void generate_add_done(long cnt) {
    FILE * file;
    char * mark0;
    char * mark1;
    char * word1 = (char *) calloc(STR_MAX, sizeof(char));
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));
    long   i;

    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    mark0 = strstr(str, m0);
    mark1 = strstr(str, m1);

    *mark0 = '\0';
    *mark1 = '\0';

    strcpy(new, str);
    strcat(new, shop.shop_name);
    mark0 += strlen(m0);
    strcat(new, mark0);


    for (i = 0; i < cnt; i++) {
        sprintf(word1,
        "<tr %s>"
        "<td>%.4ld-%.2ld-%.2ld</td>"
	"<td>%s</td>"
	"<td>%s</td>"
	"<td>%.2Lf</td>"
	"<td><a href=\"../cgi-bin/view_order?web_num=%ld&shop_id=%s&"
                        "order_num=%ld&submit=OK\">View</a></td>"
	"<td><a href=\"../cgi-bin/make_edit_order?web_num=%ld&shop_id=%s&"
                        "order_num=%ld&submit=OK\">Edit</a></td>"
	"<td><a href=\"../cgi-bin/delete_order?web_num=%ld&shop_id=%s&"
                        "order_num=%ld&submit=OK\">Delete</a></td>"
        "</tr>",
        (i & 0x1)?"":"class=\"odd\"",
        order[i].date.year, order[i].date.month, order[i].date.day,
        area_list[order[i].order_area], pay_list[order[i].pay],
        order[i].money, order[i].web_num, order[i].shop_id, order[i].order_num,
        order[i].web_num, order[i].shop_id, order[i].order_num,
        order[i].web_num, order[i].shop_id, order[i].order_num );

        strcat(new, word1);
    }
    mark1 += strlen(m1);
    strcat(new, mark1);

    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
}


int main(int argc, char** argv) {

    char         * url = (char *) calloc(STR_MAX, sizeof(char));
    char         * ins = (char *) calloc(STR_MAX, sizeof(char) );
    long           len;
    long           cnt;
    int            output;
    int            input;

    /*analyze input*/
    strcpy(url, getenv("QUERY_STRING"));

    dispense_url('L', url, ins);

    /*send it to matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    free(ins);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*get order_t*/
    transfer_input2(tmpstr, len, 'L', &order, &cnt);

    /*get_shop_name*/
    get_shop_name();

    /*generate add_result.htm*/
    generate_add_done(cnt);

    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}
