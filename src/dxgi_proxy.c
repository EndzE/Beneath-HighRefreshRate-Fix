#include <windows.h>
#include <stdio.h>
#include <stdint.h>

static HMODULE hRealDxgi = NULL;
static char g_iniPath[MAX_PATH] = "";
static int g_logging = 1;
static int g_targetHz = 165;
static int g_uncapFps = 1;

static void LoadRealDxgi(void) {
    if (!hRealDxgi) {
        char path[MAX_PATH];
        GetSystemDirectoryA(path, MAX_PATH);
        strcat(path, "\\dxgi.dll");
        hRealDxgi = LoadLibraryA(path);
    }
}

__declspec(dllexport) HRESULT WINAPI CreateDXGIFactory(REFIID riid, void **ppFactory) {
    LoadRealDxgi();
    typedef HRESULT (WINAPI *pfn_t)(REFIID, void**);
    pfn_t p = (pfn_t)GetProcAddress(hRealDxgi, "CreateDXGIFactory");
    return p ? p(riid, ppFactory) : E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void **ppFactory) {
    LoadRealDxgi();
    typedef HRESULT (WINAPI *pfn_t)(REFIID, void**);
    pfn_t p = (pfn_t)GetProcAddress(hRealDxgi, "CreateDXGIFactory1");
    return p ? p(riid, ppFactory) : E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, void **ppFactory) {
    LoadRealDxgi();
    typedef HRESULT (WINAPI *pfn_t)(UINT, REFIID, void**);
    pfn_t p = (pfn_t)GetProcAddress(hRealDxgi, "CreateDXGIFactory2");
    return p ? p(Flags, riid, ppFactory) : E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI DXGIDumpJournal(void) {
    LoadRealDxgi();
    typedef HRESULT (WINAPI *pfn_t)(void);
    pfn_t p = (pfn_t)GetProcAddress(hRealDxgi, "DXGIDumpJournal");
    return p ? p() : S_OK;
}

__declspec(dllexport) HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void **pDebug) {
    LoadRealDxgi();
    typedef HRESULT (WINAPI *pfn_t)(UINT, REFIID, void**);
    pfn_t p = (pfn_t)GetProcAddress(hRealDxgi, "DXGIGetDebugInterface1");
    return p ? p(Flags, riid, pDebug) : E_FAIL;
}

__declspec(dllexport) HRESULT WINAPI DXGIReportAdapterConfiguration(void) {
    LoadRealDxgi();
    typedef HRESULT (WINAPI *pfn_t)(void);
    pfn_t p = (pfn_t)GetProcAddress(hRealDxgi, "DXGIReportAdapterConfiguration");
    return p ? p() : S_OK;
}

typedef void* MonoDomain;
typedef void* MonoThread;
typedef void* MonoImage;
typedef void* MonoClass;
typedef void* MonoMethod;
typedef void* MonoObject;
typedef void* MonoMethodHeader;

typedef MonoDomain*       (*mono_get_root_domain_pfn)(void);
typedef MonoThread*       (*mono_thread_attach_pfn)(MonoDomain*);
typedef MonoImage*        (*mono_image_loaded_pfn)(const char*);
typedef MonoClass*        (*mono_class_from_name_pfn)(MonoImage*, const char*, const char*);
typedef MonoMethod*       (*mono_class_get_method_from_name_pfn)(MonoClass*, const char*, int);
typedef MonoObject*       (*mono_runtime_invoke_pfn)(MonoMethod*, void*, void**, MonoObject**);
typedef void*             (*mono_object_unbox_pfn)(MonoObject*);
typedef MonoMethodHeader* (*mono_method_get_header_pfn)(MonoMethod*);
typedef const unsigned char* (*mono_method_header_get_code_pfn)(MonoMethodHeader*, uint32_t*, uint32_t*);
typedef void*             (*mono_compile_method_pfn)(MonoMethod*);

static void InitConfigPath(HINSTANCE hinstDLL) {
    GetModuleFileNameA(hinstDLL, g_iniPath, MAX_PATH);
    char* p = strrchr(g_iniPath, '\\');
    if (p) {
        *(p + 1) = '\0';
        strcat(g_iniPath, "BeneathDeltaTimeFix.ini");
    } else {
        strcpy(g_iniPath, "BeneathDeltaTimeFix.ini");
    }
}

