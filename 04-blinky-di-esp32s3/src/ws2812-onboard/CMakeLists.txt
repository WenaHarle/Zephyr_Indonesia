cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky_ws2812_onboard)

target_sources(app PRIVATE main.c)
