#pragma once
#include <memory>
#include <string>

// нужен полный тип pqxx::connection
#include <pqxx/pqxx>

class PgIndex {
public:
    explicit PgIndex(const std::string& conn_str);
    ~PgIndex();

    bool IsConnected() const noexcept;
    // Алиас для кода, который ожидает Connected()
    bool Connected() const noexcept { return IsConnected(); }

    // Минимальное API, которое дергает PacsServer
    long long UpsertResource(const std::string& type,
        const std::string& uid);

    void UpsertIdentifier(long long rid,
        const std::string& system,
        const std::string& value);

    bool AttachDicom(long long instanceRid,
        const std::string& pathOnDisk);

private:
    std::unique_ptr<pqxx::connection> conn_;
};
