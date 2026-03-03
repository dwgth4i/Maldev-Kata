#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "user32.lib")

int main() {
    MessageBoxA(NULL, "Hello from the 1st MessageBox!", "Injected", MB_OK);
    MessageBoxA(NULL, "Hello from the 2nd MessageBox!", "Injected", MB_OK);
    MessageBoxA(NULL, "Hello from the 3rd MessageBox!", "Injected", MB_OK);
    return 0;
}