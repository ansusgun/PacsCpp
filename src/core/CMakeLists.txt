# Библиотека ядра PACS. Без libpqxx (т.к. PgIndex пока заглушка)
add_library(pacs_core STATIC
  Config.cpp
  Config.h
  PacsServer.cpp
  PacsServer.h
  PgIndex.h      # только заголовок-заглушка, .cpp не компилируем
 "PgIndex.cpp")

target_include_directories(pacs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(pacs_core
  PRIVATE
    spdlog::spdlog
)

if (MSVC)
  target_compile_options(pacs_core PRIVATE /utf-8)
endif()
