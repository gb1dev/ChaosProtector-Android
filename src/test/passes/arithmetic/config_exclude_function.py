#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    excluded_function_value = "memcpy_xor"
    chaos_android.config.global_func_exclude = [excluded_function_value]

    def __init__(self):
        super().__init__()
    def obfuscate_arithmetic(self, mod: chaos_android.Module,
                                   fun: chaos_android.Function) -> chaos_android.ArithmeticOpt:
        if (fun.name == self.excluded_function_value):
            print("obfuscate_arithmetic is called")
        return chaos_android.ArithmeticOpt(rounds=2)

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