static void LoadConfig(void) {
    if (GetFileAttributesA(g_iniPath) == INVALID_FILE_ATTRIBUTES) {
        FILE* f = fopen(g_iniPath, "w");
        if (f) {
            fputs("[Settings]\n", f);
            fputs("; Target display refresh rate in Hz (e.g. 60, 75, 120, 144, 165, 240, 360)\n", f);
            fputs("; The mod sets Unity's Time.fixedDeltaTime = 1 / TargetHz to eliminate camera judder & ghosting\n", f);
            fputs("TargetHz=165\n\n", f);
            fputs("; Uncap Unity target frame rate (-1 disables Unity engine frame cap)\n", f);
            fputs("; 1 = Uncapped (recommended), 0 = Game default\n", f);
            fputs("UncapFramerate=1\n\n", f);
            fputs("; Enable diagnostic logging to BeneathDeltaTimeFix.log\n", f);
            fputs("; 1 = Enabled, 0 = Disabled\n", f);
            fputs("Logging=1\n", f);
            fclose(f);
        }
    }

    g_targetHz = GetPrivateProfileIntA("Settings", "TargetHz", 165, g_iniPath);
    if (g_targetHz < 30) g_targetHz = 30;
    if (g_targetHz > 1000) g_targetHz = 1000;

    g_uncapFps = GetPrivateProfileIntA("Settings", "UncapFramerate", 1, g_iniPath);
    g_logging = GetPrivateProfileIntA("Settings", "Logging", 1, g_iniPath);
}

static void LogMsg(const char* msg) {
    if (!g_logging) return;
    FILE* f = fopen("BeneathDeltaTimeFix.log", "a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%02d:%02d:%02d.%03d] %s\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
        fclose(f);
    }
}

