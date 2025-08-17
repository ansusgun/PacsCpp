#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "Advapi32.lib")

// ====== Имя службы ======
static const wchar_t* SVC = L"PacsCpp";

// ====== Хендлы ======
static HWND g_hStatus = nullptr;
static HWND g_hLog = nullptr;
static HFONT g_hFont = nullptr;

// ====== DPI helpers ======
static UINT g_dpi = 96;
static int  S(int px) { return MulDiv(px, (int)g_dpi, 96); } // scale

static void EnablePerMonitorDpiAwareness()
{
    // Windows 10+: Per-Monitor v2
    auto set_ctx = reinterpret_cast<BOOL(WINAPI*)(HANDLE)>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"),
            "SetProcessDpiAwarenessContext"));
    if (set_ctx) {
        set_ctx((HANDLE)-4 /*DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2*/);
        return;
    }
    // Win 8.1 fallback
    HMODULE shcore = LoadLibraryW(L"Shcore.dll");
    if (shcore) {
        typedef HRESULT(WINAPI* SetAwareness)(int);
        auto f = reinterpret_cast<SetAwareness>(
            GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (f) f(2 /*Per-Monitor*/);
        FreeLibrary(shcore);
    }
    else {
        // Win7 fallback
        auto f2 = reinterpret_cast<BOOL(WINAPI*)(void)>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDPIAware"));
        if (f2) f2();
    }
}

static void RecreateFont(HWND hwnd)
{
    if (g_hFont) { DeleteObject(g_hFont); g_hFont = nullptr; }

    NONCLIENTMETRICSW ncm{ sizeof(ncm) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    ncm.lfMessageFont.lfHeight = -S(12); // примерно 12pt при текущем DPI
    g_hFont = CreateFontIndirectW(&ncm.lfMessageFont);

    if (g_hStatus) SendMessageW(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    if (g_hLog)    SendMessageW(g_hLog, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    // перерисуем
    InvalidateRect(hwnd, nullptr, TRUE);
}

static std::wstring ExpandEnv(const wchar_t* s)
{
    wchar_t buf[32768];
    DWORD n = ExpandEnvironmentStringsW(s, buf, (DWORD)std::size(buf));
    return (n && n < std::size(buf)) ? std::wstring(buf, n - 1) : std::wstring(s);
}

// ====== Служба ======
static bool QuerySvcRunning()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE svc = OpenServiceW(scm, SVC, SERVICE_QUERY_STATUS);
    if (!svc) { CloseServiceHandle(scm); return false; }
    SERVICE_STATUS_PROCESS ssp{};
    DWORD bytes = 0; bool running = false;
    if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
        reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &bytes))
        running = (ssp.dwCurrentState == SERVICE_RUNNING);
    CloseServiceHandle(svc); CloseServiceHandle(scm);
    return running;
}

static bool StartSvc()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE svc = OpenServiceW(scm, SVC, SERVICE_START);
    if (!svc) { CloseServiceHandle(scm); return false; }
    BOOL ok = StartServiceW(svc, 0, nullptr);
    CloseServiceHandle(svc); CloseServiceHandle(scm);
    return ok != FALSE;
}

static bool StopSvc()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) return false;
    SC_HANDLE svc = OpenServiceW(scm, SVC, SERVICE_STOP);
    if (!svc) { CloseServiceHandle(scm); return false; }
    SERVICE_STATUS ss{};
    BOOL ok = ControlService(svc, SERVICE_CONTROL_STOP, &ss);
    CloseServiceHandle(svc); CloseServiceHandle(scm);
    return ok != FALSE;
}

// ====== Логи ======
static std::wstring ReadLogTailUTF16(const std::wstring& path, size_t max_bytes = 64 * 1024)
{
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return L"Log not found:\r\n" + path;

    LARGE_INTEGER size{}; GetFileSizeEx(h, &size);
    ULONGLONG fsz = (ULONGLONG)size.QuadPart;
    ULONGLONG want = (fsz > max_bytes) ? max_bytes : fsz;
    LARGE_INTEGER pos{}; pos.QuadPart = (LONGLONG)((fsz > want) ? (fsz - want) : 0);
    SetFilePointerEx(h, pos, nullptr, FILE_BEGIN);

    std::vector<char> buf((size_t)want + 1, 0);
    DWORD br = 0; ReadFile(h, buf.data(), (DWORD)want, &br, nullptr);
    CloseHandle(h);

    int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf.data(), (int)br, nullptr, 0);
    if (wlen > 0) {
        std::wstring ws; ws.resize(wlen);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf.data(), (int)br, ws.data(), wlen);
        return ws;
    }
    wlen = MultiByteToWideChar(CP_ACP, 0, buf.data(), (int)br, nullptr, 0);
    std::wstring ws; ws.resize((size_t)wlen);
    MultiByteToWideChar(CP_ACP, 0, buf.data(), (int)br, ws.data(), wlen);
    return ws;
}

