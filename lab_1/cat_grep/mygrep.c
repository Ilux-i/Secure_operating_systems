#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE_LEN 1024

static int grep_stream(const char *pattern, FILE *fp, const char *filename, int show_filename) {
    char buf[MAX_LINE_LEN];
    int found = 0;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (strstr(buf, pattern) != NULL) {
            if (show_filename) {
                fprintf(stdout, "%s:", filename);
            }
            fputs(buf, stdout);
            found = 1;
        }
    }

    if (ferror(fp)) {
        return 2;
    }

    return found ? 0 : 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 2;
    }

    const char *pattern = argv[1];
    int had_error = 0;
    int any_found = 0;
    
    int show_filename = (argc > 3) ? 1 : 0;

    if (argc == 2) {
        int res = grep_stream(pattern, stdin, "(stdin)", 0);
        if (res == 2) had_error = 1;
        else if (res == 0) any_found = 1;
    } else {
        for (int i = 2; i < argc; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (fp == NULL) {
                fprintf(stderr, "mygrep: %s: %s\n", argv[i], strerror(errno));
                had_error = 1;
                continue;
            }

            int res = grep_stream(pattern, fp, argv[i], show_filename);
            if (res == 2) {
                had_error = 1;
            } else if (res == 0) {
                any_found = 1;
            }

            fclose(fp);
        }
    }

    if (had_error) return 2;
    if (!any_found) return 1;
    return 0;
}