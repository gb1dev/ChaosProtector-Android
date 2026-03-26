#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    excluded_function_value = "check_code"
    chaos_android.config.global_func_exclude = [excluded_function_value]

    def __init__(self):
        super().__init__()
    def obfuscate_string(self, _, __, string: bytes):
        if string.endswith(b"ObfuscateMe"):
            print("string-encoding is called")
            return chaos_android.StringEncOptGlobal()

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
