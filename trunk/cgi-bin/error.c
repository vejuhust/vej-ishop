#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_MAX     102400
#define MODEL       "model/error"
#define PAGE        "baby/error.htm"

const char * s = "<meta http-equiv=\"refresh\" content=\"0;"
                 "url=../baby/error.htm\" />";
const char * m0 = "[[0]]";
const char * m1 = "[[1]]";
const char * de = "../index.htm";

int main() {
    FILE * file;
    char * mark0;
    char * mark1;
    char * word0;
    char * word1;
    char * str = (char *) calloc(STR_MAX, sizeof(char));
    char * new = (char *) calloc(STR_MAX, sizeof(char));

    word0 = getenv("QUERY_STRING");
    word1 = getenv("QUERY_STRING2");

    if (NULL == word0) {
        word0 = (char *) calloc(100, sizeof(char));
        strcpy(word0, "we don't know what happened either :(");
    }

    if (NULL == word1) {
        word1 = de;
    }

    file = fopen(MODEL, "rb");
    fread(str, sizeof(char), STR_MAX, file);
    fclose(file);

    mark0 = strstr(str, m0);
    mark1 = strstr(str, m1);

    *mark0 = '\0';
    *mark1 = '\0';

    strcpy(new, str);
    strcat(new, word0);
    mark0 += strlen(m0);
    strcat(new, mark0);

    strcat(new, word1);
    mark1 += strlen(m1);
    strcat(new, mark1);

    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);

    printf("Content-length: %d\r\n", strlen(s));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", s);
    fflush(stdout);

    exit(0);
}

