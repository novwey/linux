#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

long parse_long(const char *s, int *ok){
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    *ok = (errno == 0 && *end == '\0');
    return v;
}

int main(int argc, char *argv[]){
    if(argc != 4){
        fprintf(stderr, "Usage: %s <add|sub|mul|div|mod> <x> <y>\n", argv[0]);
        return 1;
    }
    const char *op = argv[1];
    int okx=0, oky=0;
    long x = parse_long(argv[2], &okx);
    long y = parse_long(argv[3], &oky);
    if(!okx || !oky){
        fprintf(stderr, "Error: x,y는 정수여야 합니다.\n");
        return 1;
    }
    if(strcmp(op, "add")==0)      printf("%ld\n", x + y);
    else if(strcmp(op, "sub")==0) printf("%ld\n", x - y);
    else if(strcmp(op, "mul")==0) printf("%ld\n", x * y);
    else if(strcmp(op, "div")==0){
        if(y==0){ fprintf(stderr,"Error: 0으로 나눌 수 없음\n"); return 1; }
        printf("%ld\n", x / y);
    } else if(strcmp(op, "mod")==0){
        if(y==0){ fprintf(stderr,"Error: 0으로 나눌 수 없음\n"); return 1; }
        printf("%ld\n", x % y);
    } else {
        fprintf(stderr, "Error: 지원하지 않는 op\n");
        return 1;
    }
    return 0;
}
