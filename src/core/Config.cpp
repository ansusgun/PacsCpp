#include "Config.h"

#include <windows.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using nlohmann::json;
namespace fs = std::filesystem;

static std::wstring GetProgramData() {
    wchar_t buf[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"ProgramData", buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return L"C:\\ProgramData";
    return std::wstring(buf);
}

static std::wstring DefaultConfigPath() {
    auto base = GetProgramData() + L"\\PacsCpp";
    fs::create_directories(base);
    return base + L"\\config.json";
}

Config Config::Load() {
    return Load(DefaultConfigPath());
}

Config Config::Load(const std::wstring& path) {
    Config cfg;
    // Значения по умолчанию
    cfg.storageRoot = GetProgramData() + L"\\PacsCpp\\storage";
    cfg.dbConn = ""; // пустая строка -> БД не используем

    try {
        fs::create_directories(cfg.storageRoot);

        // Пытаемся прочитать JSON, если файл существует
        std::ifstream f(fs::path(path).string(), std::ios::binary);
        if (f.good()) {
            json j; f >> j;
            if (j.contains("storage_root")) {
                auto sr = j["storage_root"].get<std::string>();
                cfg.storageRoot.assign(sr.begin(), sr.end()); // простая конверсия
                fs::create_directories(cfg.storageRoot);
            }
            if (j.contains("db_conn")) {
                cfg.dbConn = j["db_conn"].get<std::string>();
            }
        }
    }
    catch (...) {
        // игнорируем: оставим значения по умолчанию
    }
    return cfg;
}
