#include "PgIndex.h"
#include <spdlog/spdlog.h>

PgIndex::PgIndex(const std::string& conn_str) {
    try {
        if (!conn_str.empty()) {
            conn_ = std::make_unique<pqxx::connection>(conn_str);
            if (!conn_->is_open()) {
                spdlog::warn("DB not open after connect string given.");
                conn_.reset();
            }
        }
        else {
            spdlog::warn("DB connection string is empty. Database features are disabled.");
        }
    }
    catch (const std::exception& e) {
        spdlog::error("DB connect failed: {}", e.what());
        conn_.reset();
    }
}

PgIndex::~PgIndex() = default;

bool PgIndex::IsConnected() const noexcept {
    return conn_ && conn_->is_open();
}

// Простейшие реализации: если БД нет — пишем в лог и возвращаем заглушки.
long long PgIndex::UpsertResource(const std::string& type,
    const std::string& uid) {
    if (!IsConnected()) {
        spdlog::warn("UpsertResource skipped (no DB). type='{}', uid='{}'", type, uid);
        return -1;
    }
    try {
        pqxx::work tx{ *conn_ };
        // Таблицы и SQL — как договоримся. Пока пример.
        auto r = tx.exec_params1(
            "insert into resource(type, uid) values($1,$2) "
            "on conflict (uid) do update set type=excluded.type "
            "returning id",
            type, uid);
        tx.commit();
        return r[0].as<long long>();
    }
    catch (const std::exception& e) {
        spdlog::error("UpsertResource error: {}", e.what());
        return -1;
    }
}

void PgIndex::UpsertIdentifier(long long rid,
    const std::string& system,
    const std::string& value) {
    if (!IsConnected()) {
        spdlog::warn("UpsertIdentifier skipped (no DB). rid={}, sys='{}'", rid, system);
        return;
    }
    try {
        pqxx::work tx{ *conn_ };
        tx.exec_params(
            "insert into identifier(resource_id, system, value) "
            "values($1,$2,$3) "
            "on conflict (resource_id, system) do update set value=excluded.value",
            rid, system, value);
        tx.commit();
    }
    catch (const std::exception& e) {
        spdlog::error("UpsertIdentifier error: {}", e.what());
    }
}

bool PgIndex::AttachDicom(long long instanceRid,
    const std::string& pathOnDisk) {
    if (!IsConnected()) {
        spdlog::warn("AttachDicom skipped (no DB). rid={}, path={}", instanceRid, pathOnDisk);
        return false;
    }
    try {
        pqxx::work tx{ *conn_ };
        tx.exec_params(
            "insert into dicom_file(resource_id, path) values($1,$2)",
            instanceRid, pathOnDisk);
        tx.commit();
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("AttachDicom error: {}", e.what());
        return false;
    }
}
