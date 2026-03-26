if(__add_gitinfo)
  return()
endif()
set(__add_gitinfo ON)

execute_process(
  COMMAND           ${GIT_EXECUTABLE} log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE   ChaosAndroid_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND           ${GIT_EXECUTABLE} rev-list --count ${ChaosAndroid_COMMIT_HASH}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE   ChaosAndroid_COMMIT_COUNT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND           ${GIT_EXECUTABLE} describe --abbrev=0 --tags HEAD --always
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE   ChaosAndroid_GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND           ${GIT_EXECUTABLE} tag --list --points-at=HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE   ChaosAndroid_GIT_COMMIT_TAGGED
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND           ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE   ChaosAndroid_GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(COMPARE NOTEQUAL "${ChaosAndroid_GIT_COMMIT_TAGGED}" "" ChaosAndroid_IS_TAGGED)
string(REGEX MATCHALL "([0-9]+)" VERSION_STRING "${ChaosAndroid_GIT_TAG}")

message(STATUS "Tagged: ${ChaosAndroid_IS_TAGGED}")
if (${ChaosAndroid_IS_TAGGED})
  message(STATUS "Tag: ${ChaosAndroid_GIT_TAG}")
else()
  if(ChaosAndroid_GIT_BRANCH MATCHES "^release-")
    string(REGEX MATCHALL "([0-9]+)" VERSION_STRING "${ChaosAndroid_GIT_BRANCH}")
    message(STATUS "${VERSION_STRING}")
  endif()
endif()
message(STATUS "Current branch: ${ChaosAndroid_GIT_BRANCH}")
message(STATUS "Current commit: ${ChaosAndroid_COMMIT_HASH}")


if (VERSION_STRING)
  list(GET VERSION_STRING 0 ChaosAndroid_VERSION_MAJOR)
  list(GET VERSION_STRING 1 ChaosAndroid_VERSION_MINOR)
  list(GET VERSION_STRING 2 ChaosAndroid_VERSION_PATCH)

  if (NOT ${ChaosAndroid_IS_TAGGED})
    if(ChaosAndroid_GIT_BRANCH MATCHES "^release-")
      message(STATUS "Release branch")
    else()
      MATH(EXPR ChaosAndroid_VERSION_MINOR "${ChaosAndroid_VERSION_MINOR}+1")
      set(ChaosAndroid_VERSION_PATCH 0)
    endif()
  endif()
else()
  set(ChaosAndroid_VERSION_MAJOR 0)
  set(ChaosAndroid_VERSION_MINOR 0)
  set(ChaosAndroid_VERSION_PATCH 0)
endif()

