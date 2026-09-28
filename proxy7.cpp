#include <windows.h>
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "log.h"

// ============================================================
// 全局状态
// ============================================================
static PROCESS_INFORMATION g_potpi      = {0};
static HWND                g_potwnd     = nullptr;
static char                g_current_url[MAX_PATH * 8] = {0};
static bool                g_playing    = false;
static double              g_duration   = 0.0;
static double              g_position   = 0.0;

// ============================================================
// PotPlayer 路径查找
// ============================================================
static const wchar_t* FindPotPlayerPath() {
    static wchar_t path[MAX_PATH] = {0};
    if (path[0]) return path;

    const wchar_t* candidates[] = {
        L"C:\\Program Files\\DAUM\\PotPlayer\\PotPlayerMini64.exe",
        L"C:\\Program Files (x86)\\DAUM\\PotPlayer\\PotPlayerMini64.exe",
        L"D:\\Program Files\\DAUM\\PotPlayer\\PotPlayerMini64.exe",
        L"D:\\Program Files (x86)\\DAUM\\PotPlayer\\PotPlayerMini64.exe",
        nullptr
    };
    for (int i = 0; candidates[i]; i++) {
        if (GetFileAttributesW(candidates[i]) != INVALID_FILE_ATTRIBUTES) {
            wcscpy_s(path, candidates[i]);
            return path;
        }
    }
    return nullptr;
}

// ============================================================
// PotPlayer 控制
// ============================================================
static void KillPotPlayer() {
    if (g_potpi.hProcess) {
        TerminateProcess(g_potpi.hProcess, 0);
        CloseHandle(g_potpi.hThread);
        CloseHandle(g_potpi.hProcess);
        g_potpi = {0};
    }
    g_potwnd = nullptr;
    g_playing = false;
    ProxyLog("[POT] killed");
}

static void StartPotPlayer(const char* url) {
    if (!url || url[0] == 0) return;

    KillPotPlayer();

    const wchar_t* potPath = FindPotPlayerPath();
    if (!potPath) { ProxyLog("[POT] PotPlayer not found"); return; }
    ProxyLog("[POT] using: %ls", potPath);

    wchar_t wurl[8192] = {0};
    MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 8192);

    // /new 开新实例，避免复用已有 PotPlayer 的窗口
    // 不加 /nosound（用户要声音）
    // 不加 /fullscreen（保持窗口模式）
    wchar_t cmdline[16384] = {0};
    swprintf_s(cmdline, L"\"%s\" \"%s\" /new", potPath, wurl);

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessW(potPath, cmdline, nullptr, nullptr, FALSE,
                        0, nullptr, nullptr, &si, &pi)) {
        ProxyLog("[POT] CreateProcess failed: %lu", GetLastError());
        return;
    }
    g_potpi = pi;
    g_playing = true;
    ProxyLog("[POT] started, pid=%lu", pi.dwProcessId);
}

// ============================================================
// 假数据表
// ============================================================
static const char* FakeConfig(const char* key) {
    if (!key) return "";

    if (strcmp(key, "file.size") == 0)        return "1000000000";
    if (strcmp(key, "video.width") == 0)      return "1920";
    if (strcmp(key, "video.height") == 0)     return "1080";

    if (strcmp(key, "audio.list") == 0)       return "eng,English";
    if (strcmp(key, "audio.track") == 0)      return "0";

    if (strcmp(key, "subtitle.list") == 0)    return "";
    if (strcmp(key, "subtitle.offset") == 0)  return "-1000,-1000";

    if (strcmp(key, "video.hdrhave") == 0)    return "0";
    if (strcmp(key, "video.hdruse") == 0)     return "0";
    if (strcmp(key, "video.process") == 0)    return "100";

    if (strcmp(key, "play.stats") == 0) {
        return "APlayer_version=5.0.1.40;"
               "APlayer_url=;"
               "APlayer_duration=0;"
               "APlayer_width=1920;"
               "APlayer_height=1080;"
               "APlayer_videoCodecName=hevc;"
               "APlayer_audioCodecName=aac;"
               "APlayer_video_render=potplayer;"
               "APlayer_audio_render=potplayer;"
               "APlayer_isHwdecoder=1;"
               "APlayer_playResult=1;"
               "APlayer_playTime=0;"
               "APlayer_drop_frame_count=0;"
               "APlayer_render_frame_count=0;";
    }

    return "";
}

