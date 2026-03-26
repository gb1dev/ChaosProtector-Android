#
# This file is distributed under the Apache License v2.0. See LICENSE for details.
#

import chaos_android
from functools import lru_cache
from pathlib import Path

class MyConfig(chaos_android.ObfuscationConfig):
    current_path = Path(__file__).resolve().parent
    chaos_android.config.output_folder = str(current_path) + "/tmp/"

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    return MyConfig()
