#include "Config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <windows.h>

static std::wstring ExpandEnv(const std::wstring& s) {
    wchar_t buf[4096];
    DWORD n = ExpandEnvironmentStringsW(s.c_str(), buf, 4096);
    if (n == 0 || n > 4096) return s;
    return std::wstring(buf);
}

Config Config::Load(const std::wstring& inPath) {
    auto p = std::filesystem::path(ExpandEnv(inPath));
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot open config file");

    nlohmann::json j;
    f >> j;

    Config c;
    if (j.contains("AETitle"))     c.aeTitle = j["AETitle"].get<std::string>();
    if (j.contains("Port"))        c.port = j["Port"].get<unsigned short>();
    if (j.contains("StoragePath")) {
        const std::string s = j["StoragePath"].get<std::string>();
        c.storagePath = std::filesystem::path(std::wstring(s.begin(), s.end()));
    }
    if (j.contains("PostgresUri")) c.pgUri = j["PostgresUri"].get<std::string>();
    if (j.contains("MaxPdu"))      c.maxPdu = j["MaxPdu"].get<unsigned int>();
    if (j.contains("AcceptedAEs")) c.acceptedAEs = j["AcceptedAEs"].get<std::vector<std::string>>();
    return c;
}
