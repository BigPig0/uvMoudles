#include <stdlib.h>
#include <string.h>
//#include <io.h>
#include <direct.h>


int file_sys_check_path(const char *path) {
    int i, len;
    char *str;
    len = strlen(path);
    str = (char*)calloc(1, len+1);
    strcpy(str, path);

    for (i = 0; i < len; i++){
        char tmp = str[i];
        if (tmp == '/' || tmp == '\\'){
            str[i] = '\0';
            if (access(str, 0) != 0){
                if(0 != mkdir(str)) {
                    //����Ŀ¼ʧ��
                    free(str);
                    return -1;
                }
            }
            str[i] = tmp;
        }
    }
    free(str);
    return 0;
}

int file_sys_exist(const char *path) {
    return access(path, 0);
}