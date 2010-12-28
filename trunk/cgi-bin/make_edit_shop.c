#include "ishop_cgis.h"

#define STR_MAX     102400
#define MODEL       "model/edit_shop"
#define PAGE        "baby/edit_shop.htm"


const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/edit_shop.htm\" />";

char            tmpstr  [STR_MAX];
char            clue    [7] = "[[0]]\0";
struct shop_t   shop;

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
                              "../cgi-bin/which_shop?v");
            }
        }

        head = c_and + 1;
        c_and  = strstr(head, "&");
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

    if ('N' == type) {
        fread(&len, sizeof(long), 1, file);
        fread(&shop, len, 1, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    fclose(file);
    remove(TMPFILE);
}


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

    /*send the ins*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*get the list*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    if ('J' != tmpstr[0]) {
        error_handler("What's up?", NULL);
    }

    file = fopen(TMPFILE, "wb");
    fwrite(&tmpstr[1], sizeof(char), len, file);
    fclose(file);

    file = fopen(TMPFILE, "rb");
    fread(&cnt, sizeof(long), 1, file);

    /**************/
    if (0 == cnt) {
        fclose(file);
        remove(TMPFILE);
        error_handler("no website record, add one first.", NULL);
    }
    /**************/

    fread(&len, sizeof(long), 1, file);
    fread(&tmpstr, sizeof(char), len, file);
    fclose(file);

    remove(TMPFILE);

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

void generate_edit_web(void) {
    FILE * file;
    char * mark[10];
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));
    char * buf = (char *) calloc(STR_MAX, sizeof(char));
    long i;

    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

            
    for (i = 0; i < 8; i++) {
        clue[2] = i + '0';
        mark[i] = strstr(str, clue);
    }

    for (i = 0; i < 8; i++) {
        *(mark[i]) = '\0';
    }

    strcpy(new, str);
    get_web_list(buf);
    strcat(new, buf);
    mark[0] += 5;

    strcat(new, mark[0]);
    strcat(new, shop.shop_id);
    mark[1] += 5;

    strcat(new, mark[1]);
    strcat(new, shop.shop_name);
    mark[2] += 5;

    strcat(new, mark[2]);
    strcat(new, shop.owner);
    mark[3] += 5;

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
    
    strcat(new, mark[5]);
    sprintf(buf, "%ld", shop.web_num);
    strcat(new, buf);
    mark[6] += 5;

    strcat(new, mark[6]);
    strcat(new, shop.shop_id);
    mark[7] += 5;

    strcat(new, mark[7]);

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

    dispense_url('N', url, ins);

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
    generate_edit_web();

    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}
