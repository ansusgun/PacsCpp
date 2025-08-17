#include "PacsServer.h"

#include <filesystem>
#include <chrono>
#include <thread>
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

PacsServer::PacsServer(const Config& cfg)
    : storageRoot_(cfg.StorageRootW()),
    cfg_(cfg),
    index_(cfg.DbConnString()) // попытаемся подключиться к БД сразу
{
    fs::create_directories(storageRoot_);
}

PacsServer::~PacsServer() {
    Stop();
}

void PacsServer::Start() {
    if (!index_.Connected()) {
        get_logger()->error("PacsServer not started: no DB connection (check config.json: \"db_conn\").");
        return;
    }

    if (worker_.joinable()) return; // уже запущен

    stop_ = false;
    get_logger()->info("PacsServer starting...");
    worker_ = std::thread(&PacsServer::Run, this);
}

void PacsServer::Stop() {
    stop_ = true;
    if (worker_.joinable()) {
        worker_.join();
        get_logger()->info("PacsServer stopped.");
    }
}

bool PacsServer::IsRunning() const {
    return worker_.joinable() && !stop_.load();
}

void PacsServer::Run() {
    get_logger()->info("PacsServer running. Storage: {}", std::string(storageRoot_.begin(), storageRoot_.end()));
    // Заглушка рабочего цикла
    while (!stop_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
