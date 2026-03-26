#include <stdio.h>

int add(int a, int b) { return a + b; }

int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int t = a + b;
        a = b;
        b = t;
    }
    return b;
}

int main() {
    printf("Testing add(3,7)...\n");
    int r1 = add(3, 7);
    printf("add(3,7) = %d (expected 10)\n", r1);

    printf("Testing add(0,0)...\n");
    int r2 = add(0, 0);
    printf("add(0,0) = %d (expected 0)\n", r2);

    printf("Testing fib(10)...\n");
    int r3 = fibonacci(10);
    printf("fib(10) = %d (expected 55)\n", r3);

    printf("Testing fib(0)...\n");
    int r4 = fibonacci(0);
    printf("fib(0) = %d (expected 0)\n", r4);

    int pass = (r1==10) + (r2==0) + (r3==55) + (r4==0);
    printf("=== %d/4 passed ===\n", pass);
    return pass == 4 ? 0 : 1;
}
