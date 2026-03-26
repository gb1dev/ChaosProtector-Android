#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from difflib import unified_diff
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    excluded_function_value = "check_password"
    chaos_android.config.global_func_exclude = [excluded_function_value]

    def __init__(self):
        super().__init__()
    def flatten_cfg(self, mod: chaos_android.Module, func: chaos_android.Function):
        if (func.name == self.excluded_function_value):
             print("flatten_cfg is called")
        return True

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
