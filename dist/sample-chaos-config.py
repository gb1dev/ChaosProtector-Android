import chaos_android
from functools import lru_cache

class MyConfig(chaos_android.ObfuscationConfig):
    def __init__(self):
        super().__init__()

    def obfuscate_arithmetic(self, mod: chaos_android.Module,
                                   fun: chaos_android.Function) -> chaos_android.ArithmeticOpt:
        return True

    def flatten_cfg(self, mod: chaos_android.Module, func: chaos_android.Function):
        return True

    def obfuscate_string(self, _, __, string: bytes):
        return chaos_android.StringEncOptGlobal()

    def indirect_call(self, mod: chaos_android.Module, func: chaos_android.Function):
        return chaos_android.ObfuscationConfig.default_config(self, mod, func, [], [], [], 10)

    def break_control_flow(self, mod: chaos_android.Module, func: chaos_android.Function):
        return chaos_android.ObfuscationConfig.default_config(self, mod, func, [], [], [], 10)

    def function_outline(self, _, __):
        return chaos_android.FunctionOutlineWithProbability(10)

    def basic_block_duplicate(self, _, __):
        return chaos_android.BasicBlockDuplicateWithProbability(10)

@lru_cache(maxsize=1)
def chaos_get_config() -> chaos_android.ObfuscationConfig:
    """
    Return an instance of `ObfuscationConfig` which
    aims at describing the obfuscation scheme
    """
    return MyConfig()
