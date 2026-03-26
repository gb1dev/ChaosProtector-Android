target_sources(ChaosAndroid PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/chaos_config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/jitter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/YamlObfuscationConfig.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Licensing.cpp
)

add_subdirectory("python")
