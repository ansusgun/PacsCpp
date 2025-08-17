#include <windows.h>
#include <shellapi.h>   

static const wchar_t* SVC = L"PacsCpp";
static HWND hStatus = nullptr;

bool QuerySvcRunning() {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE svc = OpenServiceW(scm, SVC, SERVICE_QUERY_STATUS);
    if (!svc) { CloseServiceHandle(scm); return false; }
    SERVICE_STATUS_PROCESS ssp{};
    DWORD bytes = 0; bool running = false;
    if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
        reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytes)) {
        running = (ssp.dwCurrentState == SERVICE_RUNNING);
    }
    CloseServiceHandle(svc); CloseServiceHandle(scm);
    return running;
}

bool StartSvc() {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE svc = OpenServiceW(scm, SVC, SERVICE_START);
    if (!svc) { CloseServiceHandle(scm); return false; }
    BOOL ok = StartServiceW(svc, 0, nullptr);
    CloseServiceHandle(svc); CloseServiceHandle(scm);
    return ok != FALSE;
}

bool StopSvc() {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE svc = OpenServiceW(scm, SVC, SERVICE_STOP);
    if (!svc) { CloseServiceHandle(scm); return false; }
    SERVICE_STATUS ss{};
    BOOL ok = ControlService(svc, SERVICE_CONTROL_STOP, &ss);
    CloseServiceHandle(svc); CloseServiceHandle(scm);
    return ok != FALSE;
}

void UpdateStatus(HWND hwnd) {
    bool r = QuerySvcRunning();
    SetWindowTextW(hStatus, r ? L"Running" : L"Stopped");
    SetWindowTextW(hwnd, r ? L"PACS (Running)" : L"PACS (Stopped)");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"BUTTON", L"Start", WS_CHILD | WS_VISIBLE, 10, 10, 80, 28, hwnd, (HMENU)1, 0, 0);
        CreateWindowW(L"BUTTON", L"Stop", WS_CHILD | WS_VISIBLE, 100, 10, 80, 28, hwnd, (HMENU)2, 0, 0);
        CreateWindowW(L"BUTTON", L"Open config", WS_CHILD | WS_VISIBLE, 190, 10, 120, 28, hwnd, (HMENU)3, 0, 0);
        hStatus = CreateWindowW(L"STATIC", L"...", WS_CHILD | WS_VISIBLE, 10, 50, 300, 22, hwnd, (HMENU)4, 0, 0);
        SetTimer(hwnd, 100, 1000, nullptr);
        UpdateStatus(hwnd);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(w) == 1) { StartSvc(); UpdateStatus(hwnd); }
        if (LOWORD(w) == 2) { StopSvc();  UpdateStatus(hwnd); }
        if (LOWORD(w) == 3) { ShellExecuteW(nullptr, L"open", L"%ProgramData%\\PacsCpp\\config.json", nullptr, nullptr, SW_SHOWNORMAL); }
        break;
    case WM_TIMER:
        UpdateStatus(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE h, HINSTANCE, LPWSTR, int nCmd) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = h;
    wc.lpszClassName = L"PacsTray";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"PacsTray", L"PACS", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 340, 130, nullptr, nullptr, h, nullptr);
    ShowWindow(hwnd, nCmd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
