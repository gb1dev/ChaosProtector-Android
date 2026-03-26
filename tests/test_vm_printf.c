#include <stdio.h>
int add(int a, int b) { return a + b; }
int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) { int t = a + b; a = b; b = t; }
    return b;
}
int main() {
    printf("add(3,7) = %d\n", add(3, 7));
    printf("fib(10) = %d\n", fibonacci(10));
    printf("fib(0) = %d\n", fibonacci(0));
    printf("fib(1) = %d\n", fibonacci(1));
    return 0;
}
