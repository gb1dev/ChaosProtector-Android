#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    chaos_android.config.shuffle_functions = False

    def __init__(self):
        super().__init__()
    def indirect_branch(self, mod: chaos_android.Module, func: chaos_android.Function):
        return True

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
