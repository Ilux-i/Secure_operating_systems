#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

int apply_symbolic_mode(mode_t *mode, const char *symbolic) {
    bool apply_user = false;
    bool apply_group = false;
    bool apply_other = false;
    int i = 0;
    
    // парсинг классов
    while (symbolic[i] && symbolic[i] != '+' && symbolic[i] != '-' && symbolic[i] != '=') {
        switch (symbolic[i]) {
            case 'u': apply_user = true; break;
            case 'g': apply_group = true; break;
            case 'o': apply_other = true; break;
            case 'a': apply_user = apply_group = apply_other = true; break;
            default:
                return -1;
        }
        i++;
    }
    
    // значение по умолчанию
    if (!apply_user && !apply_group && !apply_other) {
        apply_user = apply_group = apply_other = true;
    }
    
    char op = symbolic[i];
    if (op != '+' && op != '-' && op != '=') {
        return -1;
    }
    i++;
    
    // определяем права
    bool perm_r = false;
    bool perm_w = false;
    bool perm_x = false;
    while (symbolic[i]) {
        switch (symbolic[i]) {
            case 'r': perm_r = true; break;
            case 'w': perm_w = true; break;
            case 'x': perm_x = true; break;
            default:
                return -1;
        }
        i++;
    }
    
    // маски прав
    mode_t user_mask = S_IRUSR | S_IWUSR | S_IXUSR;
    mode_t group_mask = S_IRGRP | S_IWGRP | S_IXGRP;
    mode_t other_mask = S_IROTH | S_IWOTH | S_IXOTH;
    
    // обновление прав
    mode_t new_perms = 0;
    if (perm_r) new_perms |= (S_IRUSR | S_IRGRP | S_IROTH);
    if (perm_w) new_perms |= (S_IWUSR | S_IWGRP | S_IWOTH);
    if (perm_x) new_perms |= (S_IXUSR | S_IXGRP | S_IXOTH);
    
    switch (op) {
        case '+':
            if (apply_user)  *mode |= (new_perms & user_mask);
            if (apply_group) *mode |= (new_perms & group_mask);
            if (apply_other) *mode |= (new_perms & other_mask);
            break;
            
        case '-':
            if (apply_user)  *mode &= ~(new_perms & user_mask);
            if (apply_group) *mode &= ~(new_perms & group_mask);
            if (apply_other) *mode &= ~(new_perms & other_mask);
            break;
            
        case '=':
            if (apply_user) {
                *mode &= ~user_mask;
                *mode |= (new_perms & user_mask);
            }
            if (apply_group) {
                *mode &= ~group_mask;
                *mode |= (new_perms & group_mask);
            }
            if (apply_other) {
                *mode &= ~other_mask;
                *mode |= (new_perms & other_mask);
            }
            break;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        return 1;
    }
    
    const char *mode_str = argv[1];
    
    bool is_numeric = true;
    for (int i = 0; mode_str[i]; i++) {
        if (mode_str[i] < '0' || mode_str[i] > '7') {
            is_numeric = false;
            break;
        }
    }
    
    for (int i = 2; i < argc; i++) {
        const char *filename = argv[i];
        struct stat st;
        
        if (stat(filename, &st) == -1) {
            return 1;
        }
        
        mode_t new_mode = st.st_mode;
        
        if (is_numeric) {
            long mode_val = strtol(mode_str, NULL, 8);
            if (mode_val < 0 || mode_val > 07777) {
                return 1;
            }
            new_mode = (st.st_mode & ~07777) | (mode_t)mode_val;
        } else {
            if (apply_symbolic_mode(&new_mode, mode_str) == -1) {
                return 1;
            }
        }
        
        if (chmod(filename, new_mode) == -1) {
            return 1;
        }
    }
    
    return 0;
}