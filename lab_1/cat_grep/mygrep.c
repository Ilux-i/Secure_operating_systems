#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 1024

static int grep_stream(const char *pattern, FILE *fp) {
    char buf[MAX_LINE_LEN];
    int found = 0;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (strstr(buf, pattern) != NULL) {
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

    if (argc == 2) {
        return grep_stream(pattern, stdin);
    }

    for (int i = 2; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (fp == NULL) {
            had_error = 1;
            continue;
        }

        if (grep_stream(pattern, fp) == 2) {
            had_error = 1;
        }

        fclose(fp);
    }

    return had_error ? 2 : 0;
}