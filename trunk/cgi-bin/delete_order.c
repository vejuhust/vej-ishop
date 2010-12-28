#include "ishop_cgis.h"

#define PART_MAX    20
#define STR_MAX     102400

char * item[] = {
    "web_num",
    "shop_id",
    "order_num",
    NULL
};

char    meta    [STR_MAX] = "<meta http-equiv=\"refresh\" content=\"0;"
                            "url=../cgi-bin/handle_order?";

char    tmpstr  [STR_MAX];


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
                strcat(meta, tmpstr);
                strcat(meta, "&");
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

void check_input(char * str, long len) {
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

    if ('I' != type) {
        error_handler("What the fuck?", NULL);
    }

    fclose(file);
    remove(TMPFILE);
}

int main(int argc, char** argv) {
    char  * url = (char *) calloc(STR_MAX, sizeof(char));
    char  * ins = (char *) calloc(STR_MAX, sizeof(char) );
    long    len;
    int     output;
    int     input;

    /*analyze input*/
    strcpy(url, getenv("QUERY_STRING"));

    dispense_url('I', url, item, 3, ins);

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
    check_input(tmpstr, len);

    strcat(meta, "submit=OK\" />");

    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}
