#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    def __init__(self):
        super().__init__()
    def obfuscate_arithmetic(self, mod: chaos_android.Module,
                                   fun: chaos_android.Function) -> chaos_android.ArithmeticOpt:
        return chaos_android.ArithmeticOpt(rounds=2) if chaos_android.ObfuscationConfig.default_config(self, mod, fun, [], [], [], 0) else False

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
