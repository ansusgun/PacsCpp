
# Ѕиблиотека €дра PACS
add_library(pacs_core STATIC
  Config.cpp
  Config.h
  PgIndex.cpp
  PgIndex.h
  PacsServer.cpp
  PacsServer.h
)

target_include_directories(pacs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PQXX_INCLUDE_DIR})

# ѕробрасываем include директорию наружу,
# чтобы заголовки core были видны в зависимых таргетах
target_include_directories(pacs_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PQXX_INCLUDE_DIR}
)

# Ћинки
target_link_libraries(pacs_core
  PRIVATE
    DCMTK::dcmnet
    DCMTK::dcmdata
    ${PQXX_LIB}
    PostgreSQL::PostgreSQL
    nlohmann_json::nlohmann_json
)

# MSVC: тоже UTF-8 (на вс€кий случай)
if (MSVC)
  target_compile_options(pacs_core PRIVATE /utf-8)
endif()
