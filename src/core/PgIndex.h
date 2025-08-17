#pragma once
#include <string>
#include <memory>
#include <pqxx/pqxx> 

namespace pqxx { class connection; }

class PgIndex {
public:
    using Rid = long long;

    PgIndex() = default;
    explicit PgIndex(const std::string& conn);
    ~PgIndex();                       // <— добавили явный деструктор (см. реализацию в .cpp)

    bool Open(const std::string& conn);
    bool Connected() const;

    // Заглушки под вызовы сервера
    Rid  UpsertResource(const std::string& kind, const std::string& uid);
    void UpsertIdentifier(Rid rid, const std::string& system, const std::string& value);
    void AttachDicom(Rid rid, const std::wstring& path);

private:
    std::unique_ptr<pqxx::connection> conn_;
};