static void PatchMethodGravity(mono_class_get_method_from_name_pfn mono_class_get_method_from_name,
                               mono_method_get_header_pfn mono_method_get_header,
                               mono_method_header_get_code_pfn mono_method_header_get_code,
                               mono_compile_method_pfn mono_compile_method,
                               MonoClass* klass, const char* methodName, int paramCount, float scale) {
    if (!klass || !mono_class_get_method_from_name || !mono_method_get_header || !mono_method_header_get_code) return;
    MonoMethod* m = mono_class_get_method_from_name(klass, methodName, paramCount);
    if (!m) return;
    MonoMethodHeader* hdr = mono_method_get_header(m);
    if (!hdr) return;
    uint32_t code_size = 0, max_stack = 0;
    unsigned char* code = (unsigned char*)mono_method_header_get_code(hdr, &code_size, &max_stack);
    if (!code || code_size < 11) return;

    DWORD oldProt;
    if (VirtualProtect(code, code_size, PAGE_EXECUTE_READWRITE, &oldProt)) {
        int count = 0;
        for (uint32_t i = 0; i <= code_size - 11; i++) {
            // ldc.r4 30.0f (0x02, 0x22, 0x00, 0x00, 0xF0, 0x41, 0x7D)
            if (code[i] == 0x02 && code[i+1] == 0x22 && code[i+2] == 0x00 && code[i+3] == 0x00 && code[i+4] == 0xF0 && code[i+5] == 0x41 && code[i+6] == 0x7D) {
                float newMult = 30.0f * scale;
                *(float*)(code + i + 2) = newMult;
                count++;
            }
            // ldc.r4 3.0f (0x02, 0x22, 0x00, 0x00, 0x40, 0x40, 0x7D)
            if (code[i] == 0x02 && code[i+1] == 0x22 && code[i+2] == 0x00 && code[i+3] == 0x00 && code[i+4] == 0x40 && code[i+5] == 0x40 && code[i+6] == 0x7D) {
                float newMult = 3.0f * scale;
                *(float*)(code + i + 2) = newMult;
                count++;
            }
        }
        VirtualProtect(code, code_size, oldProt, &oldProt);
        if (count > 0) {
            if (mono_compile_method) mono_compile_method(m);
            char buf[160];
            sprintf(buf, "Compensated player falling gravity in %s (x%.2f scale, new multiplier: %.1f).", methodName, scale, 30.0f * scale);
            LogMsg(buf);
        }
    }
}

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    LoadConfig();

    char startMsg[160];
    sprintf(startMsg, "=== Beneath DeltaTime Native Mod Initialized (Target: %d Hz, Uncap: %d) ===", g_targetHz, g_uncapFps);
    LogMsg(startMsg);

    HMODULE hMono = NULL;
    while (!hMono) {
        Sleep(50);
        hMono = GetModuleHandleA("mono-2.0-bdwgc.dll");
    }
    LogMsg("Detected mono-2.0-bdwgc.dll loaded.");

    mono_get_root_domain_pfn mono_get_root_domain = (mono_get_root_domain_pfn)GetProcAddress(hMono, "mono_get_root_domain");
    mono_thread_attach_pfn mono_thread_attach = (mono_thread_attach_pfn)GetProcAddress(hMono, "mono_thread_attach");
    mono_image_loaded_pfn mono_image_loaded = (mono_image_loaded_pfn)GetProcAddress(hMono, "mono_image_loaded");
    mono_class_from_name_pfn mono_class_from_name = (mono_class_from_name_pfn)GetProcAddress(hMono, "mono_class_from_name");
    mono_class_get_method_from_name_pfn mono_class_get_method_from_name = (mono_class_get_method_from_name_pfn)GetProcAddress(hMono, "mono_class_get_method_from_name");
    mono_runtime_invoke_pfn mono_runtime_invoke = (mono_runtime_invoke_pfn)GetProcAddress(hMono, "mono_runtime_invoke");
    mono_object_unbox_pfn mono_object_unbox = (mono_object_unbox_pfn)GetProcAddress(hMono, "mono_object_unbox");
    mono_method_get_header_pfn mono_method_get_header = (mono_method_get_header_pfn)GetProcAddress(hMono, "mono_method_get_header");
    mono_method_header_get_code_pfn mono_method_header_get_code = (mono_method_header_get_code_pfn)GetProcAddress(hMono, "mono_method_header_get_code");
    mono_compile_method_pfn mono_compile_method = (mono_compile_method_pfn)GetProcAddress(hMono, "mono_compile_method");

    if (!mono_get_root_domain || !mono_thread_attach || !mono_image_loaded || !mono_class_from_name || !mono_class_get_method_from_name || !mono_runtime_invoke) {
        LogMsg("ERROR: Could not resolve Mono runtime function pointers.");
        return 0;
    }

    MonoDomain* domain = NULL;
    while (!domain) {
        Sleep(50);
        domain = mono_get_root_domain();
    }
    LogMsg("Obtained Mono root domain.");
    mono_thread_attach(domain);

    // Wait until UnityEngine.CoreModule is loaded
    MonoImage* coreImg = NULL;
    while (!coreImg) {
        Sleep(100);
        coreImg = mono_image_loaded("UnityEngine.CoreModule");
        if (!coreImg) coreImg = mono_image_loaded("UnityEngine");
    }

    // Time methods
    MonoClass* timeClass = NULL;
    while (!timeClass) {
        Sleep(50);
        timeClass = mono_class_from_name(coreImg, "UnityEngine", "Time");
    }
    MonoMethod* set_fixedDeltaTime = mono_class_get_method_from_name(timeClass, "set_fixedDeltaTime", 1);
    MonoMethod* get_fixedDeltaTime = mono_class_get_method_from_name(timeClass, "get_fixedDeltaTime", 0);
    MonoMethod* set_maximumDeltaTime = mono_class_get_method_from_name(timeClass, "set_maximumDeltaTime", 1);

    MonoClass* appClass = mono_class_from_name(coreImg, "UnityEngine", "Application");
    MonoMethod* set_targetFrameRate = appClass ? mono_class_get_method_from_name(appClass, "set_targetFrameRate", 1) : NULL;

    LogMsg("Resolved UnityEngine.Time and Application methods.");

    // Target timestep calculated from configured TargetHz
    float target_dt = 1.0f / (float)g_targetHz;
    float max_dt = (target_dt < 0.05f) ? 0.05f : (target_dt * 1.5f);
    int target_fps = -1;

    void* params_dt[1] = { &target_dt };
    void* params_max[1] = { &max_dt };
    void* params_fps[1] = { &target_fps };

    if (set_fixedDeltaTime) mono_runtime_invoke(set_fixedDeltaTime, NULL, params_dt, NULL);
    if (set_maximumDeltaTime) mono_runtime_invoke(set_maximumDeltaTime, NULL, params_max, NULL);
    if (g_uncapFps && set_targetFrameRate) mono_runtime_invoke(set_targetFrameRate, NULL, params_fps, NULL);

    char applyMsg[160];
    sprintf(applyMsg, "Applied Time.fixedDeltaTime = %.6f (%d Hz)%s", target_dt, g_targetHz, g_uncapFps ? " & uncapped framerate." : ".");
    LogMsg(applyMsg);

    // Wait until Assembly-CSharp is loaded
    MonoImage* gameImg = NULL;
    while (!gameImg) {
        Sleep(100);
        gameImg = mono_image_loaded("Assembly-CSharp");
    }
    LogMsg("Assembly-CSharp image resolved.");

    // Compensate player falling gravity multiplier so falling speed remains vanilla normal
    float gravityScale = (float)g_targetHz / 50.0f;
    MonoClass* fpcClass = mono_class_from_name(gameImg, "UnityStandardAssets.Characters.FirstPerson", "FirstPersonController");
    if (fpcClass) {
        PatchMethodGravity(mono_class_get_method_from_name, mono_method_get_header, mono_method_header_get_code, mono_compile_method, fpcClass, "PhysicsTick", 0, gravityScale);
        PatchMethodGravity(mono_class_get_method_from_name, mono_method_get_header, mono_method_header_get_code, mono_compile_method, fpcClass, "FixedUpdate_OLD", 0, gravityScale);
    }
    MonoClass* fpc2Class = mono_class_from_name(gameImg, "UnityStandardAssets.Characters.FirstPerson", "FirstPersonController2");
    if (fpc2Class) {
        PatchMethodGravity(mono_class_get_method_from_name, mono_method_get_header, mono_method_header_get_code, mono_compile_method, fpc2Class, "FixedUpdate4", 0, gravityScale);
    }

    int loopCount = 0;

    // Watchdog loop:
    // 1. Keeps configured DeltaTime enforced across scene loads & deaths
    // 2. Dynamically reloads BeneathDeltaTimeFix.ini if modified while running
    while (1) {
        Sleep(250);
        loopCount++;

        // Check for on-the-fly config changes every ~1 second (every 4 loops)
        if (loopCount % 4 == 0) {
            int newHz = GetPrivateProfileIntA("Settings", "TargetHz", g_targetHz, g_iniPath);
            if (newHz >= 30 && newHz <= 1000 && newHz != g_targetHz) {
                g_targetHz = newHz;
                target_dt = 1.0f / (float)g_targetHz;
                max_dt = (target_dt < 0.05f) ? 0.05f : (target_dt * 1.5f);
                if (set_fixedDeltaTime) mono_runtime_invoke(set_fixedDeltaTime, NULL, params_dt, NULL);
                if (set_maximumDeltaTime) mono_runtime_invoke(set_maximumDeltaTime, NULL, params_max, NULL);
                
                // Re-compensate gravity for new Hz
                gravityScale = (float)g_targetHz / 50.0f;
                if (fpcClass) {
                    PatchMethodGravity(mono_class_get_method_from_name, mono_method_get_header, mono_method_header_get_code, mono_compile_method, fpcClass, "PhysicsTick", 0, gravityScale);
                }
                if (fpc2Class) {
                    PatchMethodGravity(mono_class_get_method_from_name, mono_method_get_header, mono_method_header_get_code, mono_compile_method, fpc2Class, "FixedUpdate4", 0, gravityScale);
                }

                char buf[128];
                sprintf(buf, "Config reloaded: TargetHz changed to %d Hz (dt: %.6f, grav: %.1f)", g_targetHz, target_dt, 30.0f * gravityScale);
                LogMsg(buf);
            }
        }

        if (get_fixedDeltaTime && mono_object_unbox) {
            MonoObject* exc = NULL;
            MonoObject* res = mono_runtime_invoke(get_fixedDeltaTime, NULL, NULL, &exc);
            if (res && !exc) {
                float cur_dt = *(float*)mono_object_unbox(res);
                if (cur_dt > (target_dt + 0.0005f) || cur_dt < (target_dt - 0.0005f)) {
                    if (set_fixedDeltaTime) mono_runtime_invoke(set_fixedDeltaTime, NULL, params_dt, NULL);
                    if (set_maximumDeltaTime) mono_runtime_invoke(set_maximumDeltaTime, NULL, params_max, NULL);
                    if (g_uncapFps && set_targetFrameRate) mono_runtime_invoke(set_targetFrameRate, NULL, params_fps, NULL);
                    char buf[128];
                    sprintf(buf, "fixedDeltaTime was reset by game (%.4f) -> Re-enforced %d Hz (%.6f)", cur_dt, g_targetHz, target_dt);
                    LogMsg(buf);
                }
            }
        }
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        InitConfigPath(hinstDLL);
        CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
    }
    return TRUE;
}
