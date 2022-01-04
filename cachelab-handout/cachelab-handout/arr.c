#include <stdio.h>
void f(int (*a)[4]){
    printf("%d %d %d %d\n", *a[0], *(a + 1)[0], *(a + 2)[0], *(a + 3)[0]);
}

struct A{
    int a;
};

int main(){
    int a[4][4] = {0, 1, 2, 3, 4, 5, 6 , 7, 8, 9, 10, 11, 12, 13, 14, 15};
    f(a);
    struct A m;
    printf("A->a:%d\n", m.a);
    return 0;
}