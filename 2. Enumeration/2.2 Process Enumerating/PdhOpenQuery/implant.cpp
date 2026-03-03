#include <windows.h>
#include <pdh.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "pdh.lib")

std::vector<DWORD> GetPidsByNamePDH(const wchar_t* processName) {
    std::vector<DWORD> pids;
    PDH_HQUERY hQuery = NULL;
    PDH_FMT_COUNTERVALUE counterValue = {};

    if (PdhOpenQueryW(NULL, 0, &hQuery) != ERROR_SUCCESS) {
        return pids;
    }

    // Step 1: Enumerate all instances of the process name
    DWORD instanceBufSize = 0;
    DWORD counterBufSize  = 0;

    // First call to get required buffer sizes
    PdhEnumObjectItemsW(
        NULL, NULL,
        L"Process",
        NULL, &counterBufSize,
        NULL, &instanceBufSize,
        PERF_DETAIL_WIZARD, 0
    );

    std::vector<wchar_t> counterBuf(counterBufSize);
    std::vector<wchar_t> instanceBuf(instanceBufSize);

    // Second call to actually fill the buffers
    if (PdhEnumObjectItemsW(
        NULL, NULL,
        L"Process",
        counterBuf.data(), &counterBufSize,
        instanceBuf.data(), &instanceBufSize,
        PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
    {
        PdhCloseQuery(hQuery);
        return pids;
    }

    // Step 2: Walk the double-null-terminated instance list
    // and collect all instances that match processName
    // e.g. "notepad", "notepad#1", "notepad#2"
    std::vector<std::wstring> matchedInstances;
    wchar_t* p = instanceBuf.data();
    while (*p) {
        std::wstring instance(p);
        // Match exact name OR name#N pattern
        if (_wcsicmp(instance.c_str(), processName) == 0 ||
            (instance.find(processName) == 0 && instance[wcslen(processName)] == L'#'))
        {
            matchedInstances.push_back(instance);
        }
        p += instance.size() + 1;
    }

    if (matchedInstances.empty()) {
        std::wcout << L"[-] No instances found for: " << processName << std::endl;
        PdhCloseQuery(hQuery);
        return pids;
    }

    // Step 3: For each matched instance, add a counter and collect its PID
    std::vector<PDH_HCOUNTER> counters(matchedInstances.size());
    for (size_t i = 0; i < matchedInstances.size(); i++) {
        wchar_t counterPath[512];
        swprintf_s(counterPath, L"\\Process(%s)\\ID Process", matchedInstances[i].c_str());

        PdhAddCounterW(hQuery, counterPath, 0, &counters[i]);
    }

    // This delay can help ensure that there’s enough time for the counter data to update before attempting to read it.
    Sleep(100);
    PdhCollectQueryData(hQuery);

    for (size_t i = 0; i < matchedInstances.size(); i++) {
        if (PdhGetFormattedCounterValue(counters[i], PDH_FMT_LONG, NULL, &counterValue) == ERROR_SUCCESS) {
            DWORD pid = static_cast<DWORD>(counterValue.longValue);
            std::wcout << L"[+] " << matchedInstances[i] << L" -> PID: " << pid << std::endl;
            pids.push_back(pid);
        }
    }

    PdhCloseQuery(hQuery);
    return pids;
}

int main() {
    auto pids = GetPidsByNamePDH(L"notepad");

    if (pids.empty()) {
        std::wcout << L"[-] No notepad processes found." << std::endl;
    } else {
        std::wcout << L"[+] Found " << pids.size() << L" instance(s)." << std::endl;
    }

    return 0;
}