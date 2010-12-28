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

    *ins  = type;
    c_and = strstr(str, "&");
    while (NULL != c_and) {
        len = c_and - head;
        strncpy(tmpstr, head, len);
        tmpstr[len] = '\0';

        if (NULL != strcasestr(tmpstr, "web_num")) {
            strcat(ins, strstr(tmpstr, "=") + 1);
            recovery_url(ins);

            if (1 == strlen(ins)) {
                error_handler("select a web record please.",
                              "../cgi-bin/which_web?view");
            }

            strcat(ins, "\n");
            break;
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

    if ('M' == type) {
        fread(&len, sizeof(long), 1, file);
        fread(&web, len, 1, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    fclose(file);
    remove(TMPFILE);
}


void generate_view_done(void) {
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
    strcat(new, "Website");
    mark0 += strlen(m0);
    strcat(new, mark0);

    sprintf(word1,
        "<tr class=\"odd\"><td width=\"170\">Website Name</td>"
        "<td width=\"340\">%s</td></tr><tr>"
	"<td width=\"170\">URL</td><td width=\"340\">"
        "<a href=\"http://%s\">%s</a></td></tr>"
        "<tr class=\"odd\"><td width=\"170\">Website No.</td>"
        "<td width=\"340\">%ld</td></tr>"
        "<tr><td width=\"170\">Shops</td>"
        "<td width=\"340\">%ld</td></tr>"

            ,
        web.web_name, web.url, web.url, web.web_num, web.cnt);

    strcat(new, word1);
    mark1 += strlen(m1);
    strcat(new, mark1);

    strcat(new, "../cgi-bin/which_web?view");
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

    dispense_url('M', url, ins);

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

    /*generate add_result.htm*/
    generate_view_done();

    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}
