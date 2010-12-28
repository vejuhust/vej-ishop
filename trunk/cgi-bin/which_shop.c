#include "ishop_cgis.h"

#define STR_MAX     102400
#define MODEL       "model/which_shop"
#define PAGE        "baby/which_shop.htm"


char * edit_list [] = {
    "Edit",
    "Edit",
    "make_edit_shop",
    "Edit",
    "Modify",
    NULL
};

char * delete_list [] = {
    "Delete",
    "Delete",
    "delete_shop",
    "Delete",
    "Remove",
    NULL
};

char * view_list [] = {
    "View",
    "View",
    "view_shop",
    "View",
    "Check",
    NULL
};

const char * ins  = "K\n\n\0";
const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/which_shop.htm\" />";

char        tmpstr  [STR_MAX];


void error_handler(char * msg) {
    char * emptylist[] = { NULL };

    setenv("QUERY_STRING", msg, 1);
    setenv("QUERY_STRING2", "../index.htm", 1);

    Execve("cgi-bin/error", emptylist, environ);
}


int main(int argc, char** argv) {
    char    clue  [7] = "[[0]]\0";
    char ** word;
    int     output;
    int     input;
    long    len;
    long    cnt;
    long    num[WEB_MAX];
    long    i;
    long    j;
    FILE  * file;
    char  * mark;
    char  * str = (char *) calloc(STR_MAX, sizeof(char));
    char  * new = (char *) calloc(STR_MAX, sizeof(char));
    char  * buf = (char *) calloc(STR_MAX, sizeof(char));
    char  * web = (char *) calloc(STR_MAX, sizeof(char));
    char  * r0;
    char  * r1;
    char  * r2;

    switch ( *(getenv("QUERY_STRING")) ) {
        case 'e' :
            word = &edit_list;
            break;
        case 'd' :
            word = &delete_list;
            break;
        case 'v' :
            word = &view_list;
            break;
        default :
            error_handler("Oops...what did you input!?");
            break;
    }

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
        error_handler("What's happened?");
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
        error_handler("no website record, add one first!");
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
        error_handler("no shop record, add one first!");
    }


    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    *new = '\0';

    for (i = 0; i < 5; i++) {
        clue[2] = i + '0';
        mark  = strstr(str, clue);
        *mark = '\0';

        strcat(new, str);
        strcat(new, word[i]);
        str = mark + 5;
    }

    clue[2] = '5';
    mark  = strstr(str, clue);
    *mark = '\0';

    strcat(new, str);

    str = mark + 5;

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

                sprintf(buf, "<option value=\"%s#%s\" >%s</option>\r\n\0",
                             web, r0, r1);
                strcat(new, buf);

                r0 = r2;
            }
            strcat(new, "</optgroup>\r\n\0");
        }
        else {
            r0 = r2;
        }
    }

    strcat(new, str);

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

