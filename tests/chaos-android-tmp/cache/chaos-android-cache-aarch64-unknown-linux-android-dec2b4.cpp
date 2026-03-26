extern "C" {

    void decode(char *out, char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)] ^ i;
      }
    }
  }
