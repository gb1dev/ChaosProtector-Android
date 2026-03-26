//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <dlfcn.h>

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"

#include "chaos_android/PyConfig.hpp"
#include "chaos_android/log.hpp"
#include "chaos_android/chaos_config.hpp"
#include "chaos_android/utils.hpp"
#include "chaos_android/versioning.hpp"

#include "PyObfuscationConfig.hpp"
#include "init.hpp"

namespace py = pybind11;

using namespace pybind11::literals;

namespace chaos_android {

void initPythonpath() {
  if (!PyConfig::YConfig.PythonPath.empty()) {
    Py_SetPath(Py_DecodeLocale(PyConfig::YConfig.PythonPath.c_str(), nullptr));
    setenv("PYTHONHOME", PyConfig::YConfig.PythonPath.c_str(), true);
    return;
  }

  if (char *Config = getenv(PyConfig::PyEnv_Key)) {
    Py_SetPath(Py_DecodeLocale(Config, nullptr));
    setenv("PYTHONHOME", Config, true);
    return;
  }

#if defined(__linux__)
  if (auto *Hdl = dlopen("libpython3.10.so", RTLD_LAZY)) {
    char Path[400];
    int Ret = dlinfo(Hdl, RTLD_DI_ORIGIN, Path);
    if (Ret != 0)
      return;

    std::string PythonPath = Path;
    PythonPath.append("/python3.10");
    Py_SetPath(Py_DecodeLocale(PythonPath.c_str(), nullptr));
    setenv("PYTHONHOME", PythonPath.c_str(), true);
    return;
  }
#endif
}

void ChaosAndroidCtor(py::module_ &m) {
  initDefaultConfig();

  m.attr("LLVM_VERSION")  = CHAOS_ANDROID_LLVM_VERSION_STRING;
  m.attr("CHAOS_ANDROID_VERSION") = CHAOS_ANDROID_VERSION;
  m.attr("CHAOS_ANDROID_VERSION_FULL") = "CHAOS_ANDROID Version: " CHAOS_ANDROID_VERSION " / " CHAOS_ANDROID_LLVM_VERSION_STRING
                                 " (" CHAOS_ANDROID_LLVM_VERSION ")";

  py::class_<ChaosConfig>(m, "ChaosConfig",
                          R"delim(
    This class is used to configure the global behavior of ChaosProtector Android.

    It can be accessed through the global :attr:`chaos_android.config` attribute
    )delim")
      .def_readwrite("passes", &ChaosConfig::Passes,
                     R"delim(
                   This **ordered** list contains the sequence of the obfuscation passes
                   that must be used.
                   It should not be modified unless you know what you do.

                   This attribute is set by default to these values:

                   |chaos-android-passes|

                   )delim")

      .def_readwrite("inline_jni_wrappers", &ChaosConfig::InlineJniWrappers,
                     R"delim(
                   This boolean attribute is used to force inlining JNI C++ wrapper.
                   For instance ``GetStringChars``:

                   .. code-block:: cpp

                     const jchar* GetStringChars(jstring string, jboolean* isCopy)
                     { return functions->GetStringChars(this, string, isCopy); }

                   The default value is ``True``.
                   )delim")

      .def_readwrite("shuffle_functions", &ChaosConfig::ShuffleFunctions,
                     R"delim(
                    Whether the postition of Module's functions should be shuffled.

                    This randomization is used to avoid the same (relative) position of the functions
                    for two different builds of the protected binary.

                    For instance, if the original source code is composed of the following functions:

                    .. code-block:: cpp

                        void hello();
                        void hello_world();
                        void say_hi();

                    In the final (stripped) binary, the functions appear as the following sequence:

                    .. code-block:: text

                        sub_3455() // hello
                        sub_8A74() // hello_world
                        sub_AF34() // say_hi

                    If this value is set to ``True`` (which is the default value), the sequence is randomized.
                    )delim")

      .def_readwrite("global_mod_exclude", &ChaosConfig::GlobalModuleExclude,
                     R"delim(
                    This attribute is a list of strings used to exclude entire modules from obfuscation.
                    Each entry in the list can be a partial or full match of the module's name.
                    For example, if a module's name is `a/b/c/d.cpp`, it can be excluded by including `"b/"` in the list.
                    When a module is excluded, none of the obfuscation passes will be applied to it.
                    
                    By default, this list is empty.
                    )delim")

      .def_readwrite("global_func_exclude", &ChaosConfig::GlobalFunctionExclude,
                     R"delim(
                    This attribute is a list of strings used to exclude specific functions from obfuscation.
                    When a function is excluded, none of the obfuscation passes will be applied to it.
                    
                    By default, this list is empty.
                    )delim")

      .def_readwrite("probability_seed", &ChaosConfig::ProbabilitySeed,
                     R"delim(
                    probability_seed is a configurable value used to initialize the random number generator.
                    Whenever a random value is required during the obfuscation process,
                    the generator will use this predefined seed to ensure deterministic and reproducible randomness.

                    The default value is 1.
                    )delim")

      .def_readwrite("output_folder", &ChaosConfig::OutputFolder,
                     R"delim(
                    Output directory where o-mvll stores processed files (e.g., log files).

                    By default, this value is empty.
                    )delim");

  m.attr("config") = &Config;

  py_init_obf_opt(m);
  py_init_llvm_bindings(m);
  py_init_log(m);

  py::class_<ObfuscationConfig, PyObfuscationConfig>(m, "ObfuscationConfig",
                                                     R"delim(
    This class must be inherited by the user to define where and how the obfuscation
    passes must be enabled.
    )delim")
      .def(py::init<>())

      .def("obfuscate_string", &ObfuscationConfig::obfuscateString,
           R"delim(
         The default user-callback used to configure strings obfuscation.

         In addition to the associated class options, ChaosProtector Android interprets these return values as follows:

         +--------------+-------------------------------------+
         | Return Value | Interpretation                      |
         +==============+=====================================+
         | ``None``     | :class:`~chaos_android.StringEncOptSkip`    |
         +--------------+-------------------------------------+
         | ``False``    | :class:`~chaos_android.StringEncOptSkip`    |
         +--------------+-------------------------------------+
         | ``True``     | :class:`~chaos_android.StringEncOptDefault` |
         +--------------+-------------------------------------+
         | ``str``      | :class:`~chaos_android.StringEncOptReplace` |
         +--------------+-------------------------------------+
         | ``bytes``    | :class:`~chaos_android.StringEncOptReplace` |
         +--------------+-------------------------------------+

         See the :chaos_android:`strings-encoding` documentation.
         )delim",
           "module"_a, "function"_a, "string"_a)

