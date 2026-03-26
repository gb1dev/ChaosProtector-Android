#include <stdio.h>
int add(int a, int b) { return a + b; }
int main() {
    int r = add(3, 7);
    printf("%d\n", r);
    return r == 10 ? 0 : 1;
}
