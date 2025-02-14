#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib") // For PathAppend

// Function to convert WCHAR* to char* for comparison (process names)
std::string WcharToChar(const WCHAR* wcharStr) {
    int len = WideCharToMultiByte(CP_ACP, 0, wcharStr, -1, nullptr, 0, nullptr, nullptr);
    std::string charStr(len, 0);
    WideCharToMultiByte(CP_ACP, 0, wcharStr, -1, &charStr[0], len, nullptr, nullptr);
    return charStr;
}

// Function to find the process ID of Minecraft
DWORD GetProcessID(const char* processName) {
    DWORD processID = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        do {
            std::string processExe = WcharToChar(processEntry.szExeFile); // Convert WCHAR* to char* for comparison
            if (_stricmp(processExe.c_str(), processName) == 0) {
                processID = processEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    CloseHandle(snapshot);
    return processID;
}

// Function to inject the DLL into the process
bool InjectDLL(DWORD processID, const char* dllPath) {
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!process) return false;

    void* allocatedMemory = VirtualAllocEx(process, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocatedMemory) return false;

    WriteProcessMemory(process, allocatedMemory, dllPath, strlen(dllPath) + 1, nullptr);
    HANDLE remoteThread = CreateRemoteThread(process, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, allocatedMemory, 0, nullptr);

    if (!remoteThread) return false;

    CloseHandle(remoteThread);
    CloseHandle(process);
    return true;
}

// Function to get the full path of the EXE (and the DLL in the same folder)
std::string GetCurrentFolder() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer); // Removes the filename from the path
    return std::string(buffer);
}

// Window procedure for the GUI
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {  // If the Inject button is clicked
            const char* processName = "javaw.exe"; // Minecraft process
            std::string exePath = GetCurrentFolder();
            std::string dllPath = exePath + "\\Hack.dll"; // DLL path in the same folder

            DWORD processID = GetProcessID(processName);
            if (!processID) {
                MessageBox(hwnd, L"Minecraft not found!", L"Error", MB_ICONERROR);
                return 0;
            }

            if (InjectDLL(processID, dllPath.c_str())) {
                MessageBox(hwnd, L"DLL successfully injected!", L"Success", MB_ICONINFORMATION);
            }
            else {
                MessageBox(hwnd, L"Injection failed.", L"Error", MB_ICONERROR);
            }
        }
        return 0;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int main() {
    // Register the window class
    const char* className = "InjectorWindowClass";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"InjectorWindowClass";  // Use wide string here
    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindow(L"InjectorWindowClass", L"NiggaShitter", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 150,
        NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) {
        return 0;
    }

    // Create the "Inject" button
    CreateWindow(L"BUTTON", L"Inject", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        100, 50, 100, 30, hwnd, (HMENU)1, wc.hInstance, NULL);

    // Show the window
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
