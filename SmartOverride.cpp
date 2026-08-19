#include "SmartOverride.h"
#include <Windows.h>

CSmartOverride g_SmartOverride;

CSmartOverride::CSmartOverride() : m_isLoaded(false) {}
CSmartOverride::~CSmartOverride() {}

std::wstring CSmartOverride::StringToWString(const std::string& s) {
    if (s.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring ws(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], size);
    return ws;
}

bool CSmartOverride::LoadConfig(const std::wstring& exePath) {
    m_overrides.clear();
    m_isLoaded = false;
    m_appName = L"auto";

    std::wstring configPath = exePath + L"\\override_config.json";
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        return false;
    }

    try {
        json j;
        configFile >> j;

        m_appName = StringToWString(j.value("app_name", "auto"));

        auto getStr = [this](const json& obj, const char* key) -> std::wstring {
            if (obj.contains(key)) {
                if (obj[key].is_string()) {
                    return StringToWString(obj[key].get<std::string>());
                }
                else if (obj[key].is_number()) {
                    return std::to_wstring(obj[key].get<long long>());
                }
            }
            return L"auto";
            };

        if (j.contains("disks") && j["disks"].is_array()) {
            for (const auto& disk : j["disks"]) {
                DiskOverride d;
                if (disk.contains("match")) {
                    d.modelMatch = getStr(disk["match"], "model");
                    d.serialMatch = getStr(disk["match"], "serial");
                }
                else {
                    d.modelMatch = L"auto";
                    d.serialMatch = L"auto";
                }

                d.status = getStr(disk, "status");
                d.life = getStr(disk, "life");
                d.temperature = getStr(disk, "temperature");
                d.power_on_hours = getStr(disk, "power_on_hours");
                d.power_on_count = getStr(disk, "power_on_count");
                d.rotation_rate = getStr(disk, "rotation_rate");
                d.host_writes = getStr(disk, "host_writes");
                d.host_reads = getStr(disk, "host_reads");
                d.capacity_mb = getStr(disk, "capacity_mb");
                d.model_name = getStr(disk, "model_name");
                d.serial_number = getStr(disk, "serial_number");
                d.firmware_rev = getStr(disk, "firmware_rev");

                if (disk.contains("attributes") && disk["attributes"].is_array()) {
                    for (const auto& attr : disk["attributes"]) {
                        AttributeOverride a;
                        a.id = attr.value("id", -1);
                        if (a.id == -1) continue;

                        a.value = getStr(attr, "value");
                        a.worst = getStr(attr, "worst");
                        a.raw = getStr(attr, "raw");

                        d.attributes.push_back(a);
                    }
                }
                m_overrides.push_back(d);
            }
        }
        m_isLoaded = true;
    }
    catch (...) {
        return false;
    }
    return true;
}

const DiskOverride* CSmartOverride::GetDiskOverride(const std::wstring& model, const std::wstring& serial) {
    if (!m_isLoaded) return nullptr;
    for (const auto& d : m_overrides) {
        bool modelOk = (d.modelMatch == L"auto" || d.modelMatch.empty() || d.modelMatch == model);
        bool serialOk = (d.serialMatch == L"auto" || d.serialMatch.empty() || d.serialMatch == serial);
        if (modelOk && serialOk) {
            return &d;
        }
    }
    return nullptr;
}

std::wstring CSmartOverride::GetOverriddenAppName() {
    if (!m_isLoaded) return L"auto";
    return m_appName;
}