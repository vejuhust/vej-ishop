#include "ishop_cgis.h"

#define MAX_IN      1000
#define PART_MAX    20
#define STR_MAX     102400
#define MODEL       "model/trade_balance"
#define PAGE        "baby/trade_balance.htm"

char * item[] = {
    "year",
    "month",
    "area_a",
    "area_b",
    NULL
};

const char * meta = "<meta http-equiv=\"refresh\" content=\"0;"
                    "url=../baby/trade_balance.htm\" />";

char    tmpstr  [STR_MAX];
char    year    [100];
long    month;
long    area_a;
long    area_b;


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
                part[i] = (char *) calloc(len + 2, sizeof(char));
                strcpy(part[i], strstr(tmpstr, "=") + 1);
                recovery_url(part[i]);

                switch (i) {
                    case 0 :
                        strcpy(year, part[0]);
                        break;
                    case 1 :
                        month = atoi(part[1]);
                        break;
                    case 2 :
                        area_a = atoi(part[2]);
                        break;
                    case 3 :
                        area_b = atoi(part[3]);
                        break;
                }
                
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


void transfer_input2(char * str, long len, char first, void * aim) {
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
        fread(&len, sizeof(long), 1, file);
        fread(aim, len, 1, file);
    }
    else {
        error_handler("What the fuck?", NULL);
    }

    fclose(file);
    remove(TMPFILE);
}


void generate_result(long double sum) {
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
    sprintf(word1, "%s and %s", area_list[area_a], area_list[area_b]);
    strcat(new, word1);
    mark0 += strlen(m0);

    strcat(new, mark0);
    if (fabs(sum) < 1e-6) {
        sprintf(word1, "In %s %s, %s and %s made the trade balance.",
          month_list_abbr[month], year, area_list[area_a], area_list[area_b]);
    }
    else {
        if (sum > 0) {
            sprintf(word1, "In %s %s, %s had trade surplus to %s.",
            month_list_abbr[month], year, area_list[area_a], area_list[area_b]);
        }
        else {
            sprintf(word1, "In %s %s, %s had trade deficit to %s.",
            month_list_abbr[month], year, area_list[area_a], area_list[area_b]);
        }
    }

    strcat(new, word1);
    
    mark1 += strlen(m1);
    strcat(new, mark1);

    file = fopen(PAGE, "wb");
    fwrite(new, sizeof(char), strlen(new), file);
    fclose(file);
}

int empty_exist(char * str) {
    char * p = str + 1;

    while ('\0' != *p) {
        if ( ('\n' == *(p-1)) && ('\n' == *(p)) ) {
            return (1);
        }
        p++;
    }
    
    return (0);
}


int main(int argc, char** argv) {
    char         * url = (char *) calloc(STR_MAX, sizeof(char));
    char         * ins = (char *) calloc(STR_MAX, sizeof(char) );
    long           len;
    long           cnt;
    int            output;
    int            input;
    long double    sum;

    
    /*analyze input*/
    strcpy(url, getenv("QUERY_STRING"));

    dispense_url('T', url, item, 4, ins);

    if (1 == empty_exist(ins)) {
        error_handler("input missing.", NULL);
    }

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

    transfer_input2(tmpstr, len, 'T', &sum);

    generate_result(sum);
    
    /*meta refresh*/
    printf("Content-length: %d\r\n", strlen(meta));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", meta);
    fflush(stdout);

    exit(0);
}

