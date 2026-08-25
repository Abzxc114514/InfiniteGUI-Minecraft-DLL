// dllmain.cpp : 定义 DLL 应用程序的入口点。
//#include "pch.h"
//#include "Run.h"
#include "Init.hpp"
// DllMain：在 DLL 注入后创建线程初始化 Hook，卸载时清理
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    g_hModule = hModule;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // 禁用线程库通知，优化
        DisableThreadLibraryCalls(hModule);
        // 等待线程初始化完成
        //CreateThread(NULL, 0, InitThread, NULL, 0, NULL);
        CreateThread(NULL, 0, MainApp, NULL, 0, NULL);

        break;

    case DLL_PROCESS_DETACH:
	    opengl_hook::g_isDetaching.store(true, std::memory_order_release);
        break;
    }
    return TRUE;
}