      .def("break_control_flow", &ObfuscationConfig::breakControlFlow,
           R"delim(
         The default user-callback for the pass that breaks
         the control flow.

         In addition to the associated class options, ChaosProtector Android interprets these return values as follows:

         +--------------+-------------------------------------------------+
         | Return Value | Interpretation                                  |
         +==============+=================================================+
         | ``True``     | :class:`~chaos_android.BreakControlFlowOpt`\(``True``)  |
         +--------------+-------------------------------------------------+
         | ``False``    | :class:`~chaos_android.BreakControlFlowOpt`\(``False``) |
         +--------------+-------------------------------------------------+
         | ``None``     | :class:`~chaos_android.BreakControlFlowOpt`\(``False``) |
         +--------------+-------------------------------------------------+

         See the :chaos_android:`control-flow-breaking` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("flatten_cfg", &ObfuscationConfig::controlFlowGraphFlattening,
           R"delim(
         The default user-callback used to configure the
         control-flow flattening pass.

         In addition to the associated class options, ChaosProtector Android interprets these return values as follows:

         +--------------+------------------------------------------------------+
         | Return Value | Interpretation                                       |
         +==============+======================================================+
         | ``True``     | :class:`~chaos_android.ControlFlowFlatteningOpt`\(``True``)  |
         +--------------+------------------------------------------------------+
         | ``False``    | :class:`~chaos_android.ControlFlowFlatteningOpt`\(``False``) |
         +--------------+------------------------------------------------------+
         | ``None``     | :class:`~chaos_android.ControlFlowFlatteningOpt`\(``False``) |
         +--------------+------------------------------------------------------+

         See the :chaos_android:`control-flow-flattening` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("obfuscate_struct_access", &ObfuscationConfig::obfuscateStructAccess,
           R"delim(
         The default user-callback when obfuscating structures accesses.

         In addition to the associated class options, ChaosProtector Android interprets these return values as follows:

         +--------------+---------------------------------------------+
         | Return Value | Interpretation                              |
         +==============+=============================================+
         | ``True``     | :class:`~chaos_android.StructAccessOpt`\(``True``)  |
         +--------------+---------------------------------------------+
         | ``False``    | :class:`~chaos_android.StructAccessOpt`\(``False``) |
         +--------------+---------------------------------------------+
         | ``None``     | :class:`~chaos_android.StructAccessOpt`\(``False``) |
         +--------------+---------------------------------------------+

         See the :chaos_android:`opaque-fields-access` documentation.
         )delim",
           "module"_a, "function"_a, "struct"_a)

      .def("obfuscate_variable_access",
           &ObfuscationConfig::obfuscateVariableAccess,
           R"delim(
         The default user-callback when obfuscating global variables access.

         In addition to the associated class options, ChaosProtector Android interprets these return values as follows:

         +--------------+------------------------------------------+
         | Return Value | Interpretation                           |
         +==============+==========================================+
         | ``True``     | :class:`~chaos_android.VarAccessOpt`\(``True``)  |
         +--------------+------------------------------------------+
         | ``False``    | :class:`~chaos_android.VarAccessOpt`\(``False``) |
         +--------------+------------------------------------------+
         | ``None``     | :class:`~chaos_android.VarAccessOpt`\(``False``) |
         +--------------+------------------------------------------+

         See the :chaos_android:`opaque-fields-access` documentation.
         )delim",
           "module"_a, "function"_a, "variable"_a)

