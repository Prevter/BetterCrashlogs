#include "hwinfo.hpp"

#include <fmt/format.h>
#include <sstream>
#include "geode-util.hpp"

#include <Windows.h>
#include <intrin.h>
#include <atlbase.h>
#include <comdef.h>
#include <dxgi.h>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "dxgi.lib")

namespace hwinfo {

    namespace ram {
        uint64_t total() {
            MEMORYSTATUSEX status;
            status.dwLength = sizeof(status);
            GlobalMemoryStatusEx(&status);
            return status.ullTotalPhys / 1024 / 1024;
        }

        uint64_t used() {
            MEMORYSTATUSEX status;
            status.dwLength = sizeof(status);
            GlobalMemoryStatusEx(&status);
            return (status.ullTotalPhys - status.ullAvailPhys) / 1024 / 1024;
        }

        uint64_t free() {
            return total() - used();
        }
    }

    namespace swap {
        uint64_t total() {
            MEMORYSTATUSEX status;
            status.dwLength = sizeof(status);
            GlobalMemoryStatusEx(&status);
            auto total = status.ullTotalPageFile / 1024 / 1024;
            return total - ram::total();
        }

        uint64_t used() {
            MEMORYSTATUSEX status;
            status.dwLength = sizeof(status);
            GlobalMemoryStatusEx(&status);
            auto used = (status.ullTotalPageFile - status.ullAvailPageFile) / 1024 / 1024;
            return used - ram::used();
        }

        uint64_t free() {
            return total() - used();
        }
    }

    std::string getCPUName() {
        std::array<int, 4> integerBuffer = {};
        constexpr size_t sizeofIntegerBuffer = sizeof(int) * integerBuffer.size();

        std::array<char, 64> charBuffer = {};
        constexpr std::array<int, 4> functionIds = {
            static_cast<int>(0x8000'0002),
            static_cast<int>(0x8000'0003),
            static_cast<int>(0x8000'0004)
        };

        std::string cpu;

        for (auto& id : functionIds) {
            __cpuid(integerBuffer.data(), id);
            std::memcpy(charBuffer.data(), integerBuffer.data(), sizeofIntegerBuffer);
            cpu += std::string(charBuffer.data());
        }

        cpu.erase(std::find_if(cpu.rbegin(), cpu.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), cpu.end());
        return cpu;
    }

    std::string getGPUName() {
        CComPtr<IDXGIFactory> factory;
        if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory))))
            return "Unknown GPU";

        CComPtr<IDXGIAdapter> adapter;
        if (FAILED(factory->EnumAdapters(0, &adapter)))
            return "Unknown GPU";

        DXGI_ADAPTER_DESC desc;
        if (FAILED(adapter->GetDesc(&desc)))
            return "Unknown GPU";

        auto result = desc.Description;
        return _com_util::ConvertBSTRToString(result);
    }

    uint32_t getCPUCores() {
        static int coreCount = 0;

        if (coreCount == 0) {
            FILE *fp;
            char var[5] = {0};

            fp = _popen("wmic cpu get NumberOfCores", "r");
            while (fgets(var,sizeof(var),fp) != nullptr) {
                if (isdigit(var[0])) {
                    coreCount = std::atoi(var);
                    break;
                }
            }
            _pclose(fp);
        }

        return coreCount;
    }

    uint32_t getCPUThreads() {
        return std::thread::hardware_concurrency();
    }

    bool GetOSVersionString(VS_FIXEDFILEINFO* version) {
        WCHAR path[_MAX_PATH] = {};
        if (!GetSystemDirectoryW(path, _MAX_PATH))
            return false;

        wcscat_s(path, L"\\kernel32.dll");

        DWORD handle;
        DWORD len = GetFileVersionInfoSizeExW(FILE_VER_GET_NEUTRAL, path, &handle);
        if (!len) return false;

        std::unique_ptr<uint8_t> buff(new (std::nothrow) uint8_t[len]);
        if (!buff) return false;

        if (!GetFileVersionInfoExW(FILE_VER_GET_NEUTRAL, path, 0, len, buff.get()))
            return false;

        VS_FIXEDFILEINFO* vInfo = nullptr;
        UINT infoSize;

        if (!VerQueryValueW(buff.get(), L"\\", reinterpret_cast<LPVOID*>(&vInfo), &infoSize))
            return false;

        if (!infoSize)
            return false;

        *version = *vInfo;
        return true;
    }

    std::string getOSName() {
        std::stringstream os;
        os << "Windows ";

        // Version
        VS_FIXEDFILEINFO version = {};
        uint32_t productVersionHigh, productVersionLow, buildNumber = 0, revisionNumber;
        if (GetOSVersionString(&version))
        {
            productVersionHigh = (version.dwProductVersionMS >> 16) & 0xffff;
            productVersionLow = (version.dwProductVersionMS >> 0) & 0xffff;
            buildNumber = (version.dwProductVersionLS >> 16) & 0xffff;
            revisionNumber = (version.dwProductVersionLS >> 0) & 0xffff;
        }

        // https://en.wikipedia.org/wiki/List_of_Microsoft_Windows_versions
        #define OS_CASE(number, name) case number: os << name; break
        #define OS_CASE2(number, name) if (buildNumber >= number) { os << name; break; }
        switch (buildNumber) {
            OS_CASE(20348, "Server 2022 ");
            OS_CASE(19042, "Server, version 20H2 ");
            OS_CASE(19041, "Server, version 2004 ");
            OS_CASE(18363, "Server, version 1909 ");
            OS_CASE(18362, "Server, version 1903 ");
            OS_CASE(17763, "Server 2019 ");
            OS_CASE(17134, "Server, version 1803 ");
            OS_CASE(16299, "Server, version 1709 ");
            OS_CASE(14393, "Server 2016 ");
            default:
                OS_CASE2(22000, "11 ")
                OS_CASE2(10240, "10 ")
                OS_CASE2(9600, "8.1 ")
                OS_CASE2(9200, "8 ")
                OS_CASE2(7601, "7 ")
                OS_CASE2(6002, "Vista ")
                OS_CASE2(2600, "XP ")
                OS_CASE2(0, "Unknown ")
                break;
        }
        #undef OS_CASE
        #undef OS_CASE2

        _SYSTEM_INFO sysinfo{};
        GetNativeSystemInfo(&sysinfo);

        // Architecture
#define ARCH_CASE(name, title) case PROCESSOR_ARCHITECTURE_##name: os << title; break
        switch (sysinfo.wProcessorArchitecture)
        {
            ARCH_CASE(AMD64, "amd64");
            ARCH_CASE(ARM, "armeabi-v7a");
            ARCH_CASE(ARM64, "armv8");
            ARCH_CASE(IA64, "ia-64");
            ARCH_CASE(INTEL, "i386");
            default: os << "Unknown";
        }
#undef ARCH_CASE

        // Build number
        os << fmt::format(" (v.{}.{}.{}.{})", productVersionHigh, productVersionLow, buildNumber, revisionNumber);

        return os.str();
    }

    std::string getMessage() {
        static std::string message;
        if (!message.empty()) {
            return message;
        }

        message = fmt::format(
            "- CPU: {} ({} cores, {} threads)\n"
            "- GPU: {}\n"
            "- RAM: {} MB total, {} MB used, {} MB free\n"
            "- SWAP: {} MB total, {} MB used, {} MB free\n"
            "- OS: {}\n",
            getCPUName(), getCPUCores(), getCPUThreads(),
            getGPUName(),
            ram::total(), ram::used(), ram::free(),
            swap::total(), swap::used(), swap::free(),
            getOSName()
        );

        return message;
    }

}