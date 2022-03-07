# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
#include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensortile_box)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/include/portability)

# it doesn't work compiling zephyr_final (multiple definitions of "arm_mult_f32")
# target_sources(app INTERFACE $ENV{ZEPHYR_BASE}/_pio/modules/hal/cmsis/CMSIS/DSP/Source/BasicMathFunctions/arm_mult_f32.c)

target_sources(app PRIVATE ../src/main.c)
