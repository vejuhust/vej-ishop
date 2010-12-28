#include "ishop_cgis.h"

#define MAX_IN      1000
#define PART_MAX    20
#define STR_MAX     102400
#define MODEL       "model/view_done"
#define PAGE        "baby/view_done.htm"

const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/view_done.htm\" />";

char            tmpstr  [STR_MAX];
struct web_t    web;
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


void get_web_name(void) {
    char  * str = (char *) calloc(STR_MAX, sizeof(char) );
    int     output;
    int     input;
    long    len;

    sprintf(str, "M%ld\n\0", shop.web_num);

    /*send it to matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, str, strlen(str));
    close(output);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, str, STR_MAX);
    close(input);
    str[len] = '\0';

    transfer_input(str, len, 'M', &web);

    
    free(str);
}


void generate_add_done(void) {
    FILE * file;
    char * mark0;
    char * mark1;
    char * mark2;
    char * word1 = (char *) calloc(STR_MAX, sizeof(char));
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));

    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    mark0 = strstr(str, m0);
    mark1 = strstr(str, m1);
    mark2 = strstr(str, m2);

    *mark0 = '\0';
    *mark1 = '\0';
    *mark2 = '\0';

    strcpy(new, str);
    strcat(new, "Shop");
    mark0 += strlen(m0);
    strcat(new, mark0);


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

    strcat(new, "../cgi-bin/which_shop?v");
    mark2 += strlen(m2);
    strcat(new, mark2);

    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
}


int main(int argc, char** argv) {
    char         * url = (char *) calloc(STR_MAX, sizeof(char));
    char         * ins = (char *) calloc(STR_MAX, sizeof(char) );
    long           len;
    int            output;
    int            input;

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

    /*get shop_t*/
    transfer_input(tmpstr, len, 'N', &shop);

    /*get_web_name*/
    get_web_name();

    /*generate add_result.htm*/
    generate_add_done();

    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}
