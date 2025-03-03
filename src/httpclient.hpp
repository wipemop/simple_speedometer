#pragma once
#pragma comment(lib, "urlmon.lib")

#include <urlmon.h>
#include <string>
#include <sstream>
#include <format>

std::string WideToNarrow(const std::wstring& wstr) {
    int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
    return str;
}

namespace HTTPClient {
    std::string GetRequest(const std::wstring& wUrl) {
        IStream* stream;
        std::string url = WideToNarrow(wUrl); // Convert wstring to string
        HRESULT result = URLOpenBlockingStreamA(0, url.c_str(), &stream, 0, 0); // Use ANSI version
        if (result != S_OK) {
            return "An error occurred.";
        }

        const unsigned long chunkSize = 128;
        char buffer[chunkSize];
        unsigned long bytesRead;
        std::stringstream strStream;

        while (stream->Read(buffer, chunkSize, &bytesRead) == S_OK && bytesRead > 0) {
            strStream.write(buffer, bytesRead);
        }

        stream->Release();
        return strStream.str();
    }
}