// ============================================================
// 导出函数
// ============================================================
extern "C" {

__declspec(dllexport) void* aplayer_create() {
    ProxyLog("[CALL] aplayer_create()");
    static int dummy = 0x12345678;
    return &dummy;
}

__declspec(dllexport) int aplayer_destroy(void* p) {
    ProxyLog("[CALL] aplayer_destroy(%p)", p);
    KillPotPlayer();
    return 0;
}

__declspec(dllexport) int aplayer_init() {
    ProxyLog("[CALL] aplayer_init()");
    return 0;
}

__declspec(dllexport) int aplayer_uninit() {
    ProxyLog("[CALL] aplayer_uninit()");
    return 0;
}

__declspec(dllexport) int aplayer_open(void* p, const char* url) {
    ProxyLog("[CALL] aplayer_open(%p, url_len=%zu)", p, url ? strlen(url) : 0);
    if (url) strncpy_s(g_current_url, url, sizeof(g_current_url) - 1);

    // 直接把 URL 传给 PotPlayer，完全不经过原版 APlayer
    if (url) StartPotPlayer(url);
    return 0;
}

__declspec(dllexport) int aplayer_close(void* p) {
    ProxyLog("[CALL] aplayer_close(%p)", p);
    KillPotPlayer();
    g_position = 0.0;
    return 0;
}

__declspec(dllexport) int aplayer_play(void* p) {
    ProxyLog("[CALL] aplayer_play(%p)", p);
    g_playing = true;
    return 0;
}

__declspec(dllexport) int aplayer_pause(void* p) {
    ProxyLog("[CALL] aplayer_pause(%p)", p);
    // 可选：让代理层也响应暂停（转发给 PotPlayer 窗口）
    // 但 PotPlayer 是独立窗口，需要先找到它的窗口句柄
    return 0;
}

__declspec(dllexport) int aplayer_set_position(void* p, double sec) {
    ProxyLog("[CALL] aplayer_set_position(%p, %.3f)", p, sec);
    g_position = sec;
    return 0;
}

__declspec(dllexport) double aplayer_get_position(void* p) {
    return g_position;
}

__declspec(dllexport) double aplayer_get_duration(void* p) {
    return g_duration;
}

__declspec(dllexport) int aplayer_get_state(void* p) {
    return g_playing ? 3 : 0;
}

__declspec(dllexport) int aplayer_get_video_width(void* p) {
    return 1920;
}

__declspec(dllexport) int aplayer_get_video_height(void* p) {
    return 1080;
}

__declspec(dllexport) int aplayer_get_volume(void* p) {
    return 100;
}

__declspec(dllexport) int aplayer_set_volume(void* p, int v) {
    ProxyLog("[CALL] aplayer_set_volume(%p, %d)", p, v);
    return 0;
}

__declspec(dllexport) int aplayer_is_seeking(void* p) {
    return 0;
}

__declspec(dllexport) int aplayer_get_buffer_progress(void* p) {
    return 100;
}

__declspec(dllexport) int aplayer_set_view(void* p, HWND hwnd) {
    ProxyLog("[CALL] aplayer_set_view(%p, HWND=0x%p)", p, hwnd);
    // 独立窗口模式下，这个 hwnd 暂时不用
    return 0;
}

__declspec(dllexport) void aplayer_set_callback(void* p, void* cb, void* ud) {
    ProxyLog("[CALL] aplayer_set_callback(%p, cb=%p, ud=%p)", p, cb, ud);
}

__declspec(dllexport) int aplayer_set_config(void* p, const char* key, const char* val) {
    return 0;
}

__declspec(dllexport) const char* aplayer_get_config(void* p, const char* key) {
    return FakeConfig(key);
}

__declspec(dllexport) const char* aplayer_get_config_list(void* p) {
    return "";
}

__declspec(dllexport) const char* aplayer_get_version() {
    return "5.0.1.40";
}

__declspec(dllexport) void* aplayer_get_graph(void* p) {
    return nullptr;
}

__declspec(dllexport) int aplayer_log(const char* msg) {
    return 0;
}

} // extern "C"

// ============================================================
// DLL 入口
// ============================================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            ProxyLog("========== Proxy APlayer.dll (完全替身/独立窗口) attached ==========");
            break;
        case DLL_PROCESS_DETACH:
            ProxyLog("========== Proxy APlayer.dll detached ==========");
            KillPotPlayer();
            break;
    }
    return TRUE;
}