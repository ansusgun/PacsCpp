#include "PgIndex.h"

#include <filesystem>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace fs = std::filesystem;

static std::shared_ptr<spdlog::logger> get_logger() {
    if (auto lg = spdlog::get("pacs")) return lg;
    std::wstring logDir = L"C:\\ProgramData\\PacsCpp\\logs";
    fs::create_directories(logDir);
    auto logPath = fs::path(logDir) / L"pacs_service.log";
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
    auto lg = std::make_shared<spdlog::logger>("pacs", sink);
    spdlog::register_logger(lg);
    lg->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    return lg;
}

PgIndex::~PgIndex() = default;                    // <Ч деструктор определЄн тут, тип pqxx::connection уже полный

PgIndex::PgIndex(const std::string& conn) {
    Open(conn);
}

bool PgIndex::Open(const std::string& conn) {
    try {
        if (conn.empty()) {
            get_logger()->warn("DB connection string is empty. Database features are disabled.");
            conn_.reset();
            return false;
        }
        conn_ = std::make_unique<pqxx::connection>(conn);
        if (!conn_->is_open()) {
            get_logger()->error("DB connection failed: connection is not open.");
            conn_.reset();
            return false;
        }
        get_logger()->info("DB connected: {}", conn_->dbname());
        return true;
    }
    catch (const std::exception& e) {
        get_logger()->error("DB connect exception: {}", e.what());
        conn_.reset();
        return false;
    }
}

bool PgIndex::Connected() const {
    return conn_ && conn_->is_open();
}

PgIndex::Rid PgIndex::UpsertResource(const std::string& kind, const std::string& uid) {
    if (!Connected()) {
        get_logger()->warn("UpsertResource({}, {}) skipped: DB not connected.", kind, uid);
        return 0;
    }
    try {
        // TODO: реальный SQL
        return 0;
    }
    catch (const std::exception& e) {
        get_logger()->error("UpsertResource exception: {}", e.what());
        return 0;
    }
}

void PgIndex::UpsertIdentifier(Rid rid, const std::string& system, const std::string& value) {
    if (!Connected()) {
        get_logger()->warn("UpsertIdentifier(rid={}, {}, {}) skipped: DB not connected.", rid, system, value);
        return;
    }
    try {
        // TODO: реальный SQL
    }
    catch (const std::exception& e) {
        get_logger()->error("UpsertIdentifier exception: {}", e.what());
    }
}

void PgIndex::AttachDicom(Rid rid, const std::wstring& path) {
    if (!Connected()) {
        get_logger()->warn("AttachDicom(rid={}, path=...) skipped: DB not connected.", rid);
        return;
    }
    try {
        // TODO: запись прив€зок
    }
    catch (const std::exception& e) {
        get_logger()->error("AttachDicom exception: {}", e.what());
    }
}
