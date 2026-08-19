#pragma once

#include <string>
#include <vector>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

struct AttributeOverride {
    int id;
    std::wstring value;
    std::wstring worst;
    std::wstring raw;
};

struct DiskOverride {
    std::wstring modelMatch;
    std::wstring serialMatch;
    std::wstring status;
    std::wstring life;
    std::wstring temperature;
    std::wstring power_on_hours;
    std::wstring power_on_count;
    std::wstring rotation_rate;
    std::wstring host_writes;
    std::wstring host_reads;
    std::wstring capacity_mb;
    std::wstring model_name;
    std::wstring serial_number;
    std::wstring firmware_rev;
    std::vector<AttributeOverride> attributes;
};

class CSmartOverride {
public:
    CSmartOverride();
    virtual ~CSmartOverride();

    bool LoadConfig(const std::wstring& exePath);
    const DiskOverride* GetDiskOverride(const std::wstring& model, const std::wstring& serial);
    std::wstring GetOverriddenAppName();

private:
    std::vector<DiskOverride> m_overrides;
    bool m_isLoaded;
    std::wstring m_appName;
    std::wstring StringToWString(const std::string& s);
};

extern CSmartOverride g_SmartOverride;