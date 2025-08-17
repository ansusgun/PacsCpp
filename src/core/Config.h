#pragma once
#include <string>

struct Config {
    std::wstring storageRoot;  // куда складывать данные/логи
    std::string  dbConn;       // строка подключения к Postgres; пустая = БД нет

    // Загрузить конфиг из стандартного места: %ProgramData%\PacsCpp\config.json
    static Config Load();

    // Явная загрузка из указанного пути
    static Config Load(const std::wstring& path);

    // Удобные геттеры (под ожидания вашего кода)
    std::wstring StorageRootW() const { return storageRoot; }
    std::string  DbConnString() const { return dbConn; }
};
