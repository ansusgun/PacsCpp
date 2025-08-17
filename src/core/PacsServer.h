#pragma once
#include <string>
#include <thread>
#include <atomic>

#include "Config.h"
#include "PgIndex.h"

class PacsServer {
public:
    explicit PacsServer(const Config& cfg);
    ~PacsServer();

    void Start();
    void Stop();
    bool IsRunning() const;

private:
    void Run();

    std::atomic<bool> stop_{ false };
    std::thread       worker_;
    std::wstring      storageRoot_;
    Config            cfg_;
    PgIndex           index_;   // теперь есть конструктор по умолчанию и с conn string
};
