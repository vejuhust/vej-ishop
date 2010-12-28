#include "ishop_cgis.h"

#define MAX_IN      1000
#define PART_MAX    20
#define STR_MAX     102400
#define MODEL       "model/edit_done"
#define PAGE        "baby/edit_done.htm"

char * item[] = {
    "old1",
    "old2",
    "web_num",
    "shop_id",
    "shop_name",
    "owner",
    "shop_area",
    "bank",
    NULL
};

const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/edit_done.htm\" />";

char            tmpstr      [STR_MAX];
struct web_t    old_web;
struct web_t    web;
struct shop_t   old_shop;
struct shop_t   shop;

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


void error_handler(char * msg) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    setenv("QUERY_STRING2", "../cgi-bin/which_shop?edit", 1);

    Execve("cgi-bin/error", emptylist, environ);
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

        error_handler(msg);
    }

    if (first == type) {
        fread(&len, sizeof(long), 1, file);
        fread(aim, len, 1, file);
    }
    else {
        error_handler("What the fuck?");
    }

    fclose(file);
    remove(TMPFILE);
}


void get_web_name(struct shop_t * shop, struct web_t * web) {
    char  * str = (char *) calloc(STR_MAX, sizeof(char) );
    int     output;
    int     input;
    long    len;

    sprintf(str, "M%ld\n\0", shop->web_num);

    /*send it to matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, str, strlen(str));
    close(output);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, str, STR_MAX);
    close(input);
    str[len] = '\0';

    transfer_input(str, len, 'M', web);

    
    free(str);
}


void generate_edit_done(void) {
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
    strcat(new, "shop");
    mark0 += strlen(m0);
    strcat(new, mark0);

    sprintf(word1,
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Website</td>"
            "<td width=\"5%%\">old</td>"
            "<td><a href=\"../cgi-bin/view_web?web_num=%ld&"
            "submit=OK\">%s</a></td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td><a href=\"../cgi-bin/view_web?web_num=%ld&"
            "submit=OK\">%s</a></td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Shop ID</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Shop Name</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr class=\"odd\">"
            "<td rowspan=\"2\">Owner</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%s</td>"
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
            "<td rowspan=\"2\">Bank</td>"
            "<td width=\"5%%\">old</td>"
            "<td>%s</td>"
        "</tr>"
        "<tr>"
            "<td width=\"5%%\">new</td>"
            "<td>%s</td>"
        "</tr>",
         old_web.web_num, old_web.web_name, web.web_num, web.web_name,
         old_shop.shop_id,   shop.shop_id,
         old_shop.shop_name, shop.shop_name,
         old_shop.owner,     shop.owner,
         area_list[old_shop.shop_area], area_list[shop.shop_area],
         bank_list[old_shop.bank], bank_list[shop.bank]);

    strcat(new, word1);
    mark1 += strlen(m1);
    strcat(new, mark1);

    strcat(new, "../cgi-bin/which_shop?edit");
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

    dispense_url('N', url, item, 8, ins);

    /*send it to matrix*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*get shop_t*/
    transfer_input(tmpstr, len, 'N', &old_shop);

    /*get_web_name*/
    get_web_name(&old_shop, &old_web);


    ///////////
    *ins = 'E';

    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*get echo from matrix*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    /*get shop_t*/
    transfer_input(tmpstr, len, 'E', &shop);

    /*get_web_name*/
    get_web_name(&shop, &web);

    /*generate add_result.htm*/
    generate_edit_done();

    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}
