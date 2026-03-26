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
    int passed = 0, failed = 0;

    if (add(3, 7) == 10) passed++; else { printf("FAIL: add\n"); failed++; }
    if (add(0, 0) == 0) passed++; else { printf("FAIL: add0\n"); failed++; }
    if (add(-5, 5) == 0) passed++; else { printf("FAIL: addneg\n"); failed++; }
    if (fibonacci(10) == 55) passed++; else { printf("FAIL: fib10\n"); failed++; }
    if (fibonacci(0) == 0) passed++; else { printf("FAIL: fib0\n"); failed++; }
    if (fibonacci(1) == 1) passed++; else { printf("FAIL: fib1\n"); failed++; }

    printf("=== %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
