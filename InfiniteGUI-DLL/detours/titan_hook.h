#pragma once
#include <Windows.h>

#if defined(__MINGW32__) || defined(__MINGW64__) || (defined(__GNUC__) && defined(_WIN32))
// MinGW 交叉构建：Detours 的 MSVC 静态库无法与 MinGW 链接，改用 MinHook
#include <MinHook.h>

template <typename T>
class TitanHook {
public:
	void InitHook(void* targetFunc, void* myFunc) {
		targetFunc_ = targetFunc;
		myFunc_ = myFunc;
	}

	void SetHook() {
		if (targetFunc_ && myFunc_)
		{
			MH_STATUS st = MH_Initialize();
			if (st != MH_OK && st != MH_ERROR_ALREADY_INITIALIZED)
				return;
			MH_CreateHook(targetFunc_, myFunc_, reinterpret_cast<LPVOID*>(&originalFunc_));
			MH_EnableHook(targetFunc_);
		}
	}

	T GetOrignalFunc() {
		return (T)originalFunc_;
	}

	void* GetTargetFunc() {
		return targetFunc_;
	}

	void* GetMyFunc() {
		return myFunc_;
	}

	void RemoveHook() {
		if (targetFunc_)
		{
			MH_DisableHook(targetFunc_);
			MH_RemoveHook(targetFunc_);
		}
	}

	~TitanHook() {
		RemoveHook();
	}
private:
	void* targetFunc_ = 0;
	void* myFunc_ = 0;
	T originalFunc_ = nullptr;
};
#else
// MSVC 原路径：Detours
#include "detours/include/detours.h"
#include "lazy_importer.hpp"
#pragma comment(lib, "detours.lib")
template <typename T>
class TitanHook {
public:
	void InitHook(void* targetFunc, void* myFunc) {
		targetFunc_ = targetFunc;
		myFunc_ = myFunc;
	}

	void SetHook() {
		if (targetFunc_ && myFunc_)
		{
			DetourTransactionBegin();
			DetourUpdateThread(LI_FN(GetCurrentThread)());
			DetourAttach(&(LPVOID&)targetFunc_, myFunc_);

			DetourTransactionCommit();

		}
	}

	T GetOrignalFunc() {
		return (T)targetFunc_;
	}

	void* GetTargetFunc() {
		return targetFunc_;
	}

	void* GetMyFunc() {
		return myFunc_;
	}

	void RemoveHook() {
		if (targetFunc_)
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());
			DetourDetach(&(LPVOID&)targetFunc_, myFunc_);
			DetourTransactionCommit();
		}
	}

	~TitanHook() {
		RemoveHook();
	}
private:
	void* targetFunc_ = 0;
	void* myFunc_ = 0;
};
#endif