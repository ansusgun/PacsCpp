#pragma once
#include "Config.h"
#include "PgIndex.h"
#include <atomic>
#include <thread>
#include <filesystem>
#include <string>

class PacsServer {
public:
	explicit PacsServer(const std::wstring& configPath);
	~PacsServer();

	void Start();
	void Stop();

private:
	void Run();

	std::filesystem::path storageRoot_;
	Config cfg_;
	std::atomic<bool> stop_{ false };
	std::thread worker_;
};
