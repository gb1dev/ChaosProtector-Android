public class TestApp {
    // Sensitive strings that should be encrypted
    private static final String API_KEY = "sk-android-ABCDEF123456789";
    private static final String DB_CONN = "postgres://admin:S3cretP@ss@db.internal:5432/prod";
    private static final String LICENSE = "CP-PRO-XXXX-YYYY-ZZZZ";

    public static int add(int a, int b) { return a + b; }
    public static int multiply(int a, int b) { return a * b; }

    public static int fibonacci(int n) {
        if (n <= 1) return n;
        int a = 0, b = 1;
        for (int i = 2; i <= n; i++) {
            int t = a + b;
            a = b;
            b = t;
        }
        return b;
    }

    public static boolean isPrime(int n) {
        if (n < 2) return false;
        if (n < 4) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;
        for (int i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }
        return true;
    }

    public static String getApiKey() { return API_KEY; }
    public static String getDbConn() { return DB_CONN; }

    public static void main(String[] args) {
        int passed = 0, failed = 0;

        if (add(3, 7) == 10) passed++; else { System.out.println("FAIL: add"); failed++; }
        if (multiply(6, 7) == 42) passed++; else { System.out.println("FAIL: mul"); failed++; }
        if (fibonacci(10) == 55) passed++; else { System.out.println("FAIL: fib10"); failed++; }
        if (fibonacci(20) == 6765) passed++; else { System.out.println("FAIL: fib20"); failed++; }
        if (isPrime(7)) passed++; else { System.out.println("FAIL: prime7"); failed++; }
        if (isPrime(997)) passed++; else { System.out.println("FAIL: prime997"); failed++; }
        if (!isPrime(100)) passed++; else { System.out.println("FAIL: notprime"); failed++; }
        if (getApiKey().equals("sk-android-ABCDEF123456789")) passed++; else { System.out.println("FAIL: apikey"); failed++; }
        if (getDbConn().contains("S3cretP@ss")) passed++; else { System.out.println("FAIL: dbconn"); failed++; }

        System.out.println("=== " + passed + " passed, " + failed + " failed ===");
        if (failed > 0) System.exit(1);
    }
}
