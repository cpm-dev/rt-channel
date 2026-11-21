cmake_minimum_required(VERSION 3.20)
project(rt_channel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# Sanitizer builds: -DSANITIZE=thread, address, undefined, or empty
set(SANITIZE "" CACHE STRING "Sanitizer: thread, address, undefined, or empty")

add_library(rtchan INTERFACE)
target_include_directories(rtchan INTERFACE include)
target_compile_options(
  rtchan
  INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Werror
)
if(SANITIZE STREQUAL "address" OR SANITIZE STREQUAL "thread" OR SANITIZE STREQUAL "undefined")
  target_compile_options(rtchan INTERFACE -fsanitize=${SANITIZE} -fno-omit-frame-pointer -g)
  target_link_options(rtchan INTERFACE -fsanitize=${SANITIZE})
elseif(NOT SANITIZE STREQUAL "")
  message(FATAL_ERROR "Unsupported SANITIZE='${SANITIZE}'; use address, undefined, thread, or empty")
endif()

find_package(Threads REQUIRED)

add_executable(producer src/producer.cpp)
add_executable(monitor src/monitor.cpp)
target_link_libraries(producer PRIVATE rtchan Threads::Threads)
target_link_libraries(monitor PRIVATE rtchan Threads::Threads)
if(UNIX AND NOT APPLE)
  target_link_libraries(producer PRIVATE rt)
  target_link_libraries(monitor PRIVATE rt)
endif()

add_executable(test_spsc_ring tests/test_spsc_ring.cpp)
target_link_libraries(test_spsc_ring PRIVATE rtchan Threads::Threads)
add_test(NAME spsc_ring COMMAND test_spsc_ring)
