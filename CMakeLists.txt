cmake_minimum_required(VERSION 3.22)

project(PacsCpp
  VERSION 0.1.0
  LANGUAGES CXX)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: компиляция в UTF-8 (исключает ошибки кодировки)
if (MSVC)
  add_compile_options(/utf-8)
endif()

# vcpkg toolchain укажите в CMake Settings VS
# или через -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
# и -DVCPKG_TARGET_TRIPLET=x64-windows

# Пакеты из vcpkg
find_package(DCMTK REQUIRED dcmnet dcmdata)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# libpqxx (ищем вручную — как в нашем коде)
find_path(PQXX_INCLUDE_DIR pqxx/pqxx)
find_library(PQXX_LIB pqxx)

if (NOT PQXX_INCLUDE_DIR OR NOT PQXX_LIB)
  message(FATAL_ERROR "libpqxx не найден. Убедитесь, что установлен libpqxx:x64-windows в vcpkg.")
endif()

# Сабпроекты
add_subdirectory(src/core)
add_subdirectory(src/service)
add_subdirectory(src/tray)
