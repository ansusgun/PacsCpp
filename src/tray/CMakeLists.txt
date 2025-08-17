# Простой Win32 GUI (трей)
add_executable(pacs_tray WIN32
  WinMain.cpp
)

# Линки WinAPI
target_link_libraries(pacs_tray
  PRIVATE
    user32
    gdi32
    advapi32    # управление сервисом
    comctl32
    shell32     # ShellExecuteW
)

# Unicode-версии WinAPI
target_compile_definitions(pacs_tray
  PRIVATE
    UNICODE
    _UNICODE
)

# MSVC: UTF-8
if (MSVC)
  target_compile_options(pacs_tray PRIVATE /utf-8)
endif()
