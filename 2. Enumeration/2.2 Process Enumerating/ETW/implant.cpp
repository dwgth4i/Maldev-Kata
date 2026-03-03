#define INITGUID
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <iostream>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "tdh.lib")

#define SESSION_NAME L"ETW_Process_Monitor"

TRACEHANDLE gSession = 0;
TRACEHANDLE gTrace = 0;

void WINAPI EventRecordCallback(PEVENT_RECORD event)
{
    if (event->EventHeader.EventDescriptor.Opcode != 1)
        return; // Process start only

    DWORD pid = event->EventHeader.ProcessId;
    DWORD tid = event->EventHeader.ThreadId;

    std::wcout << L"[+] Process Created | PID=" << pid
               << L" TID=" << tid << std::endl;
}

int wmain()
{
    BYTE buffer[sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAX_PATH * sizeof(WCHAR)] = {};
    auto props = (PEVENT_TRACE_PROPERTIES)buffer;

    props->Wnode.BufferSize = sizeof(buffer);
    props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    props->Wnode.ClientContext = 1;
    props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    wcscpy_s(
        (PWCHAR)((PBYTE)props + props->LoggerNameOffset),
        MAX_PATH,
        SESSION_NAME
    );

    ULONG status = StartTraceW(&gSession, SESSION_NAME, props);
    if (status != ERROR_SUCCESS)
    {
        std::cerr << "StartTrace failed: " << status << std::endl;
        return 1;
    }

    status = EnableTraceEx2(
        gSession,
        &SystemTraceControlGuid,
        EVENT_CONTROL_CODE_ENABLE_PROVIDER,
        TRACE_LEVEL_INFORMATION,
        EVENT_TRACE_FLAG_PROCESS,
        0,
        0,
        nullptr
    );

    if (status != ERROR_SUCCESS)
    {
        std::cerr << "EnableTraceEx2 failed: " << status << std::endl;
        return 1;
    }

    EVENT_TRACE_LOGFILEW trace = {};
    trace.LoggerName = (LPWSTR)SESSION_NAME;
    trace.ProcessTraceMode =
        PROCESS_TRACE_MODE_REAL_TIME |
        PROCESS_TRACE_MODE_EVENT_RECORD;
    trace.EventRecordCallback = EventRecordCallback;

    gTrace = OpenTraceW(&trace);
    if (gTrace == INVALID_PROCESSTRACE_HANDLE)
    {
        std::cerr << "OpenTrace failed" << std::endl;
        return 1;
    }

    std::wcout << L"[*] Monitoring process creation...\n";
    ProcessTrace(&gTrace, 1, nullptr, nullptr);

    ControlTraceW(gSession, SESSION_NAME, props, EVENT_TRACE_CONTROL_STOP);
    return 0;
}