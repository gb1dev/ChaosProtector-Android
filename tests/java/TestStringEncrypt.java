import java.util.Base64;

public class TestStringEncrypt {
    static final int KEY = 0x5A3C9E7F;

    static String encrypt(String s) {
        byte[] bytes = s.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        byte[] result = new byte[bytes.length];
        for (int i = 0; i < bytes.length; i++) {
            result[i] = (byte)(bytes[i] ^ ((KEY >> (i % 4 * 8)) & 0xFF) ^ (i * 7 + 13));
        }
        return Base64.getEncoder().encodeToString(result);
    }

    static String decrypt(String encoded, int key) {
        byte[] data = Base64.getDecoder().decode(encoded);
        for (int i = 0; i < data.length; i++) {
            data[i] = (byte)(data[i] ^ ((key >> (i % 4 * 8)) & 0xFF) ^ (i * 7 + 13));
        }
        return new String(data, java.nio.charset.StandardCharsets.UTF_8);
    }

    public static void main(String[] args) {
        String[] testStrings = {
            "sk-android-ABCDEF123456789",
            "postgres://admin:S3cretP@ss@db.internal:5432/prod",
            "CP-PRO-XXXX-YYYY-ZZZZ",
            "Hello World! This is a test string.",
            "SELECT * FROM users WHERE password = 'secret'"
        };

        int passed = 0;
        for (String original : testStrings) {
            String encrypted = encrypt(original);
            String decrypted = decrypt(encrypted, KEY);

            if (decrypted.equals(original)) {
                passed++;
                // Verify encrypted != original
                if (!encrypted.equals(original)) {
                    System.out.println("  OK: \"" + original.substring(0, Math.min(30, original.length())) + "...\" -> encrypted " + encrypted.length() + " chars");
                }
            } else {
                System.out.println("  FAIL: \"" + original + "\" decrypted to \"" + decrypted + "\"");
            }
        }
        System.out.println("=== String Encryption: " + passed + "/" + testStrings.length + " passed ===");
    }
}