      .def("obfuscate_constants", &ObfuscationConfig::obfuscateConstants,
           R"delim(
         The default user-callback to obfuscate constants.

         In addition to the associated class options, ChaosProtector Android interprets these return values as follows:

         +-------------------+--------------------------------------------------------+
         | Return Value      | Interpretation                                         |
         +===================+========================================================+
         | ``True``          | :class:`~chaos_android.OpaqueConstantsBool`\(``True``)         |
         +-------------------+--------------------------------------------------------+
         | ``False``         | :class:`~chaos_android.OpaqueConstantsBool`\(``False``)        |
         +-------------------+--------------------------------------------------------+
         | ``None``          | :class:`~chaos_android.OpaqueConstantsBool`\(``False``)        |
         +-------------------+--------------------------------------------------------+
         | ``list(int ...)`` | :class:`~chaos_android.omvll.OpaqueConstantsSet`\(``int ...``) |
         +-------------------+--------------------------------------------------------+

         See the :chaos_android:`opaque-constants` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("obfuscate_arithmetic", &ObfuscationConfig::obfuscateArithmetics,
           R"delim(
         The default user-callback when obfuscating arithmetic operations.

         In addition to the associated class options, ChaosProtector Android interprets these return values as follows:

         +--------------+-------------------------------------------+
         | Return Value | Interpretation                            |
         +==============+===========================================+
         | ``True``     | :class:`~chaos_android.ArithmeticOpt`\(``True``)  |
         +--------------+-------------------------------------------+
         | ``False``    | :class:`~chaos_android.ArithmeticOpt`\(``False``) |
         +--------------+-------------------------------------------+
         | ``None``     | :class:`~chaos_android.ArithmeticOpt`\(``False``) |
         +--------------+-------------------------------------------+

         See the :chaos_android:`arithmetic` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("anti_hooking", &ObfuscationConfig::antiHooking,
           R"delim(
         The default user-callback to enable hooking protection.

         In addition to the associated class options, ChaosProtector Android interprets these return values as follows:

         +--------------+-----------------------------------------+
         | Return Value | Interpretation                          |
         +==============+=========================================+
         | ``True``     | :class:`~chaos_android.AntiHookOpt`\(``True``)  |
         +--------------+-----------------------------------------+
         | ``False``    | :class:`~chaos_android.AntiHookOpt`\(``False``) |
         +--------------+-----------------------------------------+
         | ``None``     | :class:`~chaos_android.AntiHookOpt`\(``False``) |
         +--------------+-----------------------------------------+

         See the :chaos_android:`anti-hook` documentation.
         )delim",
           "module"_a, "function"_a)

      .def("indirect_branch", &ObfuscationConfig::indirectBranch,
           R"delim(
         The default user-callback to enable control-flow edges replacement of
         ordinary branches into indirect jumps.
         )delim",
           "module"_a, "function"_a)

