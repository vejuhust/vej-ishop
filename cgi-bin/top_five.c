#include "ishop_cgis.h"

#define MAX_IN     1000
#define PART_MAX   20
#define STR_MAX     102400
#define MODEL       "model/top_five"
#define PAGE        "baby/top_five.htm"

const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/top_five.htm\" />";

char            tmpstr  [STR_MAX];
char            year    [100];
long            month;
struct top5_t   state   [10];


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

        if (NULL != strcasestr(tmpstr, "year")) {
            strcat(ins, strstr(tmpstr, "=") + 1);
            recovery_url(ins);

            head = strstr(ins, "#");
            if (NULL != head) {
                *(head) = '\n';
                strcat(ins, "\n");
                break;
            }

            if ('\0' == *(ins + 1) ) {
                error_handler("select the year please.",
                              "../cgi-bin/which_month");
            }
            
            strcpy(year, ins + 1);
            strcat(ins, "\n");
        }
        else {
            if (NULL != strcasestr(tmpstr, "month")) {
                long  end = strlen(ins);
                strcat(ins, strstr(tmpstr, "=") + 1);
                recovery_url(ins);

                if ('\0' == *(ins + end) ) {
                    error_handler("select the month please.",
                                  "../cgi-bin/which_month");
                }

                month = atoi(ins + end);
                strcat(ins,"\n");
                break;
            }
            else {
                error_handler("select a date please.",
                              "../cgi-bin/which_month");
            }
        }

        head = c_and + 1;
        c_and  = strstr(head, "&");
    }
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
            error_handler("no website records.", NULL);
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


void generate_state(long cnt) {
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
    sprintf(word1, "%s, %s", month_list_abbr[month], year);
    strcat(new, word1);
    mark0 += strlen(m0);

    strcat(new, mark0);

    if (0 == state[0].cnt) {
        sprintf(word1, "<p>No Orders in %s, %s.</p>",
                        month_list_abbr[month], year );
        strcat(new, word1);
    }
    else {
        strcat(new, "<table width=\"95%\"><caption></caption><tr><th>No.</th>"
                    "<th>Websites</th><th>Times</th></tr>" );
        for (i = 0; i < cnt; i++) {
            if (0 == state[i].cnt) {
                break;
            }

            sprintf(word1,
                    "<tr %s>"
                        "<td>%ld</td>"
                        "<td><a href=\"../cgi-bin/view_web?web_num=%ld&submit=OK\">%s</a></td>"
                        "<td>%ld</td>"
                    "</tr>",
                    (i & 0x1)?(""):("class=\"odd\""), i + 1, state[i].web_num,
                    state[i].web_name, state[i].cnt );
            strcat(new, word1);
        }
        strcat(new, "</table>");
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

    dispense_url('S', url, ins);

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

    transfer_input2(tmpstr, len, 'S', state, &cnt);

    generate_state(cnt);
    
    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}



