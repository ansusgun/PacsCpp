add_executable(pacs_tray WIN32
  WinMain.cpp
)

target_link_libraries(pacs_tray
  PRIVATE
    user32
    gdi32
    advapi32
    comctl32
    shell32
)

target_compile_definitions(pacs_tray
  PRIVATE
    UNICODE
    _UNICODE
)

if (MSVC)
  target_compile_options(pacs_tray PRIVATE /utf-8)
endif()
