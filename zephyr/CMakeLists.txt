# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
#include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iotnode_test_sensors)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/include/portability)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/net/ip)

target_sources(app PRIVATE ../src/main.c)
