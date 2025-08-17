#include "PacsServer.h"

#include <dcmtk/dcmnet/scp.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmnet/diutil.h>

#include <random>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <string>

using namespace std;

static string ToUpper(string s) {
    for (auto& c : s) c = (char)toupper((unsigned char)c);
    return s;
}

static string NewUuid() {
    random_device rd; mt19937_64 gen(rd()); uniform_int_distribution<uint64_t> d;
    uint64_t a = d(gen), b = d(gen);
    stringstream ss; ss << hex << setfill('0')
        << setw(8) << (uint32_t)(a >> 32) << "-" << setw(4) << (uint16_t)(a >> 16) << "-" << setw(4) << (uint16_t)(a)
        << "-" << setw(4) << (uint16_t)(b >> 48) << "-" << setw(12) << (uint64_t)(b & 0x0000FFFFFFFFFFFFULL);
    return ss.str();
}

static filesystem::path MakeStoragePath(const filesystem::path& root, const string& uuid) {
    string a = ToUpper(uuid);
    auto dir = root / a.substr(0, 2) / a.substr(2, 2);
    filesystem::create_directories(dir);
    return dir / a;
}

// Minimal SCP that stores dataset and indexes it
class StorageSCP : public DcmSCP {
public:
    StorageSCP(const filesystem::path& root, const string& pgUri)
        : storageRoot_(root), index_(pgUri) {
    }

protected:
    // DCMTK 3.6.9 signature
    virtual OFCondition handleSTORERequest(
        T_DIMSE_C_StoreRQ& reqMessage,
        const T_ASC_PresentationContextID presID,
        DcmDataset*& reqDataset)
    {
        (void)reqMessage; (void)presID;

        // Save to temp file
        const string tmpUuid = NewUuid();
        const filesystem::path tmpPath = storageRoot_ / (L"tmp_" + std::wstring(tmpUuid.begin(), tmpUuid.end()));
        DcmFileFormat ff(reqDataset);
        ff.saveFile(string(tmpPath.string().begin(), tmpPath.string().end()).c_str(), EXS_Unknown);

        // Extract UIDs
        OFString studyUid, seriesUid, sopUid, patientId;
        reqDataset->findAndGetOFString(DCM_StudyInstanceUID, studyUid);
        reqDataset->findAndGetOFString(DCM_SeriesInstanceUID, seriesUid);
        reqDataset->findAndGetOFString(DCM_SOPInstanceUID, sopUid);
        reqDataset->findAndGetOFString(DCM_PatientID, patientId);

        // Upsert hierarchy
        auto patientRid = index_.UpsertResource(1, std::nullopt, "pat-" + ToUpper(patientId.c_str()));
        auto studyRid = index_.UpsertResource(2, patientRid, "stu-" + ToUpper(studyUid.c_str()));
        auto seriesRid = index_.UpsertResource(3, studyRid, "ser-" + ToUpper(seriesUid.c_str()));
        auto instRid = index_.UpsertResource(4, seriesRid, "ins-" + ToUpper(sopUid.c_str()));

        index_.UpsertIdentifier(instRid, 0x0020, 0x000D, studyUid.c_str());
        index_.UpsertIdentifier(instRid, 0x0020, 0x000E, seriesUid.c_str());
        index_.UpsertIdentifier(instRid, 0x0008, 0x0018, sopUid.c_str());
        index_.UpsertIdentifier(instRid, 0x0010, 0x0020, patientId.c_str());

        // Move into final storage path
        string finalUuid = NewUuid();
        auto finalPath = MakeStoragePath(storageRoot_, finalUuid);

        std::error_code ec;
        filesystem::rename(tmpPath, finalPath, ec);
        if (ec) {
            filesystem::copy_file(tmpPath, finalPath, filesystem::copy_options::overwrite_existing, ec);
            filesystem::remove(tmpPath, ec);
        }
        auto size = filesystem::file_size(finalPath);
        index_.AttachDicom(instRid, finalUuid, size);

        return EC_Normal;
    }

private:
    filesystem::path storageRoot_;
    PgIndex index_;
};

PacsServer::PacsServer(const std::wstring& configPath)
    : cfg_(Config::Load(configPath)),
    storageRoot_(cfg_.storagePath) {
}

PacsServer::~PacsServer() { Stop(); }

void PacsServer::Start() {
    if (worker_.joinable()) return;
    stop_ = false;
    worker_ = std::thread(&PacsServer::Run, this);
}

void PacsServer::Stop() {
    stop_ = true;
    if (worker_.joinable()) worker_.join();
}

void PacsServer::Run() {
    while (!stop_) {
        StorageSCP scp(storageRoot_, cfg_.pgUri);
        scp.setAETitle(cfg_.aeTitle.c_str());
        scp.setPort(cfg_.port);
        scp.setDIMSETimeout(60);
        scp.setMaxReceivePDULength(cfg_.maxPdu);

        OFCondition cond = scp.listen();
        if (cond.bad()) {
            // simple backoff
            ::Sleep(1000);
        }
    }
}
