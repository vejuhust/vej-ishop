#include "ishop_cgis.h"

#define STR_MAX     102400
#define PART_MAX    20
#define MODEL       "model/edit_order"
#define PAGE        "baby/edit_order.htm"

char * item[] = {
    "web_num",
    "shop_id",
    "order_num",
    NULL
};

const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/edit_order.htm\" />";

char            tmpstr  [STR_MAX];
char            clue    [7]   = "[[0]]\0";
struct shop_t   shop;
struct order_t  order;


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


void dispense_url(char type, char * str, char * item[], long cnt, char * ins) {
    char  * head = str;
    char  * c_and;
    long    i;
    long    len;
    char  * part[PART_MAX];

    memset(part, 0, PART_MAX * sizeof(char *));
    strcat(str, "&");

    c_and = strstr(str, "&");
    while (NULL != c_and) {
        len = c_and - head;
        strncpy(tmpstr, head, len);
        tmpstr[len] = '\0';

        for (i = 0; i < cnt; i++) {
            if (NULL != strcasestr(tmpstr, item[i])) {
                part[i] = (char *) calloc(len + 2, sizeof(char));
                strcpy(part[i], strstr(tmpstr, "=") + 1);
                recovery_url(part[i]);
                break;
            }
        }
        head = c_and + 1;
        c_and  = strstr(head, "&");
    }

    *ins = type;

    for (i = 0; i < cnt; i++) {
        if (NULL != part[i]) {
            strcat(ins, part[i]);
            free(part[i]);
        }
        strcat(ins, "\n");
    }
}


void transfer_input(char * str, long len) {
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

    if ('O' == type) {
        fread(&len, sizeof(long), 1, file);
        fread(&order, len, 1, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    fclose(file);
    remove(TMPFILE);
}


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

    /*send the ins*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*get the list*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    if ('K' != tmpstr[0]) {
        error_handler("What's happened?", NULL);
    }

    file = fopen(TMPFILE, "wb");
    fwrite(&tmpstr[1], sizeof(char), len, file);
    fclose(file);

    file = fopen(TMPFILE, "rb");
    fread(&cnt, sizeof(long), 1,   file);
    fread(&num, sizeof(long), cnt, file);

    /**************/
    if (0 == cnt) {
        fclose(file);
        remove(TMPFILE);
        error_handler("no website record, add one first!", NULL);
    }
    /**************/

    fread(&len, sizeof(long), 1,   file);
    fread(&tmpstr, sizeof(char), len, file);
    fclose(file);

    remove(TMPFILE);

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

void generate_edit_order(void) {
    FILE * file;
    char * mark[15];
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));
    char * buf = (char *) calloc(STR_MAX, sizeof(char));
    long i;

    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

            
    for (i = 0; i < 11; i++) {
        clue[2] = i + '0';
        mark[i] = strstr(str, clue);
    }

    for (i = 0; i < 11; i++) {
        *(mark[i]) = '\0';
    }

    strcpy(new, str);
    get_shop_list(buf);
    strcat(new, buf);
    mark[0] += 5;

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

    strcat(new, mark[1]);
    sprintf(buf, "%.2Lf", order.money);
    strcat(new, buf);
    mark[2] += 5;

    strcat(new, mark[2]);
    sprintf(buf, "%.2ld", order.date.month);
    strcat(new, buf);
    mark[3] += 5;

    strcat(new, mark[3]);
    sprintf(buf, "%.2ld", order.date.day);
    strcat(new, buf);
    mark[4] += 5;

    strcat(new, mark[4]);
    sprintf(buf, "%.4ld", order.date.year);
    strcat(new, buf);
    mark[5] += 5;

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

    strcat(new, mark[6]);
    sprintf(buf, "%ld", order.web_num);
    strcat(new, buf);
    mark[7] += 5;

    strcat(new, mark[7]);
    strcat(new, order.shop_id);
    mark[8] += 5;

    strcat(new, mark[8]);
    sprintf(buf, "%ld", order.order_num);
    strcat(new, buf);
    mark[9] += 5;

    strcat(new, mark[9]);
    sprintf(buf, "../cgi-bin/handle_order?web_num=%ld&shop_id=%s&submit=OK",
                  order.web_num, order.shop_id);
    strcat(new, buf);
    mark[10] += 5;

    strcat(new, mark[10]);

    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
}


int main(int argc, char** argv) {
    int     output;
    int     input;
    long    len;
    char  * ins = (char *) calloc(STR_MAX, sizeof(char) );
    char  * url = (char *) calloc(STR_MAX, sizeof(char));

    /*analyze input*/
    strcpy(url, getenv("QUERY_STRING"));

    dispense_url('O', url, item, 3, ins);

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

    /*get web_t*/
    transfer_input(tmpstr, len);

    /*generate ...*/
    generate_edit_order();

    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}
