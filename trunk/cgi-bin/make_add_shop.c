#include "ishop_cgis.h"

#define STR_MAX     102400
#define MODEL       "model/add_shop"
#define PAGE        "baby/add_shop.htm"

const char * ins  = "J\n\n\0";
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/add_shop.htm\" />";

char            tmpstr  [STR_MAX];

void error_handler(char * msg) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    setenv("QUERY_STRING2", "../cgi-bin/make_add_shop", 1);

    Execve("cgi-bin/error", emptylist, environ);
}

int main(int argc, char** argv) {
    int     output;
    int     input;
    long    len;
    long    cnt;
    long    i;
    FILE  * file;
    char  * mark;
    char  * str = (char *) calloc(STR_MAX, sizeof(char));
    char  * new = (char *) calloc(STR_MAX, sizeof(char));
    char  * buf = (char *) calloc(STR_MAX, sizeof(char));
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
        error_handler("What's up?");
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
        error_handler("no website record, add one first.");
    }
    /**************/

    fread(&len, sizeof(long), 1, file);
    fread(&tmpstr, sizeof(char), len, file);
    fclose(file);

    remove(TMPFILE);

    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    mark  = strstr(str, m0);
    *mark = '\0';

    strcpy(new, str);

    r0 = tmpstr;
    for (i = 0; i < cnt; i++) {
        r1  = strstr(r0, "\n");
        *r1 = '\0';
        r1++;

        r2  = strstr(r1, "\n");
        *r2 = '\0';
        r2++;

        sprintf(buf, "<option value=\"%s\" >%s</option>\r\n\0", r0, r1);
        strcat(new, buf);

        r0 = r2;
    }

    mark += strlen(m0);
    strcat(new, mark);
    
    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
    
    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}
