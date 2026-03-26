#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from functools import lru_cache

@lru_cache(maxsize=1)
def chaos_get_config():
    return chaos_android.ObfuscationConfig()
