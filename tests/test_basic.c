#include <stdio.h>
#include <string.h>
#include <math.h>

// Sensitive strings that should be encrypted
static const char *API_KEY = "sk-android-ABCDEF123456789";
static const char *DB_CONN = "postgres://admin:S3cretP@ss@db.internal:5432/prod";
static const char *LICENSE_KEY = "CP-TEST-XXXX-YYYY-ZZZZ";

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

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

int gcd(int a, int b) {
    while (b) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int is_prime(int n) {
    if (n < 2) return 0;
    if (n < 4) return 1;
    if (n % 2 == 0 || n % 3 == 0) return 0;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return 0;
    }
    return 1;
}

// Sort array (bubble sort for simplicity)
void sort_array(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

int bit_reverse(unsigned int x) {
    x = ((x & 0x55555555) << 1) | ((x & 0xAAAAAAAA) >> 1);
    x = ((x & 0x33333333) << 2) | ((x & 0xCCCCCCCC) >> 2);
    x = ((x & 0x0F0F0F0F) << 4) | ((x & 0xF0F0F0F0) >> 4);
    x = ((x & 0x00FF00FF) << 8) | ((x & 0xFF00FF00) >> 8);
    return (x << 16) | (x >> 16);
}

int popcount(unsigned int x) {
    int c = 0;
    while (x) { c += x & 1; x >>= 1; }
    return c;
}

int main() {
    int passed = 0, failed = 0;

#define CHECK(name, ok) do { \
    if (ok) { passed++; } \
    else { printf("  FAIL: %s\n", name); failed++; } \
} while(0)

    printf("=== ChaosProtector Android Test ===\n");

    // Strings
    CHECK("api_key", strcmp(API_KEY, "sk-android-ABCDEF123456789") == 0);
    CHECK("db_conn", strstr(DB_CONN, "S3cretP@ss") != NULL);
    CHECK("license", strncmp(LICENSE_KEY, "CP-TEST", 7) == 0);

    // Arithmetic
    CHECK("add", add(3, 7) == 10);
    CHECK("mul", multiply(6, 7) == 42);
    CHECK("fib10", fibonacci(10) == 55);
    CHECK("fib20", fibonacci(20) == 6765);
    CHECK("gcd", gcd(48, 18) == 6);
    CHECK("prime7", is_prime(7) == 1);
    CHECK("prime997", is_prime(997) == 1);
    CHECK("not_prime100", is_prime(100) == 0);

    // Sorting
    int arr[] = {9, 3, 7, 1, 5, 8, 2, 6, 4, 0};
    sort_array(arr, 10);
    CHECK("sort_first", arr[0] == 0);
    CHECK("sort_last", arr[9] == 9);

    // Bit ops
    CHECK("bitrev", bit_reverse(1) == (int)0x80000000);
    CHECK("popcount", popcount(0xFF) == 8);

    // Math
    CHECK("sqrt", fabs(sqrt(2.0) - 1.4142) < 0.001);
    CHECK("pi", fabs(M_PI - 3.14159) < 0.001);

    printf("\n=== %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