      .def("indirect_call", &ObfuscationConfig::indirectCall,
           R"delim(
         The default user-callback to convert direct function calls into indirect
         ones, by splitting the target address into two additive shares.
         )delim",
           "module"_a, "function"_a)

      .def("basic_block_duplicate", &ObfuscationConfig::basicBlockDuplicate,
           R"delim(
           The default user-callback to randomly select basic blocks to be
           duplicated in a function.
         )delim",
           "module"_a, "function"_a)

      .def("function_outline", &ObfuscationConfig::functionOutline,
           R"delim(
           The default user-callback to randomly select basic blocks to be
           outlined each into a new function.
         )delim",
           "module"_a, "function"_a)

      .def("report_diff", &ObfuscationConfig::reportDiff,
           R"delim(
         User-callback to monitor IR-level changes from individual obfuscation passes.
         )delim",
           "pass_name"_a, "original"_a, "obfuscated"_a)

      .def("default_config", &ObfuscationConfig::defaultConfig, "module"_a,
           "function"_a, "ModuleExcludes"_a, "FunctionExcludes"_a,
           "FunctionIncludes"_a, "Probability"_a);
}

std::unique_ptr<py::module_> initChaosAndroidCore(py::dict Modules) {
  auto M = std::make_unique<py::module_>(
      py::module_::create_extension_module("chaos_android", "", new PyModuleDef()));
  ChaosAndroidCtor(*M);
  Modules["chaos_android"] = *M;
  return M;
}

PyConfig::~PyConfig() = default;

PyConfig &PyConfig::instance() {
  static PyConfig Instance;
  return Instance;
}

ObfuscationConfig *PyConfig::getUserConfig() {
  // If YAML-only mode set an external config, use it directly
  if (ExternalConfig)
    return ExternalConfig;

  try {
    py::gil_scoped_acquire gil;
    if (!py::hasattr(*Mod, "chaos_get_config"))
      fatalError("Missing chaos_get_config");

    auto PyUserConfig = Mod->attr("chaos_get_config");
    if (PyUserConfig.is_none())
      fatalError("Missing chaos_get_config");

    py::object Result = PyUserConfig();
    return Result.cast<ObfuscationConfig *>();
  } catch (const std::exception &Exc) {
    fatalError(Exc.what());
  }
}

PyConfig::PyConfig() {
  // In YAML-only mode, skip Python initialization entirely
  if (ExternalConfig) {
    SINFO("YAML-only mode: skipping Python initialization");
    return;
  }

  py::initialize_interpreter();
  // initialize_interpreter() already holds the GIL.

  py::module_ SysMod = py::module_::import("sys");
  py::module_ PathLib = py::module_::import("pathlib");
  py::dict Modules = SysMod.attr("modules");

  CoreMod = initChaosAndroidCore(Modules);

  llvm::StringRef ConfigPath;
  if (!PyConfig::YConfig.ConfigPath.empty())
    ConfigPath = PyConfig::YConfig.ConfigPath;
  else if (char *Config = getenv(EnvKey))
    ConfigPath = Config;

  SINFO("Using CHAOS_ANDROID_CONFIG = {}", ConfigPath);

  std::string ModName = DefaultFileName;
  if (!ConfigPath.empty()) {
    std::string Config = ConfigPath.str();
    auto PyPath = PathLib.attr("Path")(Config);
    py::list Path = SysMod.attr("path");
    Path.insert(0, PyPath.attr("parent").attr("as_posix")());
    std::string Name = PyPath.attr("stem").cast<std::string>();
    ModName = Name;
  }

  try {
    Mod = std::make_unique<py::module_>(py::module_::import(ModName.c_str()));
    ModulePath = Mod->attr("__file__").cast<std::string>();
  } catch (const std::exception &Exc) {
    fatalError(Exc.what());
  }

  // Check if configured output folder variable is not empty in order to create
  // the parents
  if (!Config.OutputFolder.empty())
    if (std::error_code EC =
            llvm::sys::fs::create_directories(Config.OutputFolder))
      fatalError("Failed to create output_folder " + Config.OutputFolder +
                 ": " + EC.message());

  // We have not manually acquired the GIL, so release it now. Subsequent
  // accesses to Python configs will manually require acquiring the GIL.
  PyEval_SaveThread();
}

std::string PyConfig::configPath() { return ModulePath; }

} // end namespace chaos_android

PYBIND11_MODULE(chaos_android, m) { chaos_android::ChaosAndroidCtor(m); }
