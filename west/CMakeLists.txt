#include(CMakePrintHelpers)
#message("${ZEPHYR_BASE}")

# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.13.1)

# configuration file shared with platformio
set(CONF_FILE "../zephyr/prj.conf")
# devicetree overlay shared with platformio
set(DTC_OVERLAY_FILE "../zephyr/app.overlay")
# custom kernel configuration options shared with platformio
get_filename_component(KCONFIG_ROOT "../zephyr/Kconfig" ABSOLUTE)
#set(KCONFIG_ROOT "../zephyr")

find_package(Zephyr)
project(app)

FILE(GLOB app_sources ../src/*.c)
FILE(GLOB app_sources_mpai_core ../lib/mpai_core/*.c)
FILE(GLOB app_sources_mpai_libs ../lib/mpai_libs/*.c)
FILE(GLOB app_sources_mpai_stm32 ../lib/stm32_libs/*.c)
FILE(GLOB app_sources_mpai_util ../lib/util_libs/*.c)
FILE(GLOB app_sources_cjson_libs ../lib/cJSON/cJSON*.c)

message("C++ compiler flags: ${CMAKE_CXX_FLAGS}")

target_compile_options(app
  PRIVATE
    ${flags}
    )

zephyr_compile_definitions(
  PUBSUB_MAX_CHANNELS=10
)

target_sources(app PRIVATE
  ${app_sources}
  ${app_sources_mpai_core}
  ${app_sources_mpai_libs}
  ${app_sources_mpai_stm32}
  ${app_sources_mpai_util}
  ${app_sources_cjson_libs}
  )

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip ../lib/mpai_core ../lib/mpai_libs ../lib/stm32_libs ../lib/util_libs ../lib/cJSON)



