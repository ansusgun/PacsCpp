#include <windows.h>
#include <memory>
#include <atomic>
#include "PacsServer.h"

static SERVICE_STATUS_HANDLE g_status = nullptr;
static std::unique_ptr<PacsServer> g_server;
static std::atomic<bool> g_stop{ false };

static void Report(DWORD state, DWORD waitHint = 0) {
    SERVICE_STATUS ss{};
    ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ss.dwCurrentState = state;
    ss.dwWaitHint = waitHint;
    ss.dwControlsAccepted = (state == SERVICE_RUNNING) ? (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN) : 0;
    SetServiceStatus(g_status, &ss);
}

static void WINAPI CtrlHandler(DWORD code) {
    if (code == SERVICE_CONTROL_STOP || code == SERVICE_CONTROL_SHUTDOWN) {
        g_stop = true;
        if (g_server) g_server->Stop();
        Report(SERVICE_STOP_PENDING, 2000);
    }
}

static void WINAPI ServiceMain(DWORD, LPTSTR*) {
    g_status = RegisterServiceCtrlHandlerW(L"PacsCpp", CtrlHandler);
    if (!g_status) return;

    Report(SERVICE_START_PENDING, 3000);
    try {
        g_server = std::make_unique<PacsServer>(L"%ProgramData%\\PacsCpp\\config.json");
        g_server->Start();
    }
    catch (...) {
        Report(SERVICE_STOPPED);
        return;
    }
    Report(SERVICE_RUNNING);

    while (!g_stop) Sleep(500);
    Report(SERVICE_STOPPED);
}

int wmain() {
    SERVICE_TABLE_ENTRYW table[] = {
      { const_cast<LPWSTR>(L"PacsCpp"), ServiceMain },
      { nullptr, nullptr }
    };
    StartServiceCtrlDispatcherW(table);
    return 0;
}
