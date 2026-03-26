cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# ChaosProtector Android Versioning from Git (if available)
if(EXISTS       "${CMAKE_CURRENT_SOURCE_DIR}/../.git" AND
   IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.git")
  find_package(Git REQUIRED)
  include(GitInfo)
else()
  set(ChaosAndroid_VERSION_MAJOR "2")
  set(ChaosAndroid_VERSION_MINOR "0")
  set(ChaosAndroid_VERSION_PATCH "0")
endif()

project(ChaosAndroid
        VERSION ${ChaosAndroid_VERSION_MAJOR}.${ChaosAndroid_VERSION_MINOR}.${ChaosAndroid_VERSION_PATCH})
message(STATUS "${PROJECT_NAME}: ${PROJECT_VERSION}")

option(ChaosAndroid_FORCE_LOG_DEBUG "Force the log debug output" OFF)
option(ChaosAndroid_PY_STANDALONE   "Build ChaosAndroid as a standalone Python module" OFF)

if(APPLE)
  set(ChaosAndroid_DEFAULT_ABI "Apple")
else()
  set(ChaosAndroid_DEFAULT_ABI "Android")
endif()

set(ChaosAndroid_ABI "${ChaosAndroid_DEFAULT_ABI}" CACHE STRING
  "Compiler ABI to build ChaosAndroid against. Supported options are \"Apple\" and \"Android\". Defaults to ${ChaosAndroid_DEFAULT_ABI}.")

if(ChaosAndroid_ABI STREQUAL "Apple")
  set(LLVM_REQUIRED_VERSION 19.1.4)
  set(LLVM_EXACT_MATCH TRUE)
elseif(ChaosAndroid_ABI STREQUAL "Android")
  set(LLVM_REQUIRED_VERSION 17)
  set(LLVM_EXACT_MATCH FALSE)
else()
  # Linux host build: accept any LLVM 17.x
  set(LLVM_REQUIRED_VERSION 17)
  set(LLVM_EXACT_MATCH FALSE)
endif()

# Try configured path first, then system
find_package(LLVM ${LLVM_REQUIRED_VERSION} CONFIG)
if(NOT LLVM_FOUND)
  find_package(LLVM ${LLVM_REQUIRED_VERSION} REQUIRED CONFIG)
endif()

set(PYTHON_VERSION "3" CACHE STRING "Python version")
find_package(Python3 COMPONENTS Development Interpreter)

find_package(spdlog REQUIRED CONFIG)
# Try configured path first, then system
find_package(pybind11 CONFIG)
if(NOT pybind11_FOUND)
  find_package(pybind11 REQUIRED CONFIG)
endif()

message(STATUS "Python version: ${Python3_VERSION}")
message(STATUS "Python lib:     ${Python3_LIBRARIES}")
message(STATUS "Python include: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python interpreter: ${Python3_EXECUTABLE}")

if(LLVM_EXACT_MATCH AND NOT "${LLVM_PACKAGE_VERSION}" VERSION_EQUAL "${LLVM_REQUIRED_VERSION}")
  message(FATAL_ERROR "Found LLVM ${LLVM_PACKAGE_VERSION}, but need LLVM ${LLVM_REQUIRED_VERSION}")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

message("LLVM STATUS:
  Definitions ${LLVM_DEFINITIONS}
  Includes    ${LLVM_INCLUDE_DIRS}
  Libraries   ${LLVM_LIBRARY_DIRS}
  Targets     ${LLVM_TARGETS_TO_BUILD}"
)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# Build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE
      STRING "Build type (default RelWithDebInfo):" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fdiagnostics-color=always")

set(ChaosAndroid_DEBUG 0)
if(CMAKE_BUILD_TYPE MATCHES Release)
  if(ChaosAndroid_FORCE_LOG_DEBUG)
    set(ChaosAndroid_DEBUG 1)
  endif()
else()
  set(ChaosAndroid_DEBUG 1)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/chaos_android/config.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/chaos_android/config.hpp")

add_library(ChaosAndroid SHARED)

set_target_properties(ChaosAndroid
  PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
target_include_directories(ChaosAndroid
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
  SYSTEM
    ${Python3_INCLUDE_DIRS}
)

target_compile_options(ChaosAndroid PUBLIC
  -fdata-sections
  -ffunction-sections
  -fvisibility-inlines-hidden -fvisibility=hidden
)

target_compile_options(ChaosAndroid PUBLIC
  -fno-rtti
)

target_compile_definitions(ChaosAndroid
  PRIVATE
    SPDLOG_DISABLE_DEFAULT_LOGGER
    SPDLOG_FUNCTION=
    SPDLOG_NO_EXCEPTIONS
    SPDLOG_NO_THREAD_ID
)

set(ChaosAndroid_LINK_OPT
  #-fuse-ld=gold
)

if(NOT APPLE)
  list(APPEND ChaosAndroid_LINK_OPT
    -Wl,-as-needed
    -Wl,--gc-sections
    -Wl,--exclude-libs,ALL
    -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports_linux.map
    -fuse-ld=gold
  )
endif()

if(APPLE)
  list(APPEND ChaosAndroid_LINK_OPT
    -Wl,-flat_namespace
    -Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/exports.txt
  )
endif()

target_link_options(ChaosAndroid PUBLIC ${ChaosAndroid_LINK_OPT})

target_link_libraries(ChaosAndroid
  PRIVATE
    spdlog::spdlog pybind11::headers
    ${Python3_LIBRARIES}
)

if(NOT APPLE)
  target_link_libraries(ChaosAndroid
    PRIVATE
    -static-libgcc
  )
endif()

set(LLVM_LIBS_DEP
  demangle
  OrcJIT
  native
)

# Used for jitAsm in Jitter. Remove when we switch to calling the host compiler.
# See discussion: https://github.com/open-obfuscator/o-mvll/pull/78#pullrequestreview-2780108790
list(APPEND LLVM_LIBS_DEP
    ARMCodeGen
    ARMAsmParser
    AArch64CodeGen
    AArch64AsmParser
    X86CodeGen
    X86AsmParser
)

if(APPLE)
  list(APPEND
    LLVM_LIBS_DEP
    Option
    MCJIT
  )
endif()

if (ChaosAndroid_PY_STANDALONE)
  find_package(Clang REQUIRED CONFIG NO_DEFAULT_PATH)
  include_directories(SYSTEM ${CLANG_INCLUDE_DIRS})
  set_target_properties(ChaosAndroid PROPERTIES PREFIX "" OUTPUT_NAME "chaos_android" CLEAN_DIRECT_OUTPUT 1)
  list(APPEND LLVM_LIBS_DEP
    core
    support
    TransformUtils
    Passes
    Option
    MCJIT
  )
  target_link_libraries(ChaosAndroid PRIVATE
    clang
    clangBasic
    clangFrontend
    clangInterpreter
  )
endif()

llvm_map_components_to_libnames(llvm_libs
  ${LLVM_LIBS_DEP}
)

# TODO: double-check with NDK on Linux
if(ChaosAndroid_ABI STREQUAL "Android")
  target_link_libraries(ChaosAndroid PRIVATE LLVM)
else()
  target_link_libraries(ChaosAndroid PRIVATE
    ${llvm_libs}
  )
endif()

if(CMAKE_BUILD_TYPE MATCHES Release AND NOT ChaosAndroid_FORCE_LOG_DEBUG)
  if(APPLE)
  add_custom_command(
    TARGET ChaosAndroid
    COMMENT "Strip ChaosProtector Android"
    POST_BUILD
    COMMAND ${CMAKE_STRIP} -x -S $<TARGET_FILE:ChaosAndroid>
  )
  endif()
endif()

add_subdirectory("core")
add_subdirectory("passes")
add_subdirectory("test")
