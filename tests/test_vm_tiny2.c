#include <stdio.h>
int add(int a, int b) { return a + b; }
int main() {
    printf("Testing add(3,7)...\n");
    int r1 = add(3, 7);
    printf("add(3,7) = %d (expected 10)\n", r1);
    printf("Testing add(0,0)...\n");
    int r2 = add(0, 0);
    printf("add(0,0) = %d (expected 0)\n", r2);
    return (r1==10 && r2==0) ? 0 : 1;
}
