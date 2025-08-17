# Windows service
add_executable(pacs_service WIN32
  ServiceMain.cpp
)


target_include_directories(pacs_service PRIVATE ${CMAKE_SOURCE_DIR}/src/core)


# Линки и дефайны
target_link_libraries(pacs_service
  PRIVATE
    pacs_core
    advapi32        # API управления службами
)

# Используем Unicode-версии WinAPI
target_compile_definitions(pacs_service
  PRIVATE
    UNICODE
    _UNICODE
)

# MSVC: UTF-8
if (MSVC)
  target_compile_options(pacs_service PRIVATE /utf-8)
endif()
