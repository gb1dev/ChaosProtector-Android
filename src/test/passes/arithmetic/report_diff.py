#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from difflib import unified_diff
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    def __init__(self):
        super().__init__()
    def obfuscate_arithmetic(self, mod: chaos_android.Module,
                                   fun: chaos_android.Function) -> chaos_android.ArithmeticOpt:
        return chaos_android.ArithmeticOpt(rounds=2)
    def report_diff(self, pass_name: str, original: str, obfuscated: str):
        print(pass_name, "applied obfuscation:")
        diff = unified_diff(original.splitlines(), obfuscated.splitlines(),
                            'original', 'obfuscated', lineterm='')
        for line in diff:
            print(line)

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
