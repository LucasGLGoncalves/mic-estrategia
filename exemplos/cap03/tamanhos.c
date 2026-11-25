/* tamanhos.c - quanto espaço cada tipo ocupa e até onde ele vai */
#include <limits.h>
#include <stdio.h>

int main(void) {
    printf("char   ocupa %zu byte(s)\n", sizeof(char));
    printf("int    ocupa %zu byte(s): de %d a %d\n", sizeof(int), INT_MIN,
           INT_MAX);
    printf("long long ocupa %zu byte(s): até %lld\n", sizeof(long long),
           LLONG_MAX);
    printf("float  ocupa %zu byte(s)\n", sizeof(float));
    printf("double ocupa %zu byte(s)\n", sizeof(double));
    return 0;
}
