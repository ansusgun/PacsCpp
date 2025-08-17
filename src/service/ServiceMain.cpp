#include <windows.h>
#include <memory>
#include <string>
#include <thread>
#include <filesystem>                  // <— добавили

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "Config.h"
#include "PacsServer.h"

static SERVICE_STATUS        g_status{};
static SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
static std::unique_ptr<PacsServer> g_server;

namespace fs = std::filesystem;        // <— удобный алиас

static std::shared_ptr<spdlog::logger> get_logger() {
    if (auto lg = spdlog::get("pacs")) return lg;
    std::wstring logDir = L"C:\\ProgramData\\PacsCpp\\logs";
    fs::create_directories(logDir);                                        // <— std::filesystem теперь виден
    auto logPath = fs::path(logDir) / L"pacs_service.log";                 // <—
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath.string(), true);
    auto lg = std::make_shared<spdlog::logger>("pacs", sink);
    spdlog::register_logger(lg);
    lg->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    return lg;
}

static void SetState(DWORD state, DWORD win32Exit = NO_ERROR) {
    g_status.dwCurrentState = state;
    g_status.dwWin32ExitCode = win32Exit;
    g_status.dwControlsAccepted = (state == SERVICE_START_PENDING) ? 0 : SERVICE_ACCEPT_STOP;
    SetServiceStatus(g_statusHandle, &g_status);
}

static void WINAPI ServiceCtrlHandler(DWORD ctrl) {
    if (ctrl == SERVICE_CONTROL_STOP) {
        get_logger()->info("Service stop requested.");
        if (g_server) g_server->Stop();
        SetState(SERVICE_STOPPED);
    }
}

static void WINAPI ServiceMain(DWORD, LPTSTR*) {
    g_statusHandle = RegisterServiceCtrlHandlerW(L"PacsCpp", ServiceCtrlHandler);
    if (!g_statusHandle) return;

    ZeroMemory(&g_status, sizeof(g_status));
    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    SetState(SERVICE_START_PENDING);

    auto cfg = Config::Load();

    g_server = std::make_unique<PacsServer>(cfg);
    g_server->Start(); // при пустом db_conn — будет ошибка в лог и сервер не запустится

    SetState(SERVICE_RUNNING);
    get_logger()->info("Service entered running state.");

    while (g_status.dwCurrentState == SERVICE_RUNNING) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

static int RunConsole() {
    auto lg = get_logger();
    lg->info("Starting in console mode (no SCM).");

    auto cfg = Config::Load();
    g_server = std::make_unique<PacsServer>(cfg);
    g_server->Start();

    lg->info("Press Ctrl+C to exit.");
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}

int wmain() {
    SERVICE_TABLE_ENTRYW table[] = {
        { const_cast<LPWSTR>(L"PacsCpp"), ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(table)) {
        return RunConsole();
    }
    return 0;
}
