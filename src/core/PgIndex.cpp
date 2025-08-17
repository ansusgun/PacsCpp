#include "PgIndex.h"
#include <algorithm>

static std::string ToUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return (char)std::toupper(c); });
    return s;
}

PgIndex::PgIndex(const std::string& uri) : conn_(uri) {}

long PgIndex::UpsertResource(int type, std::optional<long> parent, const std::string& publicId) {
    pqxx::work tx(conn_);
    long id;
    if (parent) {
        // параметризованный запрос с возвращаемой строкой
        auto row = tx.exec_params1(
            "INSERT INTO Resources(resource_type, parent_id, public_id) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT (public_id) DO UPDATE SET resource_type = EXCLUDED.resource_type "
            "RETURNING internal_id",
            type, *parent, publicId
        );
        id = row[0].as<long>();
    }
    else {
        auto row = tx.exec_params1(
            "INSERT INTO Resources(resource_type, parent_id, public_id) "
            "VALUES ($1, NULL, $2) "
            "ON CONFLICT (public_id) DO UPDATE SET resource_type = EXCLUDED.resource_type "
            "RETURNING internal_id",
            type, publicId
        );
        id = row[0].as<long>();
    }
    tx.commit();
    return id;
}


void PgIndex::UpsertIdentifier(long rid, int g, int e, const std::string& valUpper) {
    pqxx::work tx(conn_);
    tx.exec_params(
        "INSERT INTO DicomIdentifiers(id, tag_group, tag_element, value) "
        "VALUES ($1,$2,$3,$4) "
        "ON CONFLICT (id, tag_group, tag_element) DO UPDATE SET value = EXCLUDED.value",
        rid, g, e, ToUpper(valUpper)
    );
    tx.commit();
}

void PgIndex::UpsertMainTag(long rid, int g, int e, const std::string& val) {
    pqxx::work tx(conn_);
    tx.exec_params(
        "INSERT INTO MainDicomTags(id, tag_group, tag_element, value) "
        "VALUES ($1,$2,$3,$4) "
        "ON CONFLICT (id, tag_group, tag_element) DO UPDATE SET value = EXCLUDED.value",
        rid, g, e, val
    );
    tx.commit();
}

void PgIndex::AttachDicom(long rid, const std::string& uuid, std::uint64_t size) {
    pqxx::work tx(conn_);
    tx.exec_params(
        "INSERT INTO AttachedFiles(id, file_type, uuid, compressed_size, uncompressed_size) "
        "VALUES ($1, 0, $2, $3, $3) "
        "ON CONFLICT (id, file_type) DO UPDATE SET uuid = EXCLUDED.uuid, "
        "compressed_size = EXCLUDED.compressed_size, uncompressed_size = EXCLUDED.uncompressed_size",
        rid, uuid, (long long)size
    );
    tx.commit();
}
