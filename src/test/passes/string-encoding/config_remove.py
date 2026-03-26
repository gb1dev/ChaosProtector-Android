import chaos_android
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    def __init__(self):
        super().__init__()
    def obfuscate_string(self, _, __, string: bytes):
        if string.endswith(b".cpp"):
            return "REDACTED"

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
