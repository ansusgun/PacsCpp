# Windows-сервис
add_executable(pacs_service
  ServiceMain.cpp
)

# Не делаем WIN32_EXECUTABLE, чтобы был wmain, а не WinMain
target_link_libraries(pacs_service
  PRIVATE
    pacs_core
    spdlog::spdlog
    libpqxx::pqxx         # <-- было pqxx::pqxx
    Advapi32
)

if (MSVC)
  target_compile_definitions(pacs_service PRIVATE UNICODE _UNICODE)
  target_compile_options(pacs_service PRIVATE /utf-8)
endif()
