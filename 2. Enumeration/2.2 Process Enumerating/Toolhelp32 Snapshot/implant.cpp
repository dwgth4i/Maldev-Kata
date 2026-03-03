#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <set>

std::set<DWORD> GetCurrentProcesses()
{
    std::set<DWORD> pids;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        
        if (Process32FirstW(hSnapshot, &pe32))
        {
            do {
                pids.insert(pe32.th32ProcessID);
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return pids;
}

int main()
{
    std::set<DWORD> previousPids = GetCurrentProcesses();
    
    std::wcout << L"[*] Monitoring process creation (polling)...\n";
    
    while (true)
    {
        Sleep(500); // Poll every 500ms
        
        std::set<DWORD> currentPids = GetCurrentProcesses();
        
        // Find new processes
        for (DWORD pid : currentPids)
        {
            if (previousPids.find(pid) == previousPids.end())
            {
                // New process detected
                HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (hSnapshot != INVALID_HANDLE_VALUE)
                {
                    PROCESSENTRY32W pe32;
                    pe32.dwSize = sizeof(PROCESSENTRY32W);
                    
                    if (Process32FirstW(hSnapshot, &pe32))
                    {
                        do {
                            if (pe32.th32ProcessID == pid)
                            {
                                std::wcout << L"[+] Process Created | PID=" << pid
                                           << L" Name=" << pe32.szExeFile << std::endl;
                                break;
                            }
                        } while (Process32NextW(hSnapshot, &pe32));
                    }
                    CloseHandle(hSnapshot);
                }
            }
        }
        
        previousPids = currentPids;
    }
    
    return 0;
}