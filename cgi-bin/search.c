#include "ishop_cgis.h"

#define MAX_IN      1000
#define PART_MAX    20
#define STR_MAX     102400
#define MODEL       "model/search"
#define PAGE        "baby/search.htm"

const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/search.htm\" />";

char            tmpstr  [STR_MAX];
char            keyword [STR_MAX];
struct web_t    tmpweb;
struct web_t    web     [WEB_MAX];
struct shop_t   shop    [SHOP_MAX];
long            empty  = 0;
long            none   = 0;


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


void dispense_url(char type, char * str, char * ins, char * year) {
    char  * head = str;
    char  * c_and;
    long    len;

    strcat(str, "&");

    *ins  = type;
    c_and = strstr(str, "&");
    while (NULL != c_and) {
        len = c_and - head;
        strncpy(tmpstr, head, len);
        tmpstr[len] = '\0';

        if (NULL != strcasestr(tmpstr, "q")) {
            strcat(ins, strstr(tmpstr, "=") + 1);
            recovery_url(ins);

            if (1 == strlen(ins)) {
                error_handler("input a word please.",
                              "../index.htm");
            }

            strcpy(keyword, ins + 1);
            strcat(ins, "\n");
            break;
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
            empty = 1;
            return;
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


void get_web_name(long web_num) {
    char  * str = (char *) calloc(STR_MAX, sizeof(char) );
    int     output;
    int     input;
    long    len;

    sprintf(str, "M%ld\n\0", web_num);

    /*send it to matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, str, strlen(str));
    close(output);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, str, STR_MAX);
    close(input);
    str[len] = '\0';

    transfer_input(str, len, 'M', &tmpweb);

    free(str);
}


void generate_state(long cnt_web, long cnt_shop) {
    FILE * file;
    char * mark0;
    char * mark1;
    char * mark2;
    char * mark3;
    char * word1 = (char *) calloc(STR_MAX, sizeof(char));
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));
    long   i;

    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    mark0 = strstr(str, m0);
    mark1 = strstr(str, m1);
    mark2 = strstr(str, m2);
    mark3 = strstr(str, m3);

    *mark0 = '\0';
    *mark1 = '\0';
    *mark2 = '\0';
    *mark3 = '\0';

    strcpy(new, str);
    strcat(new, keyword);
    mark0 += strlen(m0);

    strcat(new, mark0);
    strcat(new, keyword);
    mark1 += strlen(m1);

    strcat(new, mark1);

    if (0 == (none & 1) ) {
        strcat(new, "<table width=\"95%\"><caption>Websites</caption>"
                    "<tr><th>Website Name</th><th>URL</th></tr>" );
        for (i = 0; i < cnt_web; i++) {
            sprintf(word1,
                    "<tr %s>"
                        "<td><a href=\"../cgi-bin/view_web?web_num=%ld"
                            "&submit=OK\">%s</a></td>"
                        "<td><a href=\"http://%s\">%s</a></td>"
                    "</tr>",
                    (i & 0x1)?(""):("class=\"odd\""), web[i].web_num,
                    web[i].web_name, web[i].url, web[i].url);
            strcat(new, word1);
        }
        strcat(new, "</table>");
    }
    else {
        strcat(new, "<p>No Websites Found!</p>");
    }
    
    mark2 += strlen(m2);

    strcat(new, mark2);

    if (0 == (none & 2) ) {
        strcat(new, "<table width=\"95%\"><caption>Shops</caption><tr>"
                    "<th>Website</th><th>Shop Name</th><th>Shop ID</th>"
                    "<th>Owner</th></tr>");
        for (i = 0; i < cnt_shop; i++) {
            get_web_name(shop[i].web_num);
            sprintf(word1,
                    "<tr %s>"
                        "<td><a href=\"../cgi-bin/view_web?web_num=%ld"
                            "&submit=OK\">%s</a></td>"
                        "<td><a href=\"../cgi-bin/view_shop?web_num=%ld"
                            "&shop_id=%s&submit=OK\">%s</a></td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                    "</tr>",
                    (i & 0x1)?(""):("class=\"odd\""), tmpweb.web_num,
                    tmpweb.web_name, shop[i].web_num, shop[i].shop_id,
                    shop[i].shop_name, shop[i].shop_id, shop[i].owner);
            strcat(new, word1);
        }
        strcat(new, "</table>");
    }
    else {
        strcat(new, "<p>No Shops Found!</p>");
    }

    mark3 += strlen(m3);

    strcat(new, mark3);

    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
}


int main(int argc, char** argv) {
    char         * year = (char *) calloc(256, sizeof(char));
    char         * url = (char *) calloc(STR_MAX, sizeof(char));
    char         * ins = (char *) calloc(STR_MAX, sizeof(char) );
    long           len;
    long           cnt_web;
    long           cnt_shop;
    int            output;
    int            input;

    /*analyze input*/
    strcpy(url, getenv("QUERY_STRING"));

    dispense_url('P', url, ins, year);

    /*send it to matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    transfer_input2(tmpstr, len, 'P', web, &cnt_web);
    
    if (1 == empty) {
        none  |= 1;
        empty = 0;
    }

    *ins = 'Q';

    /*send it to matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    transfer_input2(tmpstr, len, 'Q', shop, &cnt_shop);

    if (1 == empty) {
        none  |= 2;
    }

    generate_state(cnt_web, cnt_shop);
    
    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}




