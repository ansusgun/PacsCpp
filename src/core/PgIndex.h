#pragma once
#include <pqxx/pqxx>
#include <optional>
#include <string>
#include <cstdint>

class PgIndex {
public:
	explicit PgIndex(const std::string& uri);

	long UpsertResource(int type, std::optional<long> parent, const std::string& publicId);
	void UpsertIdentifier(long rid, int g, int e, const std::string& valUpper);
	void UpsertMainTag(long rid, int g, int e, const std::string& val);
	void AttachDicom(long rid, const std::string& uuid, std::uint64_t size);

private:
	pqxx::connection conn_;
};
