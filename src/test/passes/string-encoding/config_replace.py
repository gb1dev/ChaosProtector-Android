#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    def __init__(self):
        super().__init__()
    def obfuscate_string(self, _, __, string: bytes):
        if string.startswith(b"Hello"):
            return chaos_android.StringEncOptLocal()
        if string.endswith(b"Hello"):
            return chaos_android.StringEncOptGlobal()
        if string.endswith(b".cpp"):
            return chaos_android.StringEncOptGlobal()
        if string.endswith(b"Swift"):
            return chaos_android.StringEncOptLocal()
        if string.endswith(b"Stack"):
            return chaos_android.StringEncOptLocal()

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
