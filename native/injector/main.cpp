// lumin-loader: finds the running Minecraft client and injects lumin.dll
// into it with the classic CreateRemoteThread(LoadLibraryW) technique.
//
// The DLL is self-sufficient: once loaded it waits for the JVM, the bridge mod
// and OpenGL on its own thread, so it is safe to inject during startup.

#include <windows.h>
#include <tlhelp32.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct ProcessInfo {
    DWORD pid = 0;
    std::wstring path;
    std::wstring title;
};

std::wstring executableDirectory() {
    wchar_t buffer[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);

    std::wstring full(buffer);
    const size_t slash = full.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"." : full.substr(0, slash);
}

bool fileExists(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

std::wstring windowTitleOf(DWORD pid) {
    struct Query {
        DWORD pid;
        std::wstring title;
    } query{pid, L""};

    EnumWindows(
        [](HWND window, LPARAM param) -> BOOL {
            auto* state = reinterpret_cast<Query*>(param);

            DWORD owner = 0;
            GetWindowThreadProcessId(window, &owner);
            if (owner != state->pid) return TRUE;
            if (!IsWindowVisible(window)) return TRUE;

            wchar_t text[256] = {0};
            GetWindowTextW(window, text, 255);
            if (text[0] == L'\0') return TRUE;

            state->title = text;
            return FALSE;
        },
        reinterpret_cast<LPARAM>(&query));

    return query.title;
}

std::vector<ProcessInfo> findJavaProcesses() {
    std::vector<ProcessInfo> found;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return found;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, L"javaw.exe") != 0 &&
                _wcsicmp(entry.szExeFile, L"java.exe") != 0) {
                continue;
            }

            ProcessInfo info;
            info.pid = entry.th32ProcessID;
            info.title = windowTitleOf(info.pid);

            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, info.pid);
            if (process) {
                wchar_t path[MAX_PATH * 2] = {0};
                DWORD size = static_cast<DWORD>(std::size(path));
                if (QueryFullProcessImageNameW(process, 0, path, &size)) {
                    info.path = path;
                }
                CloseHandle(process);
            }

            found.push_back(std::move(info));
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return found;
}

bool isMinecraft(const ProcessInfo& info) {
    if (!info.title.empty() && info.title.find(L"Minecraft") != std::wstring::npos) {
        return true;
    }
    // Fall back to the install path for launchers that title the window later.
    return info.path.find(L".minecraft") != std::wstring::npos ||
           info.path.find(L"MultiMC") != std::wstring::npos ||
           info.path.find(L"PrismLauncher") != std::wstring::npos;
}

bool sameArchitecture(HANDLE process) {
    BOOL targetWow64 = FALSE;
#if defined(_WIN64)
    if (!IsWow64Process(process, &targetWow64)) return false;
    return targetWow64 == FALSE; // 64-bit loader, 64-bit target
#else
    (void)process;
    return true;
#endif
}

bool inject(HANDLE process, const std::wstring& dllPath) {
    const SIZE_T bytes = (dllPath.size() + 1) * sizeof(wchar_t);

    void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_READWRITE);
    if (!remote) {
        std::wprintf(L"[!] VirtualAllocEx failed (%lu)\n", GetLastError());
        return false;
    }

    if (!WriteProcessMemory(process, remote, dllPath.c_str(), bytes, nullptr)) {
        std::wprintf(L"[!] WriteProcessMemory failed (%lu)\n", GetLastError());
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(kernel, "LoadLibraryW"));
    if (!loadLibrary) {
        std::wprintf(L"[!] could not resolve LoadLibraryW\n");
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
    if (!thread) {
        std::wprintf(L"[!] CreateRemoteThread failed (%lu)\n", GetLastError());
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    const DWORD wait = WaitForSingleObject(thread, 15000);

    DWORD moduleHandle = 0;
    if (wait == WAIT_OBJECT_0) {
        GetExitCodeThread(thread, &moduleHandle);
    }

    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);

    if (wait != WAIT_OBJECT_0) {
        std::wprintf(L"[!] LoadLibraryW did not return within 15 s\n");
        return false;
    }
    if (moduleHandle == 0) {
        std::wprintf(L"[!] the game rejected the DLL (LoadLibrary returned NULL)\n");
        return false;
    }

    return true;
}

