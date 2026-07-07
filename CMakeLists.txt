cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(MotorDriver)

target_sources(app PRIVATE
  src/main.c
  drv2625/src/drv2625.c
)

target_include_directories(app PRIVATE
  drv2625/include
)