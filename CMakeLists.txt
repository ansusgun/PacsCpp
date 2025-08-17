cmake_minimum_required(VERSION 3.26)
project(PacsCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
  add_compile_options(/utf-8 /Zc:__cplusplus)
endif()

# Пакеты из vcpkg
find_package(spdlog CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)   # <-- было pqxx, должно быть libpqxx

add_subdirectory(src/core)
add_subdirectory(src/service)
add_subdirectory(src/tray)
