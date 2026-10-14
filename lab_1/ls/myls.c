#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>

#define COLOR_RESET     "\033[0m"
#define COLOR_BLUE      "\033[1;34m"
#define COLOR_GREEN     "\033[1;32m"
#define COLOR_CYAN      "\033[1;36m"

typedef struct {
    char name[256];
    char path[512];
    struct stat statbuf;
} FileInfo;

static bool flag_long = false;
static bool flag_all = false;

int compare_files(const void *a, const void *b) {
    const FileInfo *fa = (const FileInfo *)a;
    const FileInfo *fb = (const FileInfo *)b;
    return strcmp(fa->name, fb->name);
}

const char* get_file_color(mode_t mode) {
    if (S_ISLNK(mode)) {
        return COLOR_CYAN;
    } else if (S_ISDIR(mode)) {
        return COLOR_BLUE;
    } else if (S_ISREG(mode) && (mode & S_IXUSR)) {
        return COLOR_GREEN;
    }
    return "";
}

void print_permissions(mode_t mode) {
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');

    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');
    
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}

void print_long_format(FileInfo *file) {
    struct passwd *pwd = getpwuid(file->statbuf.st_uid);
    struct group *grp = getgrgid(file->statbuf.st_gid);
    char timebuf[64];
    struct tm *tm = localtime(&file->statbuf.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);
    
    char type;
    if (S_ISDIR(file->statbuf.st_mode)) type = 'd';
    else if (S_ISLNK(file->statbuf.st_mode)) type = 'l';
    else if (S_ISCHR(file->statbuf.st_mode)) type = 'c';
    else if (S_ISBLK(file->statbuf.st_mode)) type = 'b';
    else if (S_ISFIFO(file->statbuf.st_mode)) type = 'p';
    else if (S_ISSOCK(file->statbuf.st_mode)) type = 's';
    else type = '-';
    
    char uid_buf[16];
    char gid_buf[16];
    const char *uname;
    const char *gname;

    if (pwd != NULL) {
        uname = pwd->pw_name;
    } else {
        snprintf(uid_buf, sizeof(uid_buf), "%u",
                 (unsigned int)file->statbuf.st_uid);
        uname = uid_buf;
    }

    if (grp != NULL) {
        gname = grp->gr_name;
    } else {
        snprintf(gid_buf, sizeof(gid_buf), "%u",
                 (unsigned int)file->statbuf.st_gid);
        gname = gid_buf;
    }
    
    printf("%c", type);
    print_permissions(file->statbuf.st_mode);
    printf(" %3ld %s %s %8ld %s ",
           (long)file->statbuf.st_nlink,
           uname,
           gname,
           (long)file->statbuf.st_size,
           timebuf);
    
    const char *color = get_file_color(file->statbuf.st_mode);
    printf("%s%s%s", color, file->name, COLOR_RESET);
    
    if (S_ISLNK(file->statbuf.st_mode)) {
        char link_target[512];
        ssize_t len = readlink(file->path, link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            printf(" -> %s", link_target);
        }
    }
    
    printf("\n");
}

void list_directory(const char *dirname) {
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        fprintf(stderr, "myls: cannot access '%s': %s\n", dirname, strerror(errno));
        return;
    }
    
    FileInfo files[1000];
    int file_count = 0;
    long total_blocks = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!flag_all && entry->d_name[0] == '.') {
            continue;
        }
        
        strncpy(files[file_count].name, entry->d_name, sizeof(files[file_count].name) - 1);
        files[file_count].name[sizeof(files[file_count].name) - 1] = '\0';

        snprintf(files[file_count].path, sizeof(files[file_count].path),
            "%s/%s", dirname, entry->d_name);
        
        if (lstat(files[file_count].path, &files[file_count].statbuf) == -1) {
            continue;
        }
        
        total_blocks += files[file_count].statbuf.st_blocks;
        
        file_count++;
    }
    
    closedir(dir);
    
    qsort(files, file_count, sizeof(FileInfo), compare_files);
    
    if (flag_long) {
        printf("total %ld\n", total_blocks / 2);
    }
    
    for (int i = 0; i < file_count; i++) {
        if (flag_long) {
            print_long_format(&files[i]);
        } else {
            const char *color = get_file_color(files[i].statbuf.st_mode);
            printf("%s%s%s\n", color, files[i].name, COLOR_RESET);
        }
    }
}

int main(int argc, char *argv[]) {
    int opt;
    
    while ((opt = getopt(argc, argv, "la")) != -1) {
        switch (opt) {
            case 'l':
                flag_long = true;
                break;
            case 'a':
                flag_all = true;
                break;
            default:
                return 1;
        }
    }
    
    if (optind >= argc) {
        list_directory(".");
    } else {
        for (int i = optind; i < argc; i++) {
            if (argc - optind > 1) {
                printf("%s:\n", argv[i]);
            }
            list_directory(argv[i]);
            if (i < argc - 1) {
                printf("\n");
            }
        }
    }
    
    return 0;
}