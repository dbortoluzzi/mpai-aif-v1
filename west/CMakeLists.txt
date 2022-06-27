include(CMakePrintHelpers)

message("${ZEPHYR_BASE}")

# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr)
project(app)

FILE(GLOB app_sources ../src/*.c)
FILE(GLOB app_sources_mpai_core ../lib/mpai_core/*.c)
FILE(GLOB app_sources_mpai_libs ../lib/mpai_libs/*.c)
FILE(GLOB app_sources_mpai_stm32 ../lib/stm32_libs/*.c)
FILE(GLOB app_sources_mpai_util ../lib/util_libs/*.c)
FILE(GLOB app_sources_cjson_libs ../lib/cJSON/cJSON*.c)
FILE(GLOB app_sources_pubsub_libs ../lib/pubsub/src/*.c)

message("C++ compiler flags: ${CMAKE_CXX_FLAGS}")

target_compile_options(app
  PRIVATE
    ${flags}
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPUBSUB_MAX_CHANNELS=10")

target_sources(app PRIVATE
  ${app_sources}
  ${app_sources_mpai_core}
  ${app_sources_mpai_libs}
  ${app_sources_mpai_stm32}
  ${app_sources_mpai_util}
  ${app_sources_cjson_libs}
  ${app_sources_pubsub_libs}  
  )

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip ../lib/mpai_core ../lib/mpai_libs ../lib/stm32_libs ../lib/util_libs ../lib/cJSON ../lib/pubsub/include)


