#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#define MAX_LINE_LEN 1024

static bool flag_number_all = false;
static bool flag_number_nonblank = false;
static bool flag_show_ends = false;

static bool is_blank_line(const char *line) {
    if (line[0] == '\0') return true;
    if (line[0] == '\n' && line[1] == '\0') return true;
    return false;
}

static int cat_stream(const char *filename, FILE *fp) {
    char buf[MAX_LINE_LEN];
    int line_number = 0;

    bool do_number_all = flag_number_all && !flag_number_nonblank;
    bool do_number_nonblank = flag_number_nonblank;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        bool should_number = false;

        if (do_number_nonblank) {
            should_number = !is_blank_line(buf);
        } else if (do_number_all) {
            should_number = true;
        }

        if (should_number) {
            line_number++;
            printf("%6d\t", line_number);
        } else if (flag_show_ends && (do_number_all || do_number_nonblank)) {

		    printf("     ");
		}

        if (flag_show_ends) {
            size_t len = strlen(buf);
            if (len > 0 && buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
                fputs(buf, stdout);
                putchar('$');
                putchar('\n');
            } else {
                fputs(buf, stdout);
                putchar('$');
            }
        } else {
            fputs(buf, stdout);
        }
    }

    if (ferror(fp)) {
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int opt;
    int had_error = 0;

    while ((opt = getopt(argc, argv, "nbE")) != -1) {
        switch (opt) {
            case 'n': flag_number_all = true;       break;
            case 'b': flag_number_nonblank = true;  break;
            case 'E': flag_show_ends = true;        break;
            default:
                return 1;
        }
    }

    if (optind >= argc) {
        had_error = cat_stream("stdin", stdin);
    } else {
        for (int i = optind; i < argc; i++) {
            if (strcmp(argv[i], "-") == 0) {
                if (cat_stream("stdin", stdin) != 0)
                    had_error = 1;
                continue;
            }

            FILE *fp = fopen(argv[i], "r");
            if (fp == NULL) {
                fprintf(stderr, "mycat: %s: %s\n",
                        argv[i], strerror(errno));
                had_error = 1;
                continue;
            }
            if (cat_stream(argv[i], fp) != 0)
                had_error = 1;
            fclose(fp);
        }
    }

    return had_error ? 1 : 0;
}