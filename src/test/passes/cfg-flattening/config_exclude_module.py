#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    chaos_android.config.global_mod_exclude = ["global-exclude-"]

    def __init__(self):
        super().__init__()
    def flatten_cfg(self, mod: chaos_android.Module, func: chaos_android.Function):
        print("cfg-flattening is called")
        return True

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