static void RefreshLog()
{
    const std::wstring logPath = ExpandEnv(L"%ProgramData%\\PacsCpp\\logs\\pacs_service.log");
    std::wstring text = ReadLogTailUTF16(logPath);
    SetWindowTextW(g_hLog, text.c_str());
    SendMessageW(g_hLog, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    SendMessageW(g_hLog, EM_SCROLLCARET, 0, 0);
}

// ====== Layout ======
static void DoLayout(HWND hwnd)
{
    RECT rc; GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    int margin = S(10);
    int spacing = S(8);
    int btnW = S(100);
    int btnH = S(28);

    // координаты верхней линейки
    int x = margin, y = margin;

    // Кнопки создаются в WM_CREATE — здесь только вычисляем правый край зоны кнопок
    int buttonsRight = x + (btnW * 4) + (spacing * 3); // Start, Stop, Open cfg, Open log
    // Статус — от конца кнопок до правого поля
    if (g_hStatus) {
        SetWindowPos(g_hStatus, nullptr,
            buttonsRight + spacing, y + (btnH - S(18)) / 2, // выравнивание по центру
            w - (buttonsRight + spacing + margin), S(22),
            SWP_NOZORDER);
    }

    int contentTop = y + btnH + spacing;

    if (g_hLog) {
        SetWindowPos(g_hLog, nullptr,
            margin, contentTop,
            w - margin * 2, h - contentTop - margin,
            SWP_NOZORDER);
    }
}

static void UpdateStatus(HWND hwnd)
{
    bool r = QuerySvcRunning();
    SetWindowTextW(g_hStatus, r ? L"Running" : L"Stopped");
    SetWindowTextW(hwnd, r ? L"PACS (Running)" : L"PACS (Stopped)");
}

// ====== WinProc ======
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_dpi = GetDpiForWindow(hwnd);
        RecreateFont(hwnd);

        // Кнопки — дети ГЛАВНОГО окна
        CreateWindowW(L"BUTTON", L"Start", WS_CHILD | WS_VISIBLE,
            S(10), S(10), S(100), S(28), hwnd, (HMENU)1, 0, 0);
        CreateWindowW(L"BUTTON", L"Stop", WS_CHILD | WS_VISIBLE,
            S(118), S(10), S(100), S(28), hwnd, (HMENU)2, 0, 0);
        CreateWindowW(L"BUTTON", L"Open config", WS_CHILD | WS_VISIBLE,
            S(226), S(10), S(110), S(28), hwnd, (HMENU)3, 0, 0);
        CreateWindowW(L"BUTTON", L"Open log", WS_CHILD | WS_VISIBLE,
            S(344), S(10), S(100), S(28), hwnd, (HMENU)5, 0, 0);

        g_hStatus = CreateWindowW(L"STATIC", L"...",
            WS_CHILD | WS_VISIBLE, S(452), S(14), S(200), S(22),
            hwnd, (HMENU)4, 0, 0);

        g_hLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
            WS_VSCROLL | ES_AUTOVSCROLL,
            S(10), S(48), S(500), S(300), hwnd, (HMENU)10, 0, 0);

        SendMessageW(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessageW(g_hLog, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        SetTimer(hwnd, 100, 1000, nullptr); // статус
        SetTimer(hwnd, 200, 1000, nullptr); // лог

        UpdateStatus(hwnd);
        RefreshLog();
        DoLayout(hwnd);
        return 0;
    }

    case WM_DPICHANGED:
    {
        g_dpi = HIWORD(w); // новый DPI
        RECT* prc = (RECT*)l; // рекомендуемая новая область
        SetWindowPos(hwnd, nullptr,
            prc->left, prc->top,
            prc->right - prc->left, prc->bottom - prc->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        RecreateFont(hwnd);
        DoLayout(hwnd);
        return 0;
    }

    case WM_SIZE:
        DoLayout(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(w))
        {
        case 1:
            if (!StartSvc())
                MessageBoxW(hwnd, L"Failed to start service. Check logs.", L"PacsTray", MB_ICONERROR);
            UpdateStatus(hwnd);
            break;
        case 2:
            if (!StopSvc())
                MessageBoxW(hwnd, L"Failed to stop service. Check logs.", L"PacsTray", MB_ICONERROR);
            UpdateStatus(hwnd);
            break;
        case 3:
        {
            auto p = ExpandEnv(L"%ProgramData%\\PacsCpp\\config.json");
            ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            break;
        }
        case 5:
        {
            auto p = ExpandEnv(L"%ProgramData%\\PacsCpp\\logs\\pacs_service.log");
            ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            break;
        }
        }
        return 0;

    case WM_TIMER:
        if (w == 100) UpdateStatus(hwnd);
        if (w == 200) RefreshLog();
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 100);
        KillTimer(hwnd, 200);
        if (g_hFont) { DeleteObject(g_hFont); g_hFont = nullptr; }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

int WINAPI wWinMain(HINSTANCE h, HINSTANCE, LPWSTR, int nCmd)
{
    EnablePerMonitorDpiAwareness();

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = h;
    wc.lpszClassName = L"PacsTray";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClassW(&wc);

    // стартовый размер — масштабируемый
    int W = S(800), H = S(600);
    HWND hwnd = CreateWindowW(L"PacsTray", L"PACS",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, W, H, nullptr, nullptr, h, nullptr);

    ShowWindow(hwnd, nCmd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
