#include "ishop_cgis.h"


#define STR_MAX     102400
#define MODEL       "model/which_area"
#define PAGE        "baby/which_area.htm"

const char * ins  = "V\n\n\0";
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/which_area.htm\" />";

char        tmpstr   [STR_MAX];


void error_handler(char * msg) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    setenv("QUERY_STRING2", "../index.htm", 1);

    Execve("cgi-bin/error", emptylist, environ);
}


int main(int argc, char** argv) {
    int     output;
    int     input;
    long    len;
    long    min;
    long    max;
    long    i;
    FILE  * file;
    char  * mark;
    char  * str = (char *) calloc(STR_MAX, sizeof(char));
    char  * new = (char *) calloc(STR_MAX, sizeof(char));
    char  * buf = (char *) calloc(STR_MAX, sizeof(char));


    /*send the ins*/
    output = open(FIFO_IN, O_WRONLY | O_TRUNC);
    write(output, ins, strlen(ins));
    close(output);

    /*get the list*/
    input = open(FIFO_OUT, O_RDONLY);
    len   = read(input, tmpstr, STR_MAX);
    close(input);
    tmpstr[len] = '\0';

    if ('Z' == tmpstr[0]) {
        char * msg;

        file = fopen(TMPFILE, "wb");
        fwrite(&tmpstr[1], sizeof(char), len, file);
        fclose(file);

        file = fopen(TMPFILE, "rb");
        
        fread(&len, sizeof(long), 1, file);
        msg = (char *) calloc(len + 5, sizeof(char));

        fread(msg, sizeof(char), len, file);

        fclose(file);
        remove(TMPFILE);

        error_handler(msg);
    }

    if ('V' != tmpstr[0]) {
        error_handler("What's up?");
    }

    file = fopen(TMPFILE, "wb");
    fwrite(&tmpstr[1], sizeof(char), len, file);
    fclose(file);

    file = fopen(TMPFILE, "rb");
    fread(&min, sizeof(long), 1, file);
    fread(&max, sizeof(long), 1 ,file);

    fclose(file);

    remove(TMPFILE);

    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    *new = '\0';

    mark  = strstr(str, m0);
    *mark = '\0';

    strcat(new, str);

    for (i = min; i <= max; i++) {
        sprintf(buf, "<option value=\"%ld\" >%ld</option>\r\n\0", i, i);
        strcat(new, buf);
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

