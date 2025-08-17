#pragma once
#include <string>
#include <vector>
#include <filesystem>

struct Config {
	std::string aeTitle{ "PACS_CPP" };
	unsigned short port{ 11112 };
	std::filesystem::path storagePath{ L"C:\\PACS\\Storage" };
	std::string pgUri{ "postgresql://orthanc:orthanc@localhost:5432/orthanc" };
	unsigned int maxPdu{ 4u * 1024u * 1024u };
	std::vector<std::string> acceptedAEs{ "*" };

	static Config Load(const std::wstring& path);
};
