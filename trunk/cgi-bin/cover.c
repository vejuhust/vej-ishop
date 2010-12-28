#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern char     tmpstr  [];

int unix_error(char * msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void Execve(const char * filename, char * const argv[], char * const envp[]) {
    if (execve(filename, argv, envp) < 0)
        unix_error("Execve error");
}

void recovery_url(char * str) {
    char * sp;
    long   i = 0;
    long   t = 0;
    char   c;

    sp = str;
    while ('\0' != *sp) {
        if ('+' == *sp) {
            *sp = ' ';
        }
        ++sp;
    }

    sp = str;
    while ('\0' != *sp) {
        if ('%' != *sp) {
            tmpstr[i++] = *sp;
            ++sp;
        }
        else {
            ++sp;
            c = *sp;
            if (0 != isdigit(c)) {
                t = (c - '0') * 16;
            }
            else {
                t = (tolower(c) - 'a' + 10) * 16;
            }

            ++sp;
            c = *sp;
            if (0 != isdigit(c)) {
                t += (c - '0');
            }
            else {
                t += (tolower(c) - 'a' + 10);
            }

            tmpstr[i++] = (char) t;
            ++sp;
        }
    }
    tmpstr[i] = '\0';

    strcpy(str, tmpstr);
}

