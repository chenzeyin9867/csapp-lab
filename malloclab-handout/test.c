#include <stdio.h>
int main(){
    int * a = (int*) 0x12341230;
    a ++;
    printf("%x\n", a);
    printf("%d\n", sizeof(int*));
    return 0;
}