void usage() {
    std::printf(
        "lumin-loader\n"
        "  --list            list running Java processes\n"
        "  --pid <n>         inject into this pid\n"
        "  --dll <path>      DLL to inject (default: lumin.dll next to the loader)\n"
        "  --wait <seconds>  keep polling for Minecraft before giving up (default 0)\n"
        "  --help            this text\n");
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    std::wstring dll = executableDirectory() + L"\\lumin.dll";
    DWORD targetPid = 0;
    int waitSeconds = 0;
    bool listOnly = false;

    for (int i = 1; i < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--help" || arg == L"-h") {
            usage();
            return 0;
        } else if (arg == L"--list") {
            listOnly = true;
        } else if (arg == L"--pid" && i + 1 < argc) {
            targetPid = static_cast<DWORD>(_wtoi(argv[++i]));
        } else if (arg == L"--dll" && i + 1 < argc) {
            dll = argv[++i];
        } else if (arg == L"--wait" && i + 1 < argc) {
            waitSeconds = _wtoi(argv[++i]);
        } else {
            std::wprintf(L"[!] unknown argument: %s\n", arg.c_str());
            usage();
            return 2;
        }
    }

    if (!fileExists(dll)) {
        std::wprintf(L"[!] %s not found\n", dll.c_str());
        return 1;
    }

    if (listOnly) {
        const auto processes = findJavaProcesses();
        if (processes.empty()) {
            std::puts("no Java processes running");
            return 0;
        }
        for (const auto& info : processes) {
            std::wprintf(L"%6lu  %-28s  %s\n", info.pid,
                         info.path.empty() ? L"<unknown path>" : info.path.c_str(),
                         info.title.empty() ? L"<no window>" : info.title.c_str());
        }
        return 0;
    }

    // Pick the target: explicit pid, then a window titled Minecraft, then the only
    // Java process, and otherwise show the user what is available.
    const int attempts = waitSeconds > 0 ? waitSeconds * 2 : 1;
    for (int attempt = 0; attempt < attempts; ++attempt) {
        auto processes = findJavaProcesses();

        if (targetPid != 0) {
            for (const auto& info : processes) {
                if (info.pid == targetPid) {
                    std::wprintf(L"[*] injecting into pid %lu\n", info.pid);
                    HANDLE process = OpenProcess(
                        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                            PROCESS_VM_WRITE | PROCESS_VM_READ,
                        FALSE, info.pid);
                    if (!process) {
                        std::wprintf(L"[!] OpenProcess failed (%lu) -- run as administrator\n",
                                     GetLastError());
                        return 1;
                    }
                    if (!sameArchitecture(process)) {
                        std::puts("[!] architecture mismatch: the game is 64-bit, build the loader for x64");
                        CloseHandle(process);
                        return 1;
                    }
                    const bool ok = inject(process, dll);
                    CloseHandle(process);
                    if (ok) std::puts("[+] injected; check lumin.log next to the DLL");
                    return ok ? 0 : 1;
                }
            }
            if (attempt + 1 < attempts) {
                Sleep(500);
                continue;
            }
            std::wprintf(L"[!] pid %lu is not a running Java process\n", targetPid);
            return 1;
        }

        const ProcessInfo* chosen = nullptr;
        for (const auto& info : processes) {
            if (info.title.find(L"Minecraft") != std::wstring::npos) {
                chosen = &info;
                break;
            }
        }
        if (!chosen && processes.size() == 1) chosen = &processes.front();

        if (!chosen) {
            if (attempt + 1 < attempts) {
                Sleep(500);
                continue;
            }
            if (processes.empty()) {
                std::puts("[!] no javaw.exe found -- start Minecraft first, or pass --wait 60");
            } else {
                std::puts("[!] several Java processes are running; pick one:");
                for (const auto& info : processes) {
                    std::wprintf(L"      --pid %lu   %s\n", info.pid,
                                 info.title.empty() ? info.path.c_str() : info.title.c_str());
                }
            }
            return 1;
        }

        std::wprintf(L"[*] found Minecraft: pid %lu (%s)\n", chosen->pid,
                     chosen->title.empty() ? L"no window" : chosen->title.c_str());

        HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                         PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                                         PROCESS_VM_READ,
                                     FALSE, chosen->pid);
        if (!process) {
            std::wprintf(L"[!] OpenProcess failed (%lu) -- run as administrator\n", GetLastError());
            return 1;
        }
        if (!sameArchitecture(process)) {
            std::puts("[!] architecture mismatch: the game is 64-bit, build the loader for x64");
            CloseHandle(process);
            return 1;
        }

        std::wprintf(L"[*] injecting %s\n", dll.c_str());
        const bool ok = inject(process, dll);
        CloseHandle(process);

        if (ok) {
            std::puts("[+] injected; check lumin.log next to the DLL");
            return 0;
        }
        return 1;
    }

    return 1;